####(C)2013###############################################################
#
# Copyright (C) 2013 MIPS Tech, LLC
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
####(C)2013###############################################################
#
#   Description:	MIPS target specialisation
#
##########################################################################

define DLCOMPILER
export PATH:=$(TOOLS_DIR)/$$(TC_3)/$$(TC_VERSION)/bin/:$(PATH)
TC_TARGET=$(TOOLS_DIR)/$$(TC_3)/$$(TC_VERSION)/include/gdb/jit-reader.h
$$(TC_TARGET):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$($TC_3) $$(TC_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$$(TC_3)/$$(TC_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $$(TC_URL) -o $(TOOLS_DIR)/$$(TC_FILE)|| $(WGET) $$(TC_URL) -O $(TOOLS_DIR)/$$(TC_FILE)
	$(Q)echo $(call BANNER,EX,$$(TC_3) $$(TC_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$$(TC_FILE) $(SUP)

endef

ifdef CONFIG_ARCH_MIPS
__DO_MIPS=y
endif
ifdef CONFIG_ARCH_MIPS_MICRO
__DO_MIPS=y
endif
ifdef CONFIG_ARCH_MIPS_MICRO_R6
__DO_MIPS=y
endif
ifdef CONFIG_ARCH_MIPS_R6
__DO_MIPS=y
endif

ifdef __DO_MIPS
TC_NEED_Y = 2016
TC_NEED_M = 05
TC_NEED_R = 08
TC_VERSION = $(TC_NEED_Y).$(TC_NEED_M)-$(TC_NEED_R)
ifdef CONFIG_ARCH_MIPS_TOOLCHAIN_IMG
TC_T=IMG
TC_3=mips-img-elf
ifeq ($(HOSTARCH),i686)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Bare.Metal.CentOS-5.x86.tar.gz
endif
ifeq ($(HOSTARCH),x86_64)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Bare.Metal.CentOS-5.x86_64.tar.gz
endif
endif
ifdef CONFIG_ARCH_MIPS_TOOLCHAIN_MTI
TC_T=MTI
TC_3=mips-mti-elf
ifeq ($(HOSTARCH),i686)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Bare.Metal.CentOS-5.x86.tar.gz
endif
ifeq ($(HOSTARCH),x86_64)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Bare.Metal.CentOS-5.x86_64.tar.gz
endif
endif
ifneq ($(TC_3),)
TC_URL = http://codescape-mips-sdk.imgtec.com/components/toolchain/$(TC_VERSION)/$(TC_FILE)
TC_AVAILABLE ?= $(shell which $(TC_3)-gcc 2>/dev/null||echo false)
ifeq ($(TC_AVAILABLE),false)
$(eval $(call DLCOMPILER))
else
TC_V_STRING = $(subst -, ,$(subst ., ,$(shell $(TC_3)-gcc --version |grep -o "[0-9][0-9][0-9][0-9]\.[0-9][0-9]-[0-9][0-9]")))
TC_Y = $(firstword $(TC_V_STRING))
TC_M = $(word 2,$(TC_V_STRING))
TC_R = $(lastword $(TC_V_STRING))
ifeq ($(shell test 0$(TC_Y) -lt $(TC_NEED_Y); echo $$?),0)
$(eval $(call DLCOMPILER))
endif
ifeq ($(shell test 0$(TC_Y) -eq $(TC_NEED_Y); echo $$?),0)
ifeq ($(shell test 0$(TC_M) -lt $(TC_NEED_M); echo $$?),0)
$(eval $(call DLCOMPILER))
endif
ifeq ($(shell test 0$(TC_M) -eq $(TC_NEED_M); echo $$?),0)
ifeq ($(shell test 0$(TC_R) -lt $(TC_NEED_R); echo $$?),0)
$(eval $(call DLCOMPILER))
endif
endif
endif
endif
endif
endif

CS_AVAILABLE:=false#$(shell which Codescape-Console 2>/dev/null||echo false)
CONFIG_TOOLS_CS_SRC ?= http://mipsswrel.mipstec.com/codescape/internal/8.6.4/
CS_VERSION=8.6.4
CS_FILE=Codescape-Console-$(CS_VERSION)-linux26_$(HOSTARCH).zip
#CS_TARGETFILE := $(TOOLS_DIR)/cs/Codescape-8.6/help/_images/ItemsWaiting.png
CS_TARGETFILE := $(TOOLS_DIR)/cs/Codescape-8.6/lib/python2.7/site-packages/pexpect-4.3.0.dist-info/RECORD

$(CS_TARGETFILE):
	$(Q)echo $(call BANNER,EI,Codescape Console)
	$(Q)rm -fr $(TOOLS_DIR)/cs $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR)/cs
	$(Q)$(CURL) $(CONFIG_TOOLS_CS_SRC)$(CS_FILE) -o $(TOOLS_DIR)/cs/$(CS_FILE) || $(WGET) $(CONFIG_TOOLS_CS_SRC)$(CS_FILE) -O $(TOOLS_DIR)/cs/$(CS_FILE)
	$(Q)unzip -o -d $(TOOLS_DIR)/cs/ $(TOOLS_DIR)/cs/$(CS_FILE) $(SUP)
	$(Q)$(CURL) https://pypi.python.org/packages/ff/4e/fa4a73ccfefe2b37d7b6898329e7dbcd1ac846ba3a3b26b294a78a3eb997/ptyprocess-0.5.2-py2.py3-none-any.whl -o $(TOOLS_DIR)/cs/ptyprocess-0.5.2-py2.py3-none-any.whl || $(WGET) https://pypi.python.org/packages/ff/4e/fa4a73ccfefe2b37d7b6898329e7dbcd1ac846ba3a3b26b294a78a3eb997/ptyprocess-0.5.2-py2.py3-none-any.whl -O $(TOOLS_DIR)/cs/ptyprocess-0.5.2-py2.py3-none-any.whl
	$(Q)unzip -o -d $(TOOLS_DIR)/cs/Codescape-8.6/lib/python2.7/site-packages/ $(TOOLS_DIR)/cs/ptyprocess-0.5.2-py2.py3-none-any.whl $(SUP)
	$(Q)$(CURL) https://pypi.python.org/packages/7d/51/883102f3f288deda0c29e5177d7bdef5b9f6d60098c0d37a1f5c8f765a93/pexpect-4.3.0-py2.py3-none-any.whl -o $(TOOLS_DIR)/cs/pexpect-4.3.0-py2.py3-none-any.whl || $(WGET) https://pypi.python.org/packages/7d/51/883102f3f288deda0c29e5177d7bdef5b9f6d60098c0d37a1f5c8f765a93/pexpect-4.3.0-py2.py3-none-any.whl -O $(TOOLS_DIR)/cs/pexpect-4.3.0-py2.py3-none-any.whl
	$(Q)unzip -o -d $(TOOLS_DIR)/cs/Codescape-8.6/lib/python2.7/site-packages/ $(TOOLS_DIR)/cs/pexpect-4.3.0-py2.py3-none-any.whl $(SUP)

ifeq ($(CS_AVAILABLE),false)
MISC_PREP+=$(CS_TARGETFILE)
export PATH:=$(TOOLS_DIR)/cs/Codescape-8.6/:$(PATH)
endif

$(eval $(call DEFCONFIG,imips_prb_debug_baseline,targets/mips/imips_prb_debug_baseline.cfg))
$(eval $(call DEFCONFIG,imips_prb_debug_gic,targets/mips/imips_prb_debug_gic.cfg))
$(eval $(call DEFCONFIG,imips_prb_debug_gic_fpu,targets/mips/imips_prb_debug_gic_fpu.cfg))
$(eval $(call DEFCONFIG,imips_prb_debug_gic_msa,targets/mips/imips_prb_debug_gic_msa.cfg))
$(eval $(call DEFCONFIG,imips_prb_release_baseline,targets/mips/imips_prb_release_baseline.cfg))
$(eval $(call DEFCONFIG,imips_prb_release_gic,targets/mips/imips_prb_release_gic.cfg))
$(eval $(call DEFCONFIG,imips_prb_release_gic_fpu,targets/mips/imips_prb_release_gic_fpu.cfg))
$(eval $(call DEFCONFIG,imips_prb_release_gic_msa,targets/mips/imips_prb_release_gic_msa.cfg))
$(eval $(call DEFCONFIG,imips_sim_debug_baseline,targets/mips/imips_sim_debug_baseline.cfg))
$(eval $(call DEFCONFIG,imips_sim_debug_baseline_fpu,targets/mips/imips_sim_debug_baseline_fpu.cfg))
$(eval $(call DEFCONFIG,imips_sim_debug_baseline_msa,targets/mips/imips_sim_debug_baseline_msa.cfg))
$(eval $(call DEFCONFIG,imips_sim_debug_gic,targets/mips/imips_sim_debug_gic.cfg))
$(eval $(call DEFCONFIG,imips_sim_release_baseline,targets/mips/imips_sim_release_baseline.cfg))
$(eval $(call DEFCONFIG,imips_sim_release_baseline_fpu,targets/mips/imips_sim_release_baseline_fpu.cfg))
$(eval $(call DEFCONFIG,imips_sim_release_baseline_msa,targets/mips/imips_sim_release_baseline_msa.cfg))
$(eval $(call DEFCONFIG,imips_sim_release_gic,targets/mips/imips_sim_release_gic.cfg))

$(eval $(call DEFCONFIG,mmips_prb_debug_baseline_vz,targets/mips/mmips_prb_debug_baseline_vz.cfg))
$(eval $(call DEFCONFIG,mmips_prb_release_baseline_vz,targets/mips/mmips_prb_release_baseline_vz.cfg))
$(eval $(call DEFCONFIG,mmips_prb_debug_baseline_r6,targets/mips/mmips_prb_debug_baseline_r6.cfg))
$(eval $(call DEFCONFIG,mmips_prb_release_baseline_r6,targets/mips/mmips_prb_release_baseline_r6.cfg))

$(eval $(call DEFCONFIG,mmips_sim_debug_baseline_micro,targets/mips/mmips_sim_debug_baseline_micro.cfg))
$(eval $(call DEFCONFIG,mmips_sim_debug_baseline_vz,targets/mips/mmips_sim_debug_baseline_vz.cfg))
$(eval $(call DEFCONFIG,mmips_sim_release_baseline_micro,targets/mips/mmips_sim_release_baseline_micro.cfg))
$(eval $(call DEFCONFIG,mmips_sim_release_baseline_vz,targets/mips/mmips_sim_release_baseline_vz.cfg))

$(eval $(call DEFCONFIG,pic32_prb_debug,targets/mips/pic32_prb_debug.cfg))
$(eval $(call DEFCONFIG,pic32_prb_debug_micro,targets/mips/pic32_prb_debug_micro.cfg))
$(eval $(call DEFCONFIG,pic32_prb_release,targets/mips/pic32_prb_release.cfg))
$(eval $(call DEFCONFIG,pic32_prb_release_micro,targets/mips/pic32_prb_release_micro.cfg))

$(eval $(call DEFCONFIG,rpu_sim_debug_hwstat,targets/mips/rpu_sim_debug_hwstat.cfg))
$(eval $(call DEFCONFIG,rpu_sim_release_hwstat,targets/mips/rpu_sim_release_hwstat.cfg))


ifdef CONFIG_ARCH_MIPS

include $(MEOS_SRC)/targets/mips/baseline/module.mk
include $(MEOS_SRC)/targets/mips/gic/module.mk
include $(MEOS_SRC)/targets/mips/pic32/module.mk
include $(MEOS_SRC)/targets/mips/hwstat/module.mk
include $(MEOS_SRC)/targets/mips/regression/module.mk
include $(MEOS_SRC)/targets/mips/baseline_xpsintc/module.mk
include $(MEOS_SRC)/targets/mips/common/hal/module.mk

endif
