INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/timing/timing.py

SRC0 := main.c
ELF0 := tests/timing/timing-0.elf

LOG := logs/timing.log

PRINT_OUT=YES

include $(SPEC_DIR)/target/$(TARGET)/spec.mk
