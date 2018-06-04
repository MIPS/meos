INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/FPU_paranoia_single/FPU_paranoia_single.py

SRC0 = \
	paranoia1.c \
	paranoia2.c \
	main.c
ELF0 := tests/FPU_paranoia_single_single/FPU_paranoia_single-0.elf

LOG := logs/FPU_paranoia_single.log

#PRINT_OUT=YES
