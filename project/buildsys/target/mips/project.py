from imgtec.codescape import acquirer
from imgtec.console import *
import imgtec.console
import imgtec.codescape
from imgtec.console.support import *
from imgtec.codescape.tiny import State
from imgtec.codescape.tiny import GetCommsOptions
import getopt
import sys
import time
import runpy
import atexit

cores = 0
coremap = None
warp = False
pprobe = None
pemu = None
pacq = None
rst = None
acq = None
ldr = None

def _devind(device):
	try:
		return regs("EBase", device()).CPUNum
	except:
		return 0

class FakeFile(object):

	def __init__(self, data = ""):
		self.data = data

	def read(self, ln, ptr):
		idata = self.data + (chr(0) * (ptr + ln))
		dln = len(self.data)
		return idata[ptr : ptr + ln]

	def write(self, buf, ln, ptr):
		idata = self.data + (chr(0) * max(0, ptr + ln - len(self.data)))
		buf += (chr(0) * max(0, (ln - len(buf))))
		self.data = idata[:ptr] + buf[: ln] + idata[ptr + ln:]

class OutFile(object):

	def __init__(self):
		self.data = ""

	def read(self, ln, ptr):
		return ""

	def write(self, buf, ln, ptr):
		sys.stdout.write(buf[: max(len(buf), ln - 1)])

class FakeHandle(object):

	def __init__(self, file):
		self.file = file
		self.ptr = 0

	def read(self, ln):
		p = self.ptr
		self.ptr += ln
		d = self.file.read(ln, p)
		return d

	def write(self, buf, ln):
		p = self.ptr
		self.ptr += ln
		self.file.write(buf, ln, p)

	def lseek(self, whence, offset):
		if whence == 1:
			self.ptr = offset
		if whence == 2:
			self.ptr += offset
		if whence == 4:
			self.ptr = len(self.file.data) + offset

files = {"/dev/stdin":FakeFile(), "/dev/stdout":OutFile(), "/dev/stderr":OutFile()}
handles = {0: FakeHandle(files["/dev/stdin"]), 1: FakeHandle(files["/dev/stdout"]), 2: FakeHandle(files["/dev/stderr"])}
nexthandle = 3
argv = None

def setargs(args):
	argv = args

def setfile(name, contents):
	files[name] = FakeFile(contents)

def getfile(name):
	return files[name].data

