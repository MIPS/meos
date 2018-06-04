from imgtec.console import *
import imgtec.codescape
import time, sys, os
from collections import OrderedDict
import project

load_bin = True
load_sym = True
for opt in sys.argv:
    if opt.lower() == "-load_binary_only":
        load_sym = False
    elif opt.lower() == "-load_source_only":
        load_bin = False

@command()
def inittlbs(device=None):
    '''Initialise the TLBs, e.g. after a reset.'''
    tlb_type = get_TLB_type()
    if tlb_type == 0 or tlb_type == 6:  # if no TLB
        print 'No TLB'
        return

    cop_configs = get_available_COP_configs()
    if not does_config_exist(cop_configs, 1): # if no COP Config registers
        print 'COP Config registers not found'
        return

    print 'Initializing TLBs'
    num_of_mmu_entries = get_num_of_MMU_entries(tlb_type, cop_configs)
    make_all_TLB_available()
    invalidatetlb(num_of_mmu_entries)

@command()
def invalidatetlb(num_of_mmu_entries, device=None):
    '''Invalidate the TLBs to a system wide unique value'''
    cpu_num = get_native_core_number()
    addr = (0x80000000 | (cpu_num << 20)) + 0x400
    dump = tlbd()
    def probe(dump, addr):
        for i in dump:
          if (i.EntryHi & 0xffffe000) == (addr & ~(i.PageMask | 0x1fff)):
            return True
        return False
    for index in range(0, num_of_mmu_entries + 1):
        while probe(dump, addr) == True:
          addr += 0x2000
        tlb(index, [0, 0, addr , 0])
        addr += 0x2000

@command()
def initcaches(device=None):
    '''Detect and initialise the caches, e.g. after a reset.'''
    cop_configs = get_available_COP_configs()
    if not does_config_exist(cop_configs, 1): # if no COP Config registers
        print 'COP Config registers not found'
        return

    print 'Initializing Caches'
    reset_L1_ICache()
    reset_L1_DCache()

    # On the first core only
    if get_native_core_number() == 0 and does_config_exist(cop_configs, 2):
        at_least_L2_exists = get_L2_cache_line_size()
        if at_least_L2_exists:
            enable_L2_3_cache(cop_configs, False)
            reset_L2_cache()
            reset_L3_cache()
            enable_L2_3_cache(cop_configs, True)

def get_native_core_number():
    return regs('EBase') & 0x3ff

def get_TLB_type():
    return (regs('Config') & 0x380) >> 7

def get_available_COP_configs():
    MAX_COP_CONFIGS = 8
    result_set = set()
    for i in range(MAX_COP_CONFIGS):
        result_set.add(i)
        another_cfg_exists = regs('Config%s' % (str(i) if i > 0 else '')) & 0x80000000
        if not another_cfg_exists:
            break
    return result_set

def does_config_exist(cop_configs, config_num):
    return config_num in cop_configs

def get_the_traditional_MMU_entry_size():
    return regs('Config1').MMUSize

def get_num_of_MMU_entries(tlb_type, cop_configs):
    # Get the traditional MMU size
    entries = get_the_traditional_MMU_entry_size()

    # Cores with C0_CONFIG4 change the interpretation of this MMU size
    if does_config_exist(cop_configs, 4):
        try:
            ext_def = regs('Config4').MMUExtDef
            if (int(tlb_type) == 4) and (int(ext_def) == 3):
                # FTLBs - defined by C0_CONFIG4:FTLB_*, above VTLBs
                ways = 1 << int(regs('Config4').FTLB_Ways)
                sets = 1 << int(regs('Config4').FTLB_Sets)
                entries += ways * sets
            elif ext_def:
                # No FTLBs - C0_CONFIG4:ExtVTLBs extends C0_CONFIG1:MMUSize
                entries |= (long(regs('Config4')) & 0x7f) << 6
        except:
            pass
    return entries

def make_all_TLB_available():
    regs('Wired', 0)

def GCR_BASE():
  if regs("CMGCRBASE") != 0:
    return ((regs("CMGCRBASE") & 0xffffc00) << 4) + 0xa0000000
  else:
    return 0xbfbf8008

def enable_L2_3_cache(cop_configs, enable):
    if does_config_exist(cop_configs, 3):
        try:
            # Enable / Disable CCA in GCR_Base
            wordmodify(GCR_BASE(), 0 if enable else 0x50, 0xff)
        except RuntimeError:
            pass

def twotopowerof(value):
    return 2 << value if value else 0

def get_L1_Icache_line_size():
    return twotopowerof(regs('Config1').IL)

