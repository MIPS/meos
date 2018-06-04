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
#   Description:	MIPS HW target specialisation
#
##########################################################################
ifdef CONFIG_ARCH_MIPS_HWSTAT

INC += -I$(WORK_DIR)/targets/mips/common -I$(WORK_DIR)/targets/mips/hw -I$(WORK_DIR)/targets/mips/eic

$(ABUILD_DIR)/include/meos/%.h: targets/mips/hwstat/%.h $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$<)
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

include $(MEOS_SRC)/targets/mips/common/module.mk

CTX_SRC += targets/mips/common/ctx.c
DBG_SRC += targets/mips/hwstat/dbg.c
IRQ_SRC += targets/mips/hwstat/irq.c
TMR_SRC += targets/mips/hwstat/tmr.c

SRC += \
	targets/mips/common/ctx.c \
	targets/mips/hwstat/dbg.c \
	targets/mips/hwstat/irq.c \
	targets/mips/hwstat/tmr.c

HEADERS += \
	target/irq1.h \
	target/irq2.h \
	target/krnspec.h \
	target/tmr.h \

KERNEL_SRC += \
	targets/mips/eic/head.S \

ASMSRC += \
	targets/mips/eic/head.S \


endif