@command()
def runstate(on_halt_callbacks=True, devices=[]):
		r"""Get the running/halted state of the specified device(s).

		This command returns an instance of :class:`Runstate` which can be queried
		for various states::

				state = runstate()
				if state.is_running:
						print 'Target is running'
				else:
						print 'Target is %s at pc=%r' % (state.status, state.pc)

		If `on_halt_callbacks` is True and the target is newly halted then all
		registered :func:`onhaltcallback`\s will be called and their results
		displayed.

		This has been enhanced to service UHI calls.
		"""

		def _call_on_halt_callbacks(device):
				cbs = sorted(imgtec.console.on_halt_callbacks.items())
				onhaltdata = [(name, cb(device)) for name, cb in cbs]
				onhaltdata = [(name, x) for name, x in onhaltdata if x is not None]

				return onhaltdata


		class UHI:
				exit = 1
				open = 2
				close = 3
				read = 4
				write = 5
				lseek = 6
				unlink = 7
				fstat = 8
				argc = 9
				argnlen = 10
				argn = 11
				heapinfo = 12
				plog = 13
				assrt = 14
				findfirst = 16
				findnext = 17
				findclose = 18
				pread = 19
				pwrite = 20
				yld = 21
				link = 22

		def readCstr(device, i):
			s = ""
			ch = byte(i, device=device)
			while ch:
				s = s + chr(ch)
				i = i + 1
				ch = byte(i, device=device)
			return s

		def _tryuhi(device, state):
			tcactive((regs("TCBind", device=device) & 0x1fe00000) >> 21, device=device)
			before = 0
			config = regs("Config", device=device)
			r6 = True if (config & 0x1c00) == 0x800 else False
			sixteene = True if (config & 0x80000000) == 0x80000000 and (regs("Config1", device=device) & 4) == 4 else False
			nanomips = True if (config & 0x80000000) == 0x80000000 and (regs("Config1", device=device) & 0x80000000) == 0x80000000 and (regs("Config2", device=device) & 0x80000000) == 0x80000000 and ((regs("Config3", device=device) & 0x0001c0000) >> 18) == 3 else False
			pc = regs("pc", device=device)
			code = 0
			sdbbp = False
			size = 4
			if nanomips:
				size = 2
				inst = halfword(pc, device=device)
				if inst & 0xfff8 == 0x1018:
					code = (inst & 0x0007)
					sdbbp = True
				else:
					inst = word(pc, device=device)
					if inst & 0xfff80000 == 0x00180000:
						code = (inst & 0x0007ffff)
						sdbbp = True
						size = 4
			else:
				if pc & 1 == 1:
					size = 2
					if sixteene:
						inst = halfword(pc & ~1, device=device)
						if inst & 0xf81f == 0xe801:
							code = (inst & 0x07e0) >> 5
							sdbbp = True
					else:
						inst = halfword(pc & ~1, device=device)
						if r6:
							if inst & 0xfc3f == 0x443b:
								code = (inst & 0x03c0) >> 6
								sdbbp = True
							else:
								inst = word(pc & ~1, device=device)
								if inst & 0xfc00ffff == 0x0000db7c:
									code = (inst & 0x03ff0000) >> 16
									sdbbp = True
									size = 4
						else:
							if inst & 0xfff0 == 0x46c0:
								code = (inst & 0x000f)
								sdbbp = True
							else:
								inst = word(pc, device=device)
								if inst & 0xfc00ffff == 0x0000db7c:
									code = (inst & 0x03ff0000) >> 16
									sdbbp = True
									size = 4
				else:
					inst = word(pc, device=device)
					if r6 and (inst & 0xfc00003f) == 0x0000000e:
						code = (inst & 0x03ffffc0) >> 6
						sdbbp = True
					elif not r6 and (inst & 0xfc00003f) == 0x7000003f:
						code = (inst & 0x03ffffc0) >> 6
						sdbbp = True
			if str(state.status) == 'sw_break' and sdbbp == True and code == 1 and regs("r2", device=device) == 1:
				op = regs("r25", device=device)
				args = regs('r4 r5 r6 r7', device=device)
				result = 0
				errno = 0
				if op == UHI.exit:
					# Need to make the thread runnable for the simulator to work
					regs("pc", 0xbfc00000, device=device) # Stick it
					tcactive(-1, device=device)
					return State('stopped', pc, 0)
				elif op == UHI.open:
					fn = readCstr(device, args[0])
					if not fn in files:
						fn = str(_devind(device))+":"+fn
						if not fn in files:
							files[fn] = FakeFile()
						fh = FakeHandle(files[fn])
					else:
						fh = FakeHandle(files[fn])
					global nexthandle
					handles[nexthandle] = fh
					result = nexthandle
					nexthandle += 1
				elif op == UHI.close:
					if args[0] in handles:
						if args[0] > 2:
							handles[args[0]] = None
						result = 0
					else:
						result = -1
						errno = 22
				elif op == UHI.read:
					if args[0] in handles:
						h = handles[args[0]]
						s = h.read(args[2])
						byte(address=args[1], values = [ord(c) for c in s], count = min(len(s),args[2]), device=device)
						result = min(len(s),args[2])
					else:
						result = -1
						errno = 22
				elif op == UHI.write:
						writedata = "".join([chr(c) for c in byte(address=args[1], count=args[2], device=device)])
						if args[0] in handles:
							h = handles[args[0]]
							h.write(writedata, args[2])
							result = args[2]
						else:
							result = -1
							errno = 22
				elif op == UHI.lseek:
					if args[0] in handles:
						h = handles[args[0]]
						h.lseek(args[1], args[2])
					else:
						result = -1
						errno = 22
				elif op == UHI.unlink:
					# UHI unlink unsupported
					result = -1
					errno = 22
				elif op == UHI.fstat:
					if args[0] > 2:
						# UHI fstat unsupported
						result = -1
						errno = 22
				elif op == UHI.argc:
					if args is None:
						result = 0
					else:
						result = len(argv)
				elif op == UHI.argnlen:
					if args is None:
						result = 0
					else:
						result = len(argv[args[0]])
				elif op == UHI.argn:
					if args is not none:
						byte(address=args[1], values = [ord(c) for c in argv[args[0]]], count = len(argv[args[0]]), device=device)
					result = 0
				elif op == UHI.heapinfo:
					# UHI heapinfo unsupported
					result = -1
					errno = 22
				elif op == UHI.plog:
					s = readCstr(device, args[0])
					s = s % (args[1])
					result = len(s)
					sys.stdout.write(s)
				elif op == UHI.assrt:
					msg = readCstr(device, args[0])
					fn = readCstr(device, args[1])
					print("UHI assert '%s' %s:%d" % (msg, fn, args[2]))
					# Need to make the thread runnable for the simulator to work
					regs("pc", 0xbfc00000, device=device) # Stick it
					tcactive(-1, device=device)
					return State('stopped', pc, 0)
				elif op == UHI.findfirst:
					# UHI findfirst unsupported
					result = -1
					errno = 22
				elif op == UHI.findnext:
					# UHI findnext unsupported
					result = -1
					errno = 22
				elif op == UHI.findclose:
					# UHI findclose unsupported
					result = -1
					errno = 22
				elif op == UHI.pread:
					# UHI pread unsupported
					result = -1
					errno = 22
				elif op == UHI.pwrite:
					# UHI pwrite unsupported
					result = -1
					errno = 22
				elif op == UHI.yld:
					# UHI yield unsupported
					result = -1
					errno = 22
				elif op == UHI.link:
					# UHI link unsupported
					result = -1
					errno = 22
				regs("pc", regs("pc", device=device) + size, device=device)
				regs("r2", result, device=device)
				regs("r3", errno, device=device)
				after = regs("r8", device=device)
				go(quiet, device=device)
				tcactive(-1, device=device)
				return State('running', pc, 0)
			tcactive(-1, device=device)
			return state

		def _runstate(device):
				tcactive(-1, device)
				state = device.tiny.GetState()
				cbs = []
				was_running, device._was_running_last_time = device._was_running_last_time, state.is_running
				if str(state.status) == 'sw_break':
						state = _tryuhi(device, state)
				if not state.is_running and was_running and on_halt_callbacks:
						cbs = _call_on_halt_callbacks(device)
				return Runstate(state, cbs)

		res = imgtec.console.AllResult()
		res.call(_runstate, devices)
		return res.get_result_maybe_just_one()

