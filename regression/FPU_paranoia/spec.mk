INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

PY := tests/FPU_paranoia/FPU_paranoia.py

SRC0 = \
	paranoia1.c \
	paranoia2.c \
	main.c

ELF0 := tests/FPU_paranoia/FPU_paranoia-0.elf

LOG := logs/FPU_paranoia.log

#PRINT_OUT=YES
