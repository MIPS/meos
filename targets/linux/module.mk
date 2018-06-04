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
#   Description:	Linux target specialisation
#
##########################################################################

$(eval $(call DEFCONFIG,linux_debug,targets/linux/linux_debug.cfg))
$(eval $(call DEFCONFIG,linux_release,targets/linux/linux_release.cfg))
$(eval $(call DEFCONFIG,mips_linux_debug,targets/linux/mips_linux_debug.cfg))
$(eval $(call DEFCONFIG,mips_linux_release,targets/linux/mips_linux_release.cfg))

ifeq ($(_MEOS_MAGIC_DO),CONFIG)
define VAGRANT
$(1):
	$(Q)rm -fr $(BUILD_DIR)/vagrant $(BUILD_DIR)/tests $(BUILD_DIR)/logs $(SUP)
	$(Q)mkdir -p $(BUILD_DIR)/vagrant/logs
	$(Q)mkdir -p $(BUILD_DIR)/logs
	$(Q)printf "Vagrant.configure(\"2\") do |config|\n  config.vm.box = \"$(2)\"\n  config.vm.synced_folder \"$(WORK_DIR)\", \"/meos\"\n  config.vm.synced_folder \"$(BUILD_DIR)/vagrant\", \"/build\"\n$(3)\nend" > $(BUILD_DIR)/vagrant/Vagrantfile
	$(Q)cd $(BUILD_DIR)/vagrant/;vagrant up
	$(Q)cd $(BUILD_DIR)/vagrant/;vagrant ssh -c "$(4); make -C /meos BUILD_DIR=/build linux_release_test" $(if $(UNILOG),> $(UNILOG),) || (vagrant destroy -f; rm -fr $(BUILD_DIR)/vagrant; false;)
	$(Q)cd $(BUILD_DIR)/vagrant/;vagrant destroy -f
	$(Q)cp -R $(BUILD_DIR)/vagrant/logs/* $(BUILD_DIR)/logs/
	$(Q)rm -fr $(BUILD_DIR)/vagrant $(SUP)
endef
else
define VAGRANT
$(1):
	@:
endef
endif

$(eval $(call VAGRANT,centos6_vagrant,bento/centos-6.7,,true))
$(eval $(call VAGRANT,centos7_vagrant,bento/centos-7.2,config.vm.box_version = \"2.2.9\",true))
$(eval $(call VAGRANT,fedora18_vagrant,rafacas/fedora18-plain,,true))
$(eval $(call VAGRANT,fedora19_vagrant,rafacas/fedora19-plain,,true))
$(eval $(call VAGRANT,fedora20_vagrant,rafacas/fedora20-plain,,true))
$(eval $(call VAGRANT,fedora21_vagrant,rafacas/fedora21-plain,,true))
$(eval $(call VAGRANT,fedora22_vagrant,bento/fedora-22,,true))
$(eval $(call VAGRANT,fedora23_vagrant,bento/fedora-23,config.vm.box_version = \"2.2.9\",true))
$(eval $(call VAGRANT,fedora24_vagrant,bento/fedora-24,config.vm.box_version = \"2.2.9\",true))
$(eval $(call VAGRANT,ubuntu1604_vagrant,bento/ubuntu-16.04,config.vm.box_version = \"2.2.9\",sudo apt-get update;sudo apt-get install make))
$(eval $(call VAGRANT,debian85_vagrant,bento/debian-8.5,,sudo apt-get update))

ifdef CONFIG_ARCH_LINUX

TC_NEED_Y = 2016
TC_NEED_M = 05
TC_NEED_R = 03
TC_VERSION = $(TC_NEED_Y).$(TC_NEED_M)-$(TC_NEED_R)
ifdef CONFIG_ARCH_LINUX_MIPS_IMG
TC_T=IMG
TC_3=mips-img-linux-gnu
ifeq ($(HOSTARCH),i686)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Linux.CentOS-5.x86.tar.gz
endif
ifeq ($(HOSTARCH),x86_64)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Linux.CentOS-5.x86_64.tar.gz
endif
endif
ifdef CONFIG_ARCH_LINUX_MIPS
TC_T=MTI
TC_3=mips-mti-linux-gnu
ifeq ($(HOSTARCH),i686)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Linux.CentOS-5.x86.tar.gz
endif
ifeq ($(HOSTARCH),x86_64)
TC_FILE = Codescape.GNU.Tools.Package.$(TC_VERSION).for.MIPS.$(TC_T).Linux.CentOS-5.x86_64.tar.gz
endif
endif
ifneq ($(TC_3),)
TC_URL = http://codescape-mips-sdk.imgtec.com/components/toolchain/$(TC_VERSION)/$(TC_FILE)
TC_AVAILABLE ?= $(shell which $(TC_3)-gcc 2>/dev/null||echo false)
ifeq ($(TC_AVAILABLE),false)
export PATH:=$(PATH):$(TOOLS_DIR)/$(TC_3)/$(TC_VERSION)/bin/
TC_TARGET=$(TOOLS_DIR)/$(TC_3)/$(TC_VERSION)/include/gdb/jit-reader.h
$(TC_TARGET):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(TC_3) $(TC_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(TC_3)/$(TC_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(TC_URL) -o $(TOOLS_DIR)/$(TC_FILE)|| $(WGET) $(TC_URL) -O $(TOOLS_DIR)/$(TC_FILE)
	$(Q)echo $(call BANNER,EX,$(TC_3) $(TC_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(TC_FILE) $(SUP)
else
TC_V_STRING = $(subst -, ,$(subst ., ,$(shell $(TC_3)-gcc --version |grep -o "[0-9][0-9][0-9][0-9]\.[0-9][0-9]-[0-9][0-9]")))
TC_Y = $(firstword $(TC_V_STRING))
TC_M = $(word 2,$(TC_V_STRING))
TC_R = $(lastword $(TC_V_STRING))
ifeq ($(shell test 0$(TC_Y) -lt $(TC_NEED_Y); echo $$?),0)
$(error $(TC_3) toolchain too old! Please use $(TC_VERSION) or newer: http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/available-releases/)
endif
ifeq ($(shell test 0$(TC_Y) -eq $(TC_NEED_Y); echo $$?),0)
ifeq ($(shell test 0$(TC_M) -lt $(TC_NEED_M); echo $$?),0)
$(error $(TC_3) toolchain too old! Please use $(TC_VERSION) or newer: http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/available-releases/)
endif
ifeq ($(shell test 0$(TC_M) -eq $(TC_NEED_M); echo $$?),0)
ifeq ($(shell test 0$(TC_R) -lt $(TC_NEED_R); echo $$?),0)
$(error $(TC_3) toolchain too old! Please use $(TC_VERSION) or newer: http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/available-releases/)
endif
endif
endif
endif
endif

$(ABUILD_DIR)/include/meos/%.h: targets/linux/%.h $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$<)
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

HEADERS += \
	target/ctx.h \
	target/dbg.h \
	target/irq1.h \
	target/irq2.h \
	target/krn.h \
	target/mem.h \
	target/tmr.h

#
# Build setup
#

# Always need these
PRIVATE_CFLAGS := -g$(CONFIG_BUILD_G) -O$(CONFIG_BUILD_O) -Wall
CFLAGS := -pthread $(CFLAGS) $(patsubst "%,%,$(patsubst %",%,$(strip $(CONFIG_EXTRA_FLAGS))))
LFLAGS += -pthread $(patsubst "%,%,$(patsubst %",%,$(strip $(CONFIG_EXTRA_FLAGS))))

ifdef CONFIG_DEBUG_PARANOIA
ifdef CONFIG_ARCH_LINUX_X86
CFLAGS := -fno-omit-frame-pointer $(CFLAGS)
endif
ifdef CONFIG_ARCH_LINUX_X64
CFLAGS :=-fno-omit-frame-pointer $(CFLAGS)
endif
endif

CTX_SRC += targets/linux/ctx.c
IRQ_SRC += targets/linux/irq.c
TMR_SRC += targets/linux/tmr.c
DBG_SRC += targets/linux/dbg.c
IPM_SRC += targets/linux/ipm.c targets/linux/ipmw.c
MEM_SRC += targets/linux/mem.c

SRC += \
	targets/linux/ctx.c \
	targets/linux/dbg.c \
	targets/linux/irq.c \
	targets/linux/mem.c \
	targets/linux/tmr.c \

ifdef CONFIG_FEATURE_IMPEXP
SRC += \
	targets/linux/ipm.c \
	targets/linux/ipmw.c
endif

ASMSRC += \

TARGET_NAME := linux

endif
