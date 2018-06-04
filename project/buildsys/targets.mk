include $(SPEC_DIR)/spec.mk

# Pick out the contribution for this CPU
SRC := $(SRC$(CPU))
ASMSRC := $(ASMSRC$(CPU))
PTCSRC := $(PTCSRC$(CPU))
PTCASMSRC := $(PTCASMSRC$(CPU))
ELF := $(ELF$(CPU))
ELF ?= bin/$(CPU).elf

# Select DT
ifeq ($(DT$(CPU)),)
ifeq ($(DT),)
# No DT specified - pull the BSPs
DTS := $(MEOS_DIR)/obj/bsp.dts
else
# Single DT specified
DTS := $(DT)
endif
else
# Per cpu DT specified
DTS := $(DT$(CPU))
endif

COBJ := $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.c,$(filter-out /%,$(SRC))),$(abspath $(SPEC_DIR)/$(file))))
CPPOBJ := $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.cpp,$(filter-out /%,$(SRC))),$(abspath $(SPEC_DIR)/$(file)))) $(patsubst %.C,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.C,$(filter-out /%,$(SRC))),$(abspath $(SPEC_DIR)/$(file))))
SOBJ := $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.s,$(filter-out /%,$(ASMSRC))),$(abspath $(SPEC_DIR)/$(file)))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.S,$(filter-out /%,$(ASMSRC))),$(abspath $(SPEC_DIR)/$(file))))
COBJ += $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.c,$(filter /%,$(SRC))),$(abspath $(file))))
CPPOBJ += $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.cpp,$(filter /%,$(SRC))),$(abspath $(file)))) $(patsubst %.C,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.C,$(filter /%,$(SRC))),$(abspath $(file))))
SOBJ += $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.s,$(filter /%,$(ASMSRC))),$(abspath $(file)))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.S,$(filter /%,$(ASMSRC))),$(abspath $(file))))

OBJ := $(strip $(COBJ) $(CPPOBJ) $(SOBJ))

PTCCOBJ := $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.c,$(filter-out /%,$(PTCSRC))),$(abspath $(SPEC_DIR)/$(file))))
PTCCPPOBJ := $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.cpp,$(filter-out /%,$(PTCSRC))),$(abspath $(SPEC_DIR)/$(file)))) $(patsubst %.C,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.C,$(filter-out /%,$(PTCSRC))),$(abspath $(SPEC_DIR)/$(file))))
PTCSOBJ := $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.s,$(filter-out /%,$(PTCASMSRC))),$(abspath $(SPEC_DIR)/$(file)))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.S,$(filter-out /%,$(PTCASMSRC))),$(abspath $(SPEC_DIR)/$(file))))
PTCCOBJ += $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.c,$(filter /%,$(PTCSRC))),$(abspath $(file))))
PTCCPPOBJ += $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.cpp,$(filter /%,$(PTCSRC))),$(abspath $(file)))) $(patsubst %.C,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.C,$(filter /%,$(PTCSRC))),$(abspath $(file))))
PTCSOBJ += $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.s,$(filter /%,$(PTCASMSRC))),$(abspath $(file)))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(foreach file,$(filter %.S,$(filter /%,$(PTCASMSRC))),$(abspath $(file))))

PTCOBJ := $(strip $(PTCCOBJ) $(PTCCPPOBJ) $(PTCSOBJ))

DEPFILES = $(patsubst $(ABUILD_DIR)/obj/%.o,$(ABUILD_DIR)/dep/%.d,$(COBJ) $(CPPOBJ) $(PTCCOBJ) $(PTCCPPOBJ))
LOG ?= $(ABUILD_DIR)/log/out.log
ifdef RPROC
MLFLAGS += $(shell $(MEOS_DIR)/bin/meos-config --rproc)
endif
