ifdef CONFIG_ARCH_MIPS_VZ
# Trick uhi32.ld into giving us enough space
# This can be removed when the new hal vector code is integrated.
WALK_VARS+=-DvectorCount=36
endif

# Some defaults
PY ?= $(ELF0).py
RESET ?= $(WORK_DIR)/buildsys/target/$(TARGET)/reset.py
TEST ?= $(WORK_DIR)/buildsys/target/$(TARGET)/test.py
PROBE ?= $(DA)

ifdef EMULATE
RUN_SCRIPT ?= $(1) --emulator $(EMULATE) --reset $(RESET) --load $(1)l --xml --xml-dir $(2) --verbose
endif
ifdef ACQUIRE
RUN_SCRIPT ?= $(1) --acquire $(ACQUIRE) --reset $(RESET) --load $(1)l --xml --xml-dir $(2) --verbose
endif
ifdef PROBE
RUN_SCRIPT ?= $(1) --probe $(PROBE) --reset $(RESET) --load $(1)l --xml --xml-dir $(2) --verbose
endif

COMMA=,
TARGET_PROPAGATE ?= SOFAR="$(if $(SOFAR),$(SOFAR)$(COMMA),)'$(ABUILD_DIR)/$(ELF)'"

# Calculate the next CPU number for chained builds
PLUSONE := $(shell expr $(CPU) + 1)

ifdef ELF0
# Hook into build to generate load script
ifeq ($(SRC$(PLUSONE))$(ASMSRC$(PLUSONE)),)
RUNDEP += $(ABUILD_DIR)/$(PY)
endif

CONFIG_FEATURE_MAX_PROCESSORS ?= 1024

$(ABUILD_DIR)/$(PY)l: $(SPEC_DIR)/spec.mk
	$(Q)echo $(call BANNER,PYL,$(@))
	$(Q)mkdir -p $(dir $@)
	$(Q)echo "threads=$(shell if [ $(CONFIG_FEATURE_MAX_PROCESSORS)	-lt $(PLUSONE) ]; then echo $(CONFIG_FEATURE_MAX_PROCESSORS); else echo $(PLUSONE); fi)" > $@
	$(Q)echo "apps=[$(if $(SOFAR),$(SOFAR)$(COMMA),)'$(ABUILD_DIR)/$(ELF)']" >> $@

$(ABUILD_DIR)/$(PY): $(ABUILD_DIR)/$(PY)l $(SPEC_DIR)/spec.mk $(RESET)
	$(Q)echo $(call BANNER,PY,$(@))
	$(Q)mkdir -p $(dir $@)
	$(Q)echo "#!/usr/bin/env Codescape-Python" > $@
	$(Q)echo "import sys" >> $@
	$(Q)echo "sys.path.append(\"$(WORK_DIR)/buildsys/target/$(TARGET)/\")" >> $@
	$(Q)echo "import project" >> $@
	$(Q)echo "rsts=\"$(RESET)\"" >> $@
	$(Q)echo "ldrs=\"$(ABUILD_DIR)/$(PY)l\"" >> $@
	$(Q)cd $(SPEC_DIR);cat $(TEST) >> $@
	$(Q)chmod gou+x $@

# Hook into run to execute load script
.PHONY: $(LOG)
$(LOG): $(ABUILD_DIR)/$(PY)
	$(Q)mkdir -p $(dir $@)
	$(Q)sleep 2
	$(Q)sh $(WORK_DIR)/buildsys/run.sh $(<) $@ "$(call RUN_SCRIPT,$(<),$(dir $@)xml)" $(WORK_DIR) $(PRINT_OUT) | tee -a $(UNILOG)
endif
