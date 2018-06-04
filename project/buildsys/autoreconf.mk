# Start build, but only if we have a good configuration
.PHONY: _internal
_internal: $(CONFIG)
	$(Q)$(MAKE) --no-print-directory _BUILD_PHASE=BUILD CPU=0 $(MAKECMDGOALS)

ifdef KCONFIG_TOOL

$(CONFIG): $(OLDCONFIG_TOOL) buildsys/Kconfig $(wildcard spec.kconfig)
	$(Q)if [ ! -f $(CONFIG) ]; then echo No configuration file! Please invoke the config target!; false; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR);if [ -f $(SPEC_DIR)/Kconfig ]; then WORK_DIR=$(WORK_DIR) SPEC_CFG=$(SPEC_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --oldconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2); else WORK_DIR=$(WORK_DIR) SPEC_CFG=$(WORK_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --oldconfig $(WORK_DIR)/buildsys/Kconfig  $(SUP2);fi
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

endif

.PHONY: $(firstword $(MAKECMDGOALS))
$(firstword $(MAKECMDGOALS)): _internal

%:: ;