@command(any_or_all=[(any, any), (all, all)])
def waitforhalt(timeout=0, any_or_all=all, devices=[],):
		'''Waits until the device is stopped

		Can take timeout and device arguments.
		If get timeout argument then raise RunTimeError if not stopped until this time.
		There is no timeout by default.

		This has been enhanced to understand that a VPE with no active or unhalted TCs
		is effectively halted. It also uses project.runstate(), resulting in UHI calls
		being serviced.
		'''
		def _halted(d, chktc):
			if (chktc):
				# does it have tcs?
				tcs = tclist(d)
				tcl = len(tcs)
				tcf = [True for i in tcs if i.tcbind.CurVPE == d.index and i.vpeconf0.VPA == 1 and i.tcstatus.A == 1 and i.tchalt.H == 0] if tcl > 0 else None
				tcactive(-1)
				if tcl > 0 and len(tcf) == 0:
					return True
			r = runstate(d)
			if str(r.status) == "halted_by_probe":
				go(quiet, d)
				return False
			return not r.is_running

		target = time.time() + timeout
		i = 0
		devices = [d for d in devices if str(d.family) != 'mipscm']
		r = [_halted(d, False) for d in devices]
		while not any_or_all(r):
			i += 1
			if timeout and time.time() > target:
					raise RuntimeError('Target has not stopped after {0} seconds.'.format(timeout))
			time.sleep(0.1)
			r = [_halted(d, i % 10 == 0) for d in devices]

def emulator(target):
	'''Try to find the emulator that most closely matches the provided string,
	and configure it such that it will successfully run a multithreaded test.
	'''
	simname = str(simulators(target)[0])
	sim = GetCommsOptions(simname)['sim']
	probe(simname, advanced_options={'sim':sim + " MTFPU=2", 'disconnect-latency': 30})
	time.sleep(0.1)
	try:
		if isa().startswith("+mt+dsp"):
				halfword(0xbfc00000, 0x1bff)
		if isa().startswith("micromips"):
				halfword(0xbfc00000, 0xcfff)
				halfword(0xbfc00002, 0x0c00)
		else:
				word(0xbfc00000, 0x1000ffff)
				word(0xbfc00004, 0)
	except:
			pass

