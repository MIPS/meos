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
#   Description:	libhal build rules
#
##########################################################################

LIBHAL_SRC= \
	targets/mips/common/hal/hal/__exit.c \
	targets/mips/common/hal/hal/get_ram_range.c \
	targets/mips/common/hal/hal/link.c \
	targets/mips/common/hal/hal/mips_bemu.c \
	targets/mips/common/hal/hal/mips_excpt_handler.c \
	targets/mips/common/hal/hal/mips_intctrl.c \
	targets/mips/common/hal/hal/mips_lsemu.c \
	targets/mips/common/hal/hal/syscalls.c \
	targets/mips/common/hal/uhi/uhi_exception.c \
	targets/mips/common/hal/uhi/uhi_indirect.c

LIBHAL_ASMSRC= \
	targets/mips/common/hal/hal/abiflags.S \
	targets/mips/common/hal/hal/cache_ops.S \
	targets/mips/common/hal/hal/cache.S \
	targets/mips/common/hal/hal/crt0.S \
	targets/mips/common/hal/hal/m32cache_ops.S \
	targets/mips/common/hal/hal/m32cache.S \
	targets/mips/common/hal/hal/m32tlb_ops.S \
	targets/mips/common/hal/hal/mips_cm3_l2size.S \
	targets/mips/common/hal/hal/mips_excpt_boot.S \
	targets/mips/common/hal/hal/mips_excpt_entry.S \
	targets/mips/common/hal/hal/mips_excpt_isr_fallback.S \
	targets/mips/common/hal/hal/mips_excpt_isr_fragment.S \
	targets/mips/common/hal/hal/mips_excpt_isr.S \
	targets/mips/common/hal/hal/mips_excpt_register.S \
	targets/mips/common/hal/hal/mips_excpt_timer.S \
	targets/mips/common/hal/hal/mips_l2size.S \
	targets/mips/common/hal/hal/mxxtlb_ops.S

# Rules of how to build the objects, and where they go
$(ABUILD_DIR)/obj/targets/mips/common/hal/%.o: targets/mips/common/hal/%.c $(TC_TARGET) $(INDENT_EXEC)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),$(INDENT) targets/mips/common/hal/$*.c -st|$(INDENT) -st -|diff targets/mips/common/hal/$*.c - > /dev/null || ( echo $(call BANNER,FIXC,$(<));chmod u+w targets/mips/common/hal/$*.c;$(INDENT) targets/mips/common/hal/$*.c;$(INDENT) targets/mips/common/hal/$*.c ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKC,$(<));$(INDENT) targets/mips/common/hal/$*.c -st|$(INDENT) -st -|diff targets/mips/common/hal/$*.c - > /dev/null || ( echo "targets/mips/common/hal/$*.c breaks indentation policy: run $(INDENT) $(CURDIR)/targets/mips/common/hal/$*.c"; false ))
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) -fno-instrument-functions $(INC) -c $(WORK_DIR)/targets/mips/common/hal/$*.c -o $@

ifdef CONFIG_ARCH_MIPS_HARD_FLOAT
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_fp.S
endif
ifdef CONFIG_ARCH_MIPS_MSA
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_msa.S
PRIVATE_CFLAGS += -DHAVE_MSA
endif
ifdef CONFIG_ARCH_MIPS_VZ
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_vz.S
endif
ifdef CONFIG_ARCH_MIPS_DSP
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_dsp.S
endif
ifdef CONFIG_ARCH_MIPS_MICRO
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_xpa.S
PRIVATE_CFLAGS += -DHAVE_XPA
endif
ifdef CONFIG_ARCH_MIPS_MICRO_R6
LIBHAL_ASMSRC += targets/mips/common/hal/hal/mips_xpa.S
PRIVATE_CFLAGS += -DHAVE_XPA
endif

PRIVATE_CFLAGS += -DVERBOSE_EXCEPTIONS

LIBHAL_COBJ:=$(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(filter %.c,$(LIBHAL_SRC)))
LIBHAL_SOBJ:=$(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(filter %.s,$(LIBHAL_ASMSRC))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(filter %.S,$(LIBHAL_ASMSRC)))
LIBHAL_OBJ=$(LIBHAL_COBJ) $(LIBHAL_SOBJ)

.PRECIOUS: $(LIBHAL_OBJ)

LIBHAL_LIBRARY_SHORT=$(ABUILD_DIR)/lib/libhal.a
LIBHAL_LIBRARY=$(LIBHAL_LIBRARY_SHORT)($(LIBHAL_OBJ))

.PHONY: halh
halh:
	$(Q)cp -Rf $(MEOS_SRC)/targets/mips/common/hal/include/* $(ABUILD_DIR)/include/

.PHONY: libhal
libhal: halh $(LIBHAL_LIBRARY)

$(ABUILD_DIR)/lib/libhal.a: libhal

INC += -isystem $(ABUILD_DIR)/include
LFLAGS += -L $(ABUILD_DIR)/lib
MISC_PREP += $(ABUILD_DIR)/lib/libhal.a
CLEAN_EXTRA += $(LIBHAL_LIBRARY_SHORT)
