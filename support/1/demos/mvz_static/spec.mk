INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := demos/mvz_static/mvz_static.py

ROOTDTS := root.dts
ROOTDTSI := $(ABUILD_DIR)/src/$(notdir $(ROOTDTS)).rootdtsi
ROOTDTB := $(ABUILD_DIR)/src/$(notdir $(ROOTDTS)).rootdtb
ROOTDTB_C := $(ROOTDTB).c

GUESTS := guest0.elf guest1.elf guest2.elf
GUESTS_C := $(patsubst %.elf,$(ABUILD_DIR)/src/%.elf.c,$(GUESTS))

SRC0 := main.c $(ROOTDTB_C) $(GUESTS_C)
ELF0 := demos/mvz_static/mvz_static-0.elf

LOG := logs/mvz_static.log

#PRINT_OUT=YES

##

# DTB
.PRECIOUS: $(ROOTDTSI)
$(ROOTDTSI): $(SPEC_DIR)/$(ROOTDTS) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,BSPI,$<)
	$(Q)$(CC) -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp -I $(dir %<) $< -o $@

$(ROOTDTB): $(ROOTDTSI) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTC,$<)
	$(Q)$(DTC_TOOL) -o $@ -O dtb $<

$(ROOTDTB_C): $(ROOTDTB) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTWC,$<)
	$(Q)$(DTWALK_TOOL) $(SPEC_DIR)/walks/mvz/c $< -Dprocessor=$(CPU) $(WALK_VARS) > $@

# Guest

.PRECIOUS: $(ABUILD_DIR)/src/%.elf.c
$(ABUILD_DIR)/src/%.elf.c: $(SPEC_DIR)/%.elf
	$(Q)rm -f $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(WORK_DIR)/buildsys/f2c.py $@ _binary_`basename $<|cut -d'.' -f1`_elf $<