#def acquire(identifier, timeout=0):
#	'''Try to acquire a target that matches the provided string, and ensure it is
#	automatically released upon completion.
#	'''
#
#	acq = None
#
#	def _timeoutacq(fn, target):
#		timedout = time.time() + timeout
#		r = fn(target)
#		while r is None:
#			if timeout and time.time() > target:
#				raise RuntimeError('Could not acquire target after {0} seconds.'.format(timeout))
#			time.sleep(10)
#			r = fn(target)
#		return r
#
#	for target in acquirer.list_of_targets(bitstreams=True):
#		if identifier in target[1]:
#			acq = _timeoutacq(acquirer.acquire_bitstream, target[1])
#
#	if acq is None:
#		acq = _timeoutacq(acquirer.acquire, identifier)
#
#	probe(acq[0])
#
#	atexit.register(stop)

# START PALEO HACK
import os, pexpect
import shlex
from subprocess import Popen, PIPE
primary = None

def paleoacquire(Registry, FPGAClass, BitStream, Version = None, timeout = 3600):
	"""
	You should override this method when you subclass Paleo. It will be called after the process has been
	Daemonized by start() or restart().
	"""
	timeout += time.time()
	global primary
	if Version == "":
		Version = None
	first = True
	while True:
		primary = pexpect.spawn('conmux-console %s/%s' % (Registry, FPGAClass))
		index = primary.expect(["Connected to ([\w-]*) .*", "console: .*", "Conmux::Registry.*", "Conmux::connect.*", pexpect.EOF, pexpect.TIMEOUT])
		if index == 0 or time.time() > timeout:
			break
		if first:
			first = False
			sys.stderr.write("Waiting to allocate fpga\n")
		primary.terminate()
	if index != 0:
		sys.stderr.write("Failed to allocate fpga\n")
		sys.stderr.write(str(primary))
		primary.terminate()
		process = Popen(shlex.split("conmux-console --list %s" % Registry), stdout=PIPE)
		process.communicate()
		exit_code = process.wait()
		sys.exit(2)
	fpga = primary.match.group(1)
	sys.stderr.write("Connected to fpga %s\n" % fpga)
	secondary = pexpect.spawn("conmux-console --list %s" % Registry)
	index = secondary.expect(['.*%s[^\n]* "(sp[0-9]*)' % fpga, pexpect.EOF, pexpect.TIMEOUT])
	if index != 0:
		sys.stderr.write("Failed to find probe\n")
		sys.stderr.write(str(secondary))
		primary.terminate()
		secondary.terminate()
		sys.exit(2)
	DA = secondary.match.group(1)
	secondary.terminate()
	sys.stderr.write("Found probe %s\n" % (DA))
	primary.sendline("~$lrb")
	index = primary.expect([".* SN=(\w*)", pexpect.EOF, pexpect.TIMEOUT])
	if index != 0:
		sys.stderr.write("Failed to find SN\n")
		sys.stderr.write(str(primary))
		primary.terminate()
		sys.exit(2)
	SN = primary.match.group(1)
	sys.stderr.write("Running SN %s\n" % (SN))
	index = primary.expect(["VER: CPU=(\w*)", pexpect.EOF, pexpect.TIMEOUT])
	if index != 0:
		sys.stderr.write("Failed to find VER\n")
		sys.stderr.write(str(primary))
		primary.terminate()
		sys.exit(2)
	VER = primary.match.group(1)
	sys.stderr.write("Running VER %s\n" % (VER))
	if (Version is not None and Version != VER) or (Version is None and SN != BitStream):
		sys.stderr.write("Flashing fpga to %s\n" % (BitStream))
		primary.sendline("~$flash " + BitStream)
		index = primary.expect(["SUCCESS", "ERROR", pexpect.EOF, pexpect.TIMEOUT], timeout = 1200)
		if index != 0:
			sys.stderr.write("Failed to flash fpga\n")
			sys.stderr.write(str(primary))
			primary.terminate()
			sys.exit(2)
		time.sleep(5)
	logging(comms,on)
	try:
		probe(DA)
	except RuntimeError:
		probe(DA)
		print(logfile(comms))
	logging(comms,off)
	reset(probe)
	scanonly()
	reset(tap)
	autodetect()
	atexit.register(paleorelease)

def paleorelease():
	try:
		primary.sendline("~$quit")
		time.sleep(1)
		primary.terminate()
	except:
		pass

# END PALEO HACK