def get_L1_Dcache_line_size():
    return twotopowerof(regs('Config1').DL)

def get_L2_cache_line_size():
    try:
        GCR_L2_CONFIG = word(GCR_BASE() + 0x130)
        if GCR_L2_CONFIG & 0x80000000 == 0:
            raise RuntimeError('No GCR L2')
        return twotopowerof((GCR_L2_CONFIG & 0xf00) >> 8)
    except:
        return twotopowerof((regs('Config2') & 0x000000f0) >> 4)

def get_L3_cache_line_size():
    return twotopowerof((regs('Config2') & 0x00f00000) >> 20)

def reset_L1_ICache():
    L1_I_line_size = get_L1_Icache_line_size()
    if L1_I_line_size:
        regs('ITagLo', 0)
        regs('ITagHi', 0)
        config1 = regs('Config1')
        i_lines = (64 << config1.IS) * (config1.IA + 1)
        print 'Initializing L1I: ' + str(i_lines) + ' sets, ' + str(L1_I_line_size) + ' byte line size'
        cacheop(icache, 0x80000000, L1_I_line_size, 2, count=i_lines)
    else:
        print 'No L1 I Cache'

def reset_L1_DCache():
    L1_D_line_size = get_L1_Dcache_line_size()
    if L1_D_line_size:
        regs('DTagLo', 0)
        regs('DTagHi', 0)
        config1 = regs('Config1')
        d_lines = (64 << config1.DS) * (config1.DA + 1)
        print 'Initializing L1D: ' + str(d_lines) + ' sets, ' + str(L1_D_line_size) + ' byte line size'
        cacheop(dcache, 0x80000000, L1_D_line_size, 2, count=d_lines)
    else:
        print 'No L1 D Cache'

def reset_L2_cache():
    l2_line_size = get_L2_cache_line_size()
    if l2_line_size: # do we have an L2 Cache
        GCR_L2_CONFIG = word(GCR_BASE() + 0x130)

        def reset_L2_cache_gcr():
            GCR_L2_RAM_CONFIG = word(GCR_BASE() + 0x240)
            if GCR_L2_RAM_CONFIG & 0x60000000 == 0x60000000:
              print 'Self initialising L2 Cache'
              return
            sets = 0x40 << ((GCR_L2_CONFIG & 0xf000) >> 12)
            assoc = (GCR_L2_CONFIG & 0xff) + 1
            l2_lines = sets * assoc
            word(GCR_BASE() + 0x600, [0, 0, 0, 0, 0, 0, 0, 0]) # Zero tag addr, state, data, ecc
            wordmodify(GCR_BASE() + 0x130, 1 << 26, 1 << 26)
            print 'Initializing L2 via GCR: ' + str(l2_lines) + ' sets, ' + str(l2_line_size) + ' byte line size'
            cacheop(l2cache, 0x80000000, l2_line_size, 2, count=l2_lines)
            wordmodify(GCR_BASE() + 0x130, 0, 1 << 20)

        def reset_L2_cache_old():
            regs('L23TagLo', 0)
            # cp0.29.4 is L23TAGHI, currently this has no name because no known mips
            # cores implement it.  This write here is just belt and braces in case a
            # core comes along with this register.
            regs('cp0.29.4', 0)
            config2 = regs('Config2')
            l2_lines = (64 << ((config2 & 0x00000f00) >> 8)) * ((config2 & 0x0000000f) + 1)
            print 'Initializing L2: ' + str(l2_lines) + ' sets, ' + str(l2_line_size) + ' byte line size'
            cacheop(l2cache, 0x80000000, l2_line_size, 2, count=l2_lines)

        if GCR_L2_CONFIG & 0x80000000 == 0:
            return reset_L2_cache_old()
        else:
            return reset_L2_cache_gcr()
    else:
        print 'No L2 Cache'

def reset_L3_cache():
    l3_line_size = get_L3_cache_line_size()
    if l3_line_size: # do we have an L3 Cache
        config2 = regs('Config2')
        l3_lines = (64 << ((config2 & 0x0f000000) >> 24)) * (((config2 & 0x000f0000) >> 16) + 1)
        print 'Initializing L3: ' + str(l3_lines) + ' sets, ' + str(l3_line_size) + ' byte line size'
        cacheop(l3cache, 0x80000000, l3_line_size, 2, count=l3_lines)
    else:
        print 'No L3 Cache'

