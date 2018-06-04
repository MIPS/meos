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
#   Description:	PIC32 target specialisation
#
##########################################################################

ifdef CONFIG_ARCH_MIPS_PIC

INC += -I$(WORK_DIR)/targets/mips/common -I$(WORK_DIR)/targets/mips/eic
GENERATED_HEADERS += $(ABUILD_DIR)/include/picpart.h

$(ABUILD_DIR)/include/picpart.h:
	$(Q)echo $(call BANNER,INSH,$<)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo "#include <proc/p$(call LOWER,$(patsubst "%",%,$(CONFIG_ARCH_MIPS_PIC_PROC))).h>" > $@ $(SUP2)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/meos/%.h: targets/mips/pic32/%.h $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$<)
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

include $(MEOS_SRC)/targets/mips/common/module.mk

CTX_SRC += targets/mips/common/ctx.c
DBG_SRC += targets/mips/pic32/dbg.c
IRQ_SRC += targets/mips/pic32/irq.c
TMR_SRC += targets/mips/pic32/tmr.c

SRC += \
	targets/mips/common/ctx.c \
	targets/mips/pic32/dbg.c \
	targets/mips/pic32/irq.c \
	targets/mips/pic32/tmr.c

HEADERS += \
	target/irq1.h \
	target/irq2.h \
	target/krnspec.h \
	target/tmr.h \

KERNEL_SRC += \
	targets/mips/eic/head.S \
	targets/mips/pic32/svechead.S \

ASMSRC += \
	targets/mips/eic/head.S \
	targets/mips/pic32/svechead.S \

CFLAGS += -I$(P32PS_PATH)/include
LFLAGS += -L$(P32PS_PATH)/lib -lp$(call LOWER, $(patsubst "%",%,$(CONFIG_ARCH_MIPS_PIC_PROC)))
MISC_PREP += $(P32PS_LIBFILE)

endif
