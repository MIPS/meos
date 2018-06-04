INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/interThread/interThread.py

SRC0 := main.c
ELF0 := tests/interThread/interThread-0.elf
SRC1 := main.c
ELF1 := tests/interThread/interThread-1.elf
ifndef CONFIG_ARCH_MIPS_BASELINE
SRC2 := main.c
ELF2 := tests/interThread/interThread-2.elf
SRC3 := main.c
ELF3 := tests/interThread/interThread-3.elf
endif

LOG := logs/interThread.log

#PRINT_OUT=YES
