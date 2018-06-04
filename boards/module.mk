####(C)2016###############################################################
#
# Copyright (C) 2016 MIPS Tech, LLC
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
####(C)2016###############################################################
#
#   Description:	Support build rules
#
##########################################################################

BOARDS := none custom mvz mti-sead3 mti-malta chipkit-wifire xlnx-nexys4ddr pic32-starterkit rpusim
include $(patsubst %, boards/%/module.mk,$(BOARDS))

.PRECIOUS: $(ABUILD_DIR)/obj/bsp.dts
$(ABUILD_DIR)/obj/bsp.dts: $(BSP)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,BSPI,$<)
	$(Q)cp $< $@

.PRECIOUS: $(ABUILD_DIR)/obj/bsp.dtsi
$(ABUILD_DIR)/obj/bsp.dtsi: $(ABUILD_DIR)/obj/bsp.dts
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTSI,$<)
	$(Q)$(CC) -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp -I $(dir %<) $< -o $@

.PRECIOUS: $(ABUILD_DIR)/obj/bsp.dtb
$(ABUILD_DIR)/obj/bsp.dtb: $(ABUILD_DIR)/obj/bsp.dtsi $(DTC_EXECFILE)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTC,$<)
	$(Q)$(DTC_EXECFILE) -o $@ -O dtb $<

.PRECIOUS: $(ABUILD_DIR)/obj/bsp.c
$(ABUILD_DIR)/obj/bsp.c: $(ABUILD_DIR)/obj/bsp.dtb $(DTWALK_TOOL) $(ABUILD_DIR)/walks
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTWC,$<)
	$(Q)$(DTWALK_TOOL) $(ABUILD_DIR)/walks/drivers/c $< > $@

.PRECIOUS: $(ABUILD_DIR)/obj/bsp.o
$(ABUILD_DIR)/obj/%.o: $(ABUILD_DIR)/obj/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -c $(ABUILD_DIR)/obj/$*.c -o $@

XOBJ += $(ABUILD_DIR)/obj/bsp.o
CLEAN_EXTRA += \
	$(ABUILD_DIR)/obj/bsp.dtb \
	$(ABUILD_DIR)/obj/bsp.c
