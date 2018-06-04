####(C)2015###############################################################
#
# Copyright (C) 2015 MIPS Tech, LLC
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
####(C)2015###############################################################
#
#   Description:	Automated distribution library build rules
#
##########################################################################

SUPPORT_DIR = $(ABUILD_DIR)/support
CURL ?= $(shell which curl 2>/dev/null||echo false)
WGET ?= $(shell which wget 2>/dev/null||echo false)

ifeq (false,$(CURL))
ifeq (false,$(WGET))
	$(error Install curl or wget!)
endif
endif

NEWTRON_VERSION = 8cf8f54dcf4cb2dc67ccd9353af464446d27b86e
NEWTRON_FILE = $(NEWTRON_VERSION).zip
CONFIG_NEWTRON_SRC ?= https://github.com/apache/incubator-mynewt-core/archive/
NEWTRON_URL = $(CONFIG_NEWTRON_SRC)$(NEWTRON_FILE)
NEWTRON_EXISTFILE := $(SUPPORT_DIR)/incubator-mynewt-core-$(NEWTRON_VERSION)/RELEASE_NOTES.md
$(NEWTRON_EXISTFILE):
	$(Q)mkdir -p $(SUPPORT_DIR)
	$(Q)echo $(call BANNER,DL,$(NEWTRON_VERSION))
	$(Q)rm -fr $(SUPPORT_DIR)/$(NEWTRON_VERSION) $(SUP)
	$(Q)mkdir -p $(SUPPORT_DIR) $(SUP)
	$(Q)$(CURL) -L $(NEWTRON_URL) -o $(SUPPORT_DIR)/$(NEWTRON_FILE) || $(WGET) $(NEWTRON_URL) -O $(SUPPORT_DIR)/$(NEWTRON_FILE)
	$(Q)echo $(call BANNER,EX,$(NEWTRON_VERSION))
	$(Q)unzip -d $(SUPPORT_DIR) $(SUPPORT_DIR)/$(NEWTRON_FILE) $(SUP)
	$(Q)cd $(SUPPORT_DIR)/incubator-mynewt-core-$(NEWTRON_VERSION);patch -p1  < $(WORK_DIR)/support/3/newtron/newtron.patch $(SUP)
	#$(Q)cp -R support/3/newtron/* $(SUPPORT_DIR)/incubator-mynewt-core-$(NEWTRON_VERSION)/

NEWTRON_DIR = $(SUPPORT_DIR)/incubator-mynewt-core-$(NEWTRON_VERSION)
NEWTRON_ARCH = $(WORK_DIR)/support/3/newtron
NEWTRON_SRC += \
	$(NEWTRON_DIR)/hw/hal/src/hal_flash.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_area.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_block.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_cache.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_config.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_crc.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_dir.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_file.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_flash.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_format.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_gc.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_hash.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_inode.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_misc.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_path.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_restore.c \
	$(NEWTRON_DIR)/fs/nffs/src/nffs_write.c \
	$(NEWTRON_DIR)/fs/fs/src/fs_dirent.c \
	$(NEWTRON_DIR)/fs/fs/src/fs_file.c \
	$(NEWTRON_DIR)/fs/fs/src/fs_mkdir.c \
	$(NEWTRON_DIR)/fs/fs/src/fs_mount.c \
	$(NEWTRON_DIR)/fs/fs/src/fsutil.c \
	$(NEWTRON_DIR)/kernel/os/src/os_mempool.c \
	$(NEWTRON_DIR)/sys/stats/src/stats.c \
	$(NEWTRON_DIR)/sysflash/sysflash.c \
	$(NEWTRON_DIR)/util/crc/src/crc16.c \

NEWTRON_OBJ:=$(patsubst $(NEWTRON_DIR)/%.c,$(ABUILD_DIR)/obj/newtron/%.o,$(filter %.c,$(NEWTRON_SRC)))

# NEWTRON_ARCH_SRC: Architecture specific files
NEWTRON_ARCH_SRC = \
	$(NEWTRON_ARCH)/newtron.c

NEWTRON_ARCH_OBJ:=$(patsubst $(NEWTRON_ARCH)/%.c,$(ABUILD_DIR)/obj/newtron/%.o,$(filter %.c,$(NEWTRON_ARCH_SRC)))

NEWTRON_LIBRARY_SHORT = $(ABUILD_DIR)/lib/libnewtron.a
NEWTRON_LIBRARY = $(NEWTRON_LIBRARY_SHORT)($(NEWTRON_OBJ) $(NEWTRON_ARCH_OBJ))