def start():
	'''Prepare the test suite: request a probe.
	'''
	if pprobe is not None:
		probe(pprobe)
	if pemu is not None:
		emulator(pemu)
	if pacq is not None:
		args = pacq.split("/")
		paleoacquire("le-fw-console.mipstec.com", args[0], args[1], args[2] if len(args) == 3 else None)

def stop():
	'''Shut down the test suite: release the probe.
	'''
	try:
		closeprobe()
	except:
		pass
	if acq is not None:
		try:
			acquirer.release(acq)
		except:
			pass

def countcores():
	'''Return the number of cores available for loading ELF files onto.
	'''
	global cores, coremap
	if coremap is None:
		coremap = dict()
		for soc in probe().all_socs:
			for core in soc.cores:
				for thread in core.vpes:
					if str(runstate(device=thread).status) != "in_reset":
						if len(tclist(device=core)) == 0:
							if str(thread.abi) == "o32" or str(thread.abi) == "p32":
								try:
									coremap[regs("Ebase", device = thread) & 0x3ff] = (thread, (soc.index, core.index, 0))
								except:
									return cores
								cores += 1
						else:
							tcactive(1, device=core.vpes[0])
							before = regs("TCBind", device=core.vpes[0]) & 0xf
							tcactive(-1, device=core.vpes[0])
							if str(thread.abi) == "o32" or str(thread.abi) == "p32":
								bindtc(1, thread.index, core)
								coremap[regs("Ebase", device = thread) & 0x3ff] = (thread, (soc.index, core.index, thread.index))
								bindtc(1, before, core)
								cores += 1
	return cores

def findcore(i):
	'''Return the nth core within the system, regardless of which hardware it
	resides upon.
	'''
	global coremap
	if coremap is None:
		countcores()
	return coremap[i]

def rhalt(device):
	if str(runstate(device=device).status) != "in_reset":
		halt(nodasm, device=device)

def bindtc(tc, vpe, device):
	'''Bind a TC to the current device. Trashes 0xa0000004.'''
	if isa().startswith("+mt+dsp"):
		halfword(0xa0000004, 0x1bff, device=device)
	else:
		word(0xa0000004, values=[0x7000003f], device=device)
	if tclist(device=device)[tc].tcbind.CurVPE != vpe:
		tcactive(tc, device=device)
		regs("TCHalt", 1, device=device)
		regs("TCStatus", regs("TCStatus") &~ 0x2000, device=device)
		tcactive(-1, device=device)
		before = device.tiny.GetThread()
		device.tiny.SetThread(vpe)
		device.tiny.BindTC(tc, 0xa0000004)
		device.tiny.SetThread(before)
	else:
		tcactive(tc, device=device)
		regs("TCHalt", 1, device=device)
		regs("TCStatus", regs("TCStatus") &~ 0x2000, device=device)
		tcactive(-1, device=device)
	cmdall(rhalt, devices=[device])

def smp():
	'''Configure a "pleasant" initial state for an SMP system using bareuseful.
	'''
	if len(tclist()) == 0:
		return
	# Might be self hosted SMP - configure appropriately
	regs("MVPControl", 0)
	# Split TCs between VPEs
	device(probe().all_socs[0].cores[0].vpes[0])
	vpes = regs("MVPConf0").PVPE + 1
	tcs = regs("MVPConf0").PTC + 1
	for i in range(1, tcs):
		vpe = int((i * vpes) / tcs)
		if vpe >= vpes:
			vpe = vpes - 1
		bindtc(i, vpe, device())
	# Ensure it's clean for bareuseful
	orig = device()
	for soc in probe().all_socs:
			for core in soc.cores:
					device(core)
					if len(tclist()) > 0:
						for tc in range(len(tclist())):
							tcactive(tc)
							regs("TCHalt", 1)
							regs("TCStatus", 0)
						tcactive(-1)
						go(quiet)
	device(orig)
	tcactive(0)
	halt(nodasm)
	regs("TCStatus", 0x2000)
	regs("TCHalt", 0)
	tcactive(-1)
	cmdall(rhalt)

