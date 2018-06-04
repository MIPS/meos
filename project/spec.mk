# Indentation policy - these flags will be used with GNU indent to enforce code
# quality. Source will not be checked if left undefined. Uncomment the line
# below to use the same settings as MEOS.
#INDENT_POLICY := -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

# Load script to be generated within $BUILD_DIR.
PY := bin/main.py

# Execution log to be generated within $BUILD_DIR.
LOG := logs/out.log

# ELF file to be generated for processor 0 within $BUILD_DIR.
ELF0 := bin/main-0.elf

# Source to be built for processor 0 from working directory.
SRC0 := src/main.c

# Assembler source to be built for processor 0 from working directory.
#ASMSRC0 :=

# Source to be built for each TC on processor 0 from working directory.
#PTCSRC0 :=

# Additional compiler to be used for compilation
# override CFLAGS +=

# Additional linker flags to be used for linking
# override LFLAGS +=

# imgtec.test compliant test case. The default just runs the ELFs to completion.
#TEST :=

# Define to always show execution results. Otherwise, only failed execution will
# dump output to stdout. Note that output will always be logged to $LOG.
#PRINT_OUT=YES