def bind_tc(tc, vpe):
    '''Bind a TC to the current device. Trashes 0xa0000004.'''
    word(0xa0000004, values=[0x7000003f])
    if tclist()[tc].tcbind.CurVPE != vpe:
      tcactive(tc)
      regs("TCHalt", 1)
      regs("TCStatus", regs("TCStatus") &~ 0x2000)
      tcactive(-1)
      before = device().tiny.GetThread()
      device().tiny.SetThread(vpe)
      device().tiny.BindTC(tc, 0xa0000004)
      device().tiny.SetThread(before)
    else:
      tcactive(tc)
      regs("TCHalt", 1)
      regs("TCStatus", regs("TCStatus") &~ 0x2000)
    tcactive(-1)
    cmdall(project.rhalt)

KSEG1 = 0xA0000000
GCR_CPC_BASE = 0x88
GCR_CL_COHERENCE = 0x2008
GCR_CL_RESET_RELEASE = 0x2000
GCR_CL_OTHER_REG = 0x2018
GCR_CO_COHERENCE = 0x4008
GCR_CO_RESET_RELEASE = 0x4000
CPC_CL_CMD_REG = 0x2000
CPC_CL_STAT_CONF = 0x2008
CPC_CL_OTHER_REG = 0x2010
CPC_CO_CMD_REG = 0x4000
CPC_CO_STAT_CONF = 0x4008

@command()
def cpc_command(command=None,device=None):
    """ Send the CPC a Command - Leave command empty to read status"""

    if device.probe.identifier.split(' ')[0] == 'DA-net' or device.probe.identifier.split(' ')[0] == 'Simulator' or device.probe.identifier.split(' ')[0] == 'RemoteImperas':
        #find where the GCRS are located.
        gcr_base= (regs('cmgcrbase')<<4) + KSEG1 # requires a classic KSEG1 region and GCRS to be < 512MB.
        cpc_base= word(gcr_base+GCR_CPC_BASE)
        cpc_base_virt = (cpc_base & 0xFFFF0000) + KSEG1 # requires a classic KSEG1 region and GCRS to be < 512MB.

        #enable gcr reset and coherence
        word(gcr_base+GCR_CL_COHERENCE, 0xff)
        word(gcr_base+GCR_CL_RESET_RELEASE, 0)

        #enable cpc register access
        word(gcr_base+GCR_CPC_BASE,cpc_base | 0x1)

        if command != None:
            word(cpc_base_virt + 0x2000,command,verify=False)

            time.sleep(2)

            try:
                runstate() #Will fail with unexpected reset (if power up or reset command is sent).
            except:
                pass

        #disable cpc register access
        word(gcr_base+GCR_CPC_BASE,cpc_base & ~0x1)

    else:

        #enable cpc reg access
        cpc_base = regs('gcr_cpc_base')
        #enable gcr reset and coherence
        regs('gcr_cl_coherence', 0xff)
        try:
            regs('gcr_cl_reset_release', 0)
        except:
            pass

        if command != None:

            regs('cpc_cl_cmd_reg',command)

            time.sleep(2)

            try:
                runstate() #Will fail with unexpected reset (if power up or reset command is sent).
            except:
                pass

        #disable cpc register access
        regs('gcr_cpc_base',cpc_base & ~1)

class use_other_core():
    def __enter__(self):
        config("gcr core other",True) #redirect *_cl_* (core local) registers to *_co_* (core other) registers.
    def __exit__(self,type, value, traceback):
        config("gcr core other",False)


