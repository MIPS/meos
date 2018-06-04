INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := demos/mvz_comms/mvz_comms.py

GUESTS := guest0.elf guest1.elf
GUESTS_C := $(patsubst %.elf,$(ABUILD_DIR)/src/%.elf.c,$(GUESTS))

SRC0 := main.c  $(GUESTS_C)
ELF0 := demos/mvz_comms/mvz_comms-0.elf

LOG := logs/mvz_comms.log

#PRINT_OUT=YES

##

# Guest

.PRECIOUS: $(ABUILD_DIR)/src/%.elf.c
$(ABUILD_DIR)/src/%.elf.c: $(SPEC_DIR)/%.elf
	$(Q)rm -f $@
	$(Q)mkdir -p $(dir $@)
	$(Q)$(WORK_DIR)/buildsys/f2c.py $@ _binary_`basename $<|cut -d'.' -f1`_elf $<
