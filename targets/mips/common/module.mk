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
#   Description:	MIPS common target specialisation
#
##########################################################################

ifdef CONFIG_ARCH_MIPS

$(ABUILD_DIR)/include/meos/%.h: targets/mips/common/%.h $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$<)
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

HEADERS += \
	target/ctx.h \
	target/dbg.h \
	target/krn.h \
	target/ipm.h \
	target/mem.h

#
# Build setup
#

# Always need these
PRIVATE_CFLAGS := -g$(CONFIG_BUILD_G) -O$(CONFIG_BUILD_O) -Wall
CFLAGS += $(patsubst "%,%,$(patsubst %",%,$(strip $(CONFIG_EXTRA_FLAGS))))
LFLAGS += $(patsubst "%,%,$(patsubst %",%,$(strip $(CONFIG_EXTRA_FLAGS))))

ifdef CONFIG_ARCH_MIPS_BIG_ENDIAN
MIPSFLAGS += -EB
endif
ifdef CONFIG_ARCH_MIPS_LITTLE_ENDIAN
MIPSFLAGS += -EL
endif

ifdef CONFIG_ARCH_MIPS_MICRO_R6
MIPSFLAGS += -march=mips32r6
endif
ifdef CONFIG_ARCH_MIPS_R6
MIPSFLAGS += -march=mips32r6
endif
ifndef CONFIG_ARCH_MIPS_R6
ifndef CONFIG_ARCH_MIPS_MICRO_R6
MIPSFLAGS += -mips32r2
endif
endif

ifdef CONFIG_ARCH_MIPS_NO_FLOAT
MIPSFLAGS += -mno-float
endif
ifdef CONFIG_ARCH_MIPS_SOFT_FLOAT
MIPSFLAGS += -msoft-float
endif
ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
MIPSFLAGS += -mhard-float
endif

ifdef CONFIG_ARCH_MIPS_DSP
MIPSFLAGS += -mdsp
endif

ifdef CONFIG_ARCH_MIPS_MICRO
MIPSFLAGS += -mmicromips
endif
ifdef CONFIG_ARCH_MIPS_MICRO_R6
MIPSFLAGS += -mmicromips
endif

ifdef CONFIG_ARCH_MIPS_INTERLINK
MIPSFLAGS += -minterlink-compressed
endif

ifdef CONFIG_ARCH_MIPS_MANY_SECTIONS
MIPSFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections
endif

ifdef CONFIG_ARCH_MIPS_MT
MIPSFLAGS += -mmt
endif

MEM_SRC += targets/mips/common/mem.c

SRC += \
	targets/mips/common/mem.c

ifdef CONFIG_ARCH_MIPS_MSA
MIPSFLAGS += -mmsa

endif

CFLAGS := $(MIPSFLAGS) $(CFLAGS)
LFLAGS := $(MIPSFLAGS) $(LFLAGS)
TARGET_FLAGS := $(MIPSFLAGS)

TARGET_NAME := mips

endif