#INC +=  -I $(NEWTRON_DIR)/hw/bsp/include \
	#-I $(NEWTRON_DIR)/hw/hal/include \
	#-I $(NEWTRON_DIR)/hw/bsp/meos/include \
	#-I $(NEWTRON_DIR)/fs/nffs/include \
	#-I $(NEWTRON_DIR)/libs/util/include \
	#-I $(NEWTRON_DIR)/fs/fs/include \
	#-I $(NEWTRON_DIR)/libs/os/include \
	#-I $(NEWTRON_DIR)/libs/testutil/include \
	#-I $(NEWTRON_DIR)/


NEWTRON_INCLUDES = \
	-I $(NEWTRON_DIR) \
	-I $(NEWTRON_DIR)/hw/hal/include/ \
	-I $(NEWTRON_DIR)/hw/bsp/meos/include/ \
	-I $(NEWTRON_DIR)/kernel/os/include/ \
	-I $(NEWTRON_DIR)/fs/nffs/include/ \
	-I $(NEWTRON_DIR)/fs/fs/include/ \
	-I $(NEWTRON_DIR)/libs/os/include/ \
	-I $(NEWTRON_DIR)/libs/util/include/ \
	-I $(NEWTRON_DIR)/test/testutil/include/ \
	-I $(NEWTRON_DIR)/fs/nffs/src/ \
	-I $(NEWTRON_DIR)/sys/defs/include/ \
	-I $(NEWTRON_DIR)/sys/mfg/include/ \
	-I $(NEWTRON_DIR)/sys/flash_map/include/ \
	-I $(NEWTRON_DIR)/sys/stats/include/ \
	-I $(NEWTRON_DIR)/util/crc/include/ \

NEWTRON_CFLAGS = -DLWIP_HDR_STATS_H=1 -DNFFS_AREA_MAX=$(CONFIG_NEWTRON_AREA_MAX) -DOS_ALIGNMENT=4 -Dos_sr_t=IRQ_IPL_T -D"OS_ENTER_CRITICAL(sr)=(sr)=IRQ_raiseIPL()" -D"OS_EXIT_CRITICAL(sr)=IRQ_restoreIPL(sr)" -D"SYSINIT_PANIC_ASSERT(rc)=DBG_assert(rc, \"NEWTRON SYSINIT ASSERT!\")" -D"MYNEWT_VAL(X)=MYNEWT_VAL_\#\#X"

.PRECIOUS: $(ABUILD_DIR)/obj/newtron/%.o
$(ABUILD_DIR)/obj/newtron/%.o: $(NEWTRON_DIR)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) $(NEWTRON_CFLAGS) $(NEWTRON_INCLUDES) -c $< -o $@
$(ABUILD_DIR)/obj/newtron/%.o: $(NEWTRON_ARCH)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) $(NEWTRON_CFLAGS) $(NEWTRON_INCLUDES) -c $< -o $@

.PHONY: newtron
newtron: $(NEWTRON_EXISTFILE) $(NEWTRON_LIBRARY)
	$(Q)echo $(call BANNER,NEWTRON,$@)

.PRECIOUS: $(NEWTRON_DIR)/%
$(NEWTRON_DIR)/%: $(NEWTRON_EXISTFILE)
	$(Q)mkdir -p $(dir $@)


$(ABUILD_DIR)/include/newtron/fs/fs/%.h: $(NEWTRON_DIR)/fs/fs/include/fs/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/newtron/fs/nffs/%.h: $(NEWTRON_DIR)/fs/nffs/include/nffs/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/newtron/flash_map/%.h: $(NEWTRON_DIR)/sys/flash_map/include/flash_map/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/newtron/hw/bsp/%.h: $(NEWTRON_DIR)/hw/bsp/meos/include/bsp/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

ifdef CONFIG_NEWTRON

CFLAGS += -I $(ABUILD_DIR)/include/newtron/fs
LFLAGS += -lnewtron
EXTRA_TARGETS += newtron
MISC_PREP += $(NEWTRON_EXISTFILE)
INSTALLED_HEADERS += \
	$(ABUILD_DIR)/include/newtron/fs/fs/fs.h \
	$(ABUILD_DIR)/include/newtron/fs/fs/fs_if.h \
	$(ABUILD_DIR)/include/newtron/fs/nffs/nffs.h \
	$(ABUILD_DIR)/include/newtron/hw/bsp/bsp.h

CLEAN_EXTRA += $(NEWTRON_LIBRARY_SHORT)
XMLDOCS += \
	support/3/newtron/newtron.xml

ALL_TESTS += $(call ADDTEST, regression/newtron)

endif
