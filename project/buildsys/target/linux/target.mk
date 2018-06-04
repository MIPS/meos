# Some defaults
COMMA=,
TARGET_PROPAGATE ?= SOFAR="$(if $(SOFAR),$(SOFAR)$(COMMA),)'$(ABUILD_DIR)/$(ELF)'"

# Calculate the next CPU number for chained builds
PLUSONE := $(shell expr $(CPU) + 1)


ifdef ELF0
ifndef CONFIG_ARCH_LINUX_MIPS
ifndef CONFIG_ARCH_LINUX_MIPS_IMG
# Hook into run to execute load script
.PHONY: $(LOG)
$(LOG): $(ABUILD_DIR)/$(ELF0)
	$(Q)mkdir -p $(dir $@)
	$(Q)sleep 2
	$(Q)sh $(WORK_DIR)/buildsys/run.sh $(<) $@ $(<) $(WORKDIR) $(PRINT_OUT) | tee -a $(UNILOG)
else
.PHONY: $(LOG)
$(LOG): $(ABUILD_DIR)/$(ELF0)
	$(Q)echo $(call BANNER,CANTRUN,$(@))
endif
else
.PHONY: $(LOG)
$(LOG): $(ABUILD_DIR)/$(ELF0)
	$(Q)echo $(call BANNER,CANTRUN,$(@))
endif
endif