@command()
def cpc_command_other_core(core,command=None,device=None):
    """ Send the CPC a Command for an 'other' core ie not the one we are halted on currently
        Leave command empty to read status"""

    if device.probe.identifier.split(' ')[0] == 'DA-net' or device.probe.identifier.split(' ')[0] == 'Simulator' or device.probe.identifier.split(' ')[0] == 'RemoteImperas':
        #find where the GCRS are located.
        gcr_base= (regs('cmgcrbase')<<4) + KSEG1 # requires a classic KSEG1 region and GCRS to be < 512MB.
        cpc_base= word(gcr_base+GCR_CPC_BASE)
        cpc_base_virt = (cpc_base & 0xFFFF0000) + KSEG1 # requires a classic KSEG1 region and GCRS to be < 512MB.

        word(gcr_base + GCR_CL_OTHER_REG,(core & 0xFF) << 16)
        word(cpc_base_virt + CPC_CL_OTHER_REG,(core & 0xFF) << 16)

        #enable cpc register access
        word(gcr_base+GCR_CPC_BASE,cpc_base | 0x1)

        word(cpc_base_virt + CPC_CL_OTHER_REG,(core & 0xFF) << 16)
        #enable gcr reset and coherence
        word(gcr_base+GCR_CO_COHERENCE, 0xff)
        word(gcr_base+GCR_CO_RESET_RELEASE, 0)

        if command != None:
            word(cpc_base_virt + CPC_CO_CMD_REG,command,verify=False)

            time.sleep(2)

            try:
                runstate() #Will fail with unexpected reset (if power up or reset command is sent).
            except:
                pass

        #disable cpc register access
        word(gcr_base+GCR_CPC_BASE,cpc_base & ~0x1)

    else: #SysProbe

        #enable cpc reg access
        cpc_base = regs('gcr_cpc_base')
        regs('gcr_cpc_base',cpc_base | 1)
        regs('gcr_cl_other',(core & 0xFF) << 16)
        regs('cpc_cl_other_reg',(core & 0xFF) << 16)

        with use_other_core():

            #enable gcr reset and coherence
            regs('gcr_cl_coherence', 0xff)
            try:
                regs('gcr_cl_reset_release', 0)
            except:
                pass


            if command != None:

                regs('cpc_cl_cmd_reg',command)

                time.sleep(2)

                try:
                    runstate() #Will fail with unexpected reset.
                except:
                    pass

            #disable register access
            regs('gcr_cpc_base',cpc_base & ~1)

@command()
def cpc_core_on(n, device=None):
    """ Request a Domain reset and power up of a cores """
    cur_core = device.core
    cores = list(OrderedDict.fromkeys([dev.core for dev in listdevices()]))
    for i,core in enumerate(cores):
        if i == n and core != cur_core:
            cpc_command_other_core(i,4)

def enablejtag():
  reset(probe)
  config('Assert nHardReset', 0)
  tapi('8 0x2f')
  config('Assert nHardReset', 1)
  reset(tap)
  autodetect()
  bkpt(sethw, 0x9d002110)
  halt(nodasm)

def main(hardReset = True, loadType = 0, showProgress = True):
    for attempt in range(10):
        try:
            load_sym = True
            load_bin = True
            if probe() is None:
                probe(args=parse_startup_args())
                for opt in sys.argv:
                    if opt.lower() == "-load_binary_only":
                        load_sym = False
                    elif opt.lower() == "-load_source_only":
                        load_bin = False

            csls = 0
            if loadType:
                load_bin = False
                load_sym = False
                if loadType & 1:
                    load_bin = True
                if loadType & 2:
                    load_sym = True
                    csls = 2
            reset(hard_halt)
            autodetect()
            if hardReset:
                reset(hard_all_run)
                time.sleep(5)
            autodetect()
            if probe().mode == "failed-autodetection":
              enablejtag()
            cmdall(project.rhalt)
            if str(runstate().status) != "stopped":
              scanonly()
              reset(tap)
              autodetect()
              continue
            # Write a stick into 0xbfc00000 so the simulator spins instead of crashing
            try:
                if isa().startswith("micromips"):
                    halfword(0xbfc00000, 0xcfff)
                    halfword(0xbfc00002, 0x0c00)
                else:
                    word(0xbfc00000, 0x1000ffff)
                    word(0xbfc00004, 0)
            except:
                pass
            # Ensure the core is on
            try:
                cpc_command(4)
            except:
                pass
            device(probe().all_socs[0].cores[0].vpes[0])
            for soc in probe().all_socs:
                for core in soc.cores:
                    try:
                        cpc_core_on(core.index)
                    except:
                        pass
            autodetect()
            for i in range(project.countcores()):
                try:
                    device(project.findcore(i)[0])
                except KeyError:
                    continue
                inittlbs()
                initcaches()
            # VPEs on
            for i in range(project.countcores()):
                try:
                    target = project.findcore(i)
                except KeyError:
                    continue
                device(target[0])
                try:
                    regs("MVPControl", 0)
                    regs("MVPControl", 2)
                    if device().probe.identifier.split(' ')[0] == 'Simulator' or device().probe.identifier.split(' ')[0] == 'RemoteImperas':
                        regs("VPEConf0", 0x800f0003 | (target[1] << 21))
                    else:
                        regs("VPEConf0", 0x800f0003)
                    regs("MVPControl", 0)
                    regs("MVPControl", 1)
                    regs("TCHalt", 0)
                    regs("TCStatus", 0x2000)
                    regs("VPEControl", regs("VPEControl") | 0x8000)
                    halt()
                    regs("TCStatus", 0)
                    regs("Config", 5)
                except:
                    pass
            device(probe().all_socs[0].cores[0].vpes[0])
        except:
            time.sleep(10)
            continue
        break
main()