def loadelfs(threads, elfs):
	'''Load the specified elf files onto an appropriate number of threads, in a
	manner compatible with both Codescape and Codescape-Console. Additionally
	obeys UHI start up ABI.
	'''
	# Load code
	targets = []
	before = probe().identifier
	for i in range(threads):
		target = findcore(i)
		targets.append(target[0])
		device(target[0])
		tcactive(-1, device = target[0])
		imgtec.console.load(elfs[i], load_symbols=True, load_binary=True, device = target[0])
		regs("Config", (regs("Config") & ~7) | 5, device = target[0])
		regs("a0", 0, device = target[0])
		regs("a1", 0, device = target[0])
		regs("a2", 0, device = target[0])
		regs("a3", 0, device = target[0])
		regs("Status", 0, device = target[0])
		regs("Cause", 0, device = target[0])
		regs("Count", 0, device = target[0])
		regs("TCStatus", 0x2000, device = target[0])
		tcactive(-1, device = target[0])
	if imgtec.codescape.environment == "codescape":
		closeprobe()
		cs = imgtec.codescape.ConnectProbe(before)
		for i in range(threads):
			target = findcore(i)
			cst = cs.socs[target[1][0]].cores[target[1][1]].hwthreads[target[1][2]] if imgtec.codescape.environment == "codescape" else None
			cst.LoadProgramFile(elfs[i], hard_reset=False, progress=True, load_type=imgtec.codescape.da_types.LoadType.symbols|0x400)
	return targets

def load(rsti = None, ldri = None, interstitial = None):
	'''Reset the attach system, and load the provided application.

	By default, the reset and load script will be parsed from the command line,
	but may instead be overridden with rsti and ldri.

	Additionally, a function - interstitial - may be passed which will be executed
	between reset and load. This allows a test to set up TLBs, etc. required for
	load.

	A reset script should be a plain script that resets the attach system to the
	required state.

	A load script should declare two globals:
	1) "threads", an integer specifying the number of threads to be loaded.
	2) "apps", an array of strings, containing the ELF files for each thread.
	'''
	# Reset stdio
	global files
	global handles
	global nexthandle
	files = {"/dev/stdin":FakeFile(), "/dev/stdout":OutFile(), "/dev/stderr":OutFile()}
	handles = {0: FakeHandle(files["/dev/stdin"]), 1: FakeHandle(files["/dev/stdout"]), 2: FakeHandle(files["/dev/stderr"])}
	nexthandle = 3
	# Run reset script
	global rst, ldr, cores
	if rsti is not None:
		rst = rsti
	if ldri is not None:
		ldr = ldri
	if probe() is None:
		probe(args=parse_startup_args())
		for opt in sys.argv:
			if opt.lower() == "-load_binary_only":
				load_sym = False
			elif opt.lower() == "-load_source_only":
				load_bin = False

	if rst is not None:
		runpy.run_path(rst)

	countcores()
	if interstitial is not None:
		interstitial()
	if ldr is not None:
		res = runpy.run_path(ldr)
		if res["threads"] == 1:
			smp()
		res["targets"] = loadelfs(res["threads"], res["apps"])
		return res

def main():
	'''Initialises the test system, adding extra parameters allowing probe
	configuration to be controlled by the project build system.

	--probe=X:	Specify a probe to use.
	--emulator=X:	Specify an emulator to instantiate.
	--acquire=X:	Specify an acquisition specifier to acquire.
	--reset=X:	Specify a reset script.
	--load=X:	Specify a load script.
	--timeout=X:	Specify a timeout.
	--timewarp:	Engage timewarp mode under emulation. Causes CP0.Count to skip to CP0.Compare on wait.

	See imgtec.test.main for further details and parameters.
	'''
	global pprobe, pemu, warp, pacq, rst, ldr, timeout
	parser = test.get_argument_parser()
	parser.add_argument('--probe', help='Specify a probe to use')
	parser.add_argument('--emulator', help='Specify an emulator to instantiate')
	parser.add_argument('--acquire', help='Specify an acquisition specifier to acquire')
	parser.add_argument('--reset', help='Specify a reset script')
	parser.add_argument('--load', help='Specify a load script')
	parser.add_argument('--timeout', help='Specify a timeout')
	parser.add_argument('--timewarp', help='Engage timewarp mode under emulation')
	args = parser.parse_args()
	vargs = vars(args)
	if "probe" in vargs:
		pprobe = vargs["probe"]
	if "emulator" in vargs:
		pemu = vargs["emulator"]
	if "timewarp" in vargs:
		timewarp = True
	if "acquire" in vargs:
		pacq = vargs["acquire"]
	if "reset" in vargs:
		rst = vargs["reset"]
	if "load" in vargs:
		ldr = vargs["load"]
	if "timeout" in vargs:
		timeout = vargs["timeout"]
	return test.main(parsed_args=args)
