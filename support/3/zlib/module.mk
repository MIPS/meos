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

ZLIB_VERSION = zlib-1.2.8
ZLIB_FILE = $(ZLIB_VERSION).tar.gz
CONFIG_ZLIB_SRC ?= http://sourceforge.mirrorservice.org/l/project/li/libpng/zlib/1.2.8/
ZLIB_URL = $(CONFIG_ZLIB_SRC)$(ZLIB_FILE)
ZLIB_EXISTFILE := $(SUPPORT_DIR)/$(ZLIB_VERSION)/zutil.h
$(ZLIB_EXISTFILE):
	$(Q)mkdir -p $(SUPPORT_DIR)
	$(Q)echo $(call BANNER,DL,$(ZLIB_VERSION))
	$(Q)rm -fr $(SUPPORT_DIR)/$(ZLIB_VERSION) $(SUP)
	$(Q)mkdir -p $(SUPPORT_DIR) $(SUP)
	$(Q)$(CURL) -L $(ZLIB_URL) -o $(SUPPORT_DIR)/$(ZLIB_FILE) || $(WGET) $(ZLIB_URL) -O $(SUPPORT_DIR)/$(ZLIB_FILE)
	$(Q)echo $(call BANNER,EX,$(ZLIB_VERSION))
	$(Q)mkdir -p $(SUPPORT_DIR)/$(ZLIB_VERSION) $(SUP)
	$(Q)tar -C $(SUPPORT_DIR) -xvzf $(SUPPORT_DIR)/$(ZLIB_FILE) $(SUP)

ZLIB_DIR = $(SUPPORT_DIR)/$(ZLIB_VERSION)

.PHONY: zlib
zlib: $(ZLIB_EXISTFILE)
	$(Q)echo $(call BANNER,ZLIB,$@)
	$(Q)cd $(ZLIB_DIR);(CFLAGS="$(CFLAGS)" CROSS_PREFIX=mips-mti-elf- ./configure --prefix=$(ABUILD_DIR) && make libz.a && make install) $(SUP)

$(ABUILD_DIR)/include/zlib.h: $(ZLIB_EXISTFILE)
	$(Q)echo $(call BANNER,ZLIB,$@)
	$(Q)cd $(ZLIB_DIR);(CFLAGS="$(CFLAGS)" CROSS_PREFIX=mips-mti-elf- ./configure --prefix=$(ABUILD_DIR) && make libz.a && make install) $(SUP)

$(ABUILD_DIR)/include/zconf.h: $(ZLIB_EXISTFILE)
	$(Q)echo $(call BANNER,ZLIB,$@)
	$(Q)cd $(ZLIB_DIR);(CFLAGS="$(CFLAGS)" CROSS_PREFIX=mips-mti-elf- ./configure --prefix=$(ABUILD_DIR) && make libz.a && make install) $(SUP)

ifdef CONFIG_ZLIB
LFLAGS += -lz
EXTRA_TARGETS += $(ABUILD_DIR)/include/zlib.h $(ABUILD_DIR)/include/zconf.h
INSTALLED_HEADERS += $(ABUILD_DIR)/include/zlib.h $(ABUILD_DIR)/include/zconf.h
CLEAN_EXTRA += $(ABUILD_DIR)/lib/libz.a $(ABUILD_DIR)/include/zconf.h $(ABUILD_DIR)/include/zlib.h
endif
