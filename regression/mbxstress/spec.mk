INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/mbxstress/mbxstress.py

SRC0 := master.c
ELF0 := tests/mbxstress/mbxstress-0.elf
SRC1 := slave.c
ELF1 := tests/mbxstress/mbxstress-1.elf
ifndef CONFIG_ARCH_MIPS_BASELINE
SRC2 := slave.c
ELF2 := tests/mbxstress/mbxstress-2.elf
SRC3 := slave.c
ELF3 := tests/mbxstress/mbxstress-3.elf
endif

LOG := logs/mbxstress.log
RPROC := Y

#PRINT_OUT=YES
