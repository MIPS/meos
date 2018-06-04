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

FATFS_VERSION = ff12c
FATFS_FILE = $(FATFS_VERSION).zip
CONFIG_FATFS_SRC ?= http://elm-chan.org/fsw/ff/
FATFS_URL = $(CONFIG_FATFS_SRC)$(FATFS_FILE)
FATFS_ARCURL = $(CONFIG_FATFS_SRC)arc/$(FATFS_FILE)
FATFS_EXISTFILE := $(SUPPORT_DIR)/$(FATFS_VERSION)/doc/00index_e.html
$(FATFS_EXISTFILE):
	$(Q)mkdir -p $(SUPPORT_DIR)
	$(Q)echo $(call BANNER,DL,$(FATFS_VERSION))
	$(Q)rm -fr $(SUPPORT_DIR)/$(FATFS_VERSION) $(SUP)
	$(Q)mkdir -p $(SUPPORT_DIR) $(SUP)
	$(Q)$(CURL) --max-redirs 0 -L $(FATFS_URL) -o $(SUPPORT_DIR)/$(FATFS_FILE) || $(WGET) --max-redirect 0 $(FATFS_URL) -O $(SUPPORT_DIR)/$(FATFS_FILE) || $(Q)$(CURL) --max-redirs 0 -L $(FATFS_ARCURL) -o $(SUPPORT_DIR)/$(FATFS_FILE) || $(WGET) --max-redirect 0 $(FATFS_ARCURL) -O $(SUPPORT_DIR)/$(FATFS_FILE)
	$(Q)echo $(call BANNER,EX,$(FATFS_VERSION))
	$(Q)mkdir -p $(SUPPORT_DIR)/$(FATFS_VERSION) $(SUP)
	$(Q)unzip -d $(SUPPORT_DIR)/$(FATFS_VERSION) $(SUPPORT_DIR)/$(FATFS_FILE) $(SUP)

FATFS_DIR = $(SUPPORT_DIR)/$(FATFS_VERSION)
FATFS_ARCH = $(WORK_DIR)/support/3/fatfs

FATFS_SRC := \
	$(FATFS_DIR)/src/ff.c

FATFS_OBJ:=$(patsubst $(FATFS_DIR)/src/%.c,$(ABUILD_DIR)/obj/fatfs/%.o,$(filter %.c,$(FATFS_SRC)))

# FATFS_ARCH_SRC: Architecture specific files
FATFS_ARCH_SRC = \
	$(FATFS_ARCH)/disk.c \
	$(FATFS_ARCH)/syscall.c

FATFS_ARCH_OBJ:=$(patsubst $(FATFS_ARCH)/%.c,$(ABUILD_DIR)/obj/fatfs/%.o,$(filter %.c,$(FATFS_ARCH_SRC)))
FATFS_LIBRARY_SHORT := $(ABUILD_DIR)/lib/libfatfs.a
FATFS_LIBRARY := $(FATFS_LIBRARY_SHORT)($(FATFS_OBJ) $(FATFS_ARCH_OBJ))

FATFS_INCLUDES := \
	-I$(FATFS_ARCH) \
	-I$(ABUILD_DIR)/include/fatfs \
	-I$(ABUILD_DIR)/include/gen/fatfs

.PRECIOUS: $(FATFS_DIR)/%
$(FATFS_DIR)/%: $(FATFS_EXISTFILE)
	$(Q)mkdir -p $(dir $@)

.PRECIOUS: $(ABUILD_DIR)/obj/fatfs/%.o
$(ABUILD_DIR)/obj/fatfs/%.o: $(FATFS_DIR)/src/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)rm -f $(FATFS_DIR)/src/ffconf.h
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(FATFS_INCLUDES) $(INC) -c $< -o $@
$(ABUILD_DIR)/obj/fatfs/%.o: $(FATFS_ARCH)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)rm -f $(FATFS_DIR)/src/ffconf.h
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(FATFS_INCLUDES) $(INC) -c $< -o $@

.PHONY: fatfs
fatfs: $(FATFS_EXISTFILE) $(FATFS_LIBRARY)
	$(Q)echo $(call BANNER,FATFS,$@)

$(ABUILD_DIR)/include/fatfs/%.h: $(FATFS_DIR)/src/%.h $(FATFS_EXISTFILE)
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/fatfs/%.h: $(FATFS_ARCH)/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

ifdef CONFIG_FATFS
CFLAGS += -I$(ABUILD_DIR)/include/fatfs
LFLAGS += -lfatfs
EXTRA_TARGETS += fatfs
INSTALLED_HEADERS += $(ABUILD_DIR)/include/fatfs/diskio.h $(ABUILD_DIR)/include/fatfs/ff.h $(ABUILD_DIR)/include/fatfs/integer.h $(ABUILD_DIR)/include/fatfs/ffconf.h
INC += $(FATFS_INCLUDES)
CLEAN_EXTRA += $(FATFS_LIBRARY_SHORT)

ALL_TESTS += $(call ADDTEST, regression/fatfs)
endif
