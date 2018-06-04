INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

sinclude $(MEOS_CONFIG)

PY := tests/coverage/coverage.py

SRC0 := \
	main.c \
	lst.c \
	lst_inline.c \
	dq.c \
	dq_inline.c\
	tre.c \
	krn.c \
	dbg.c
ifdef CONFIG_QIO
SRC0 += \
	qio.c \
	nulldev.c
endif

ELF0 := tests/coverage/coverage-0.elf

LOG := logs/coverage.log

#PRINT_OUT=YES
