include $(SPEC_DIR)/spec.mk

ifdef KCONFIG_TOOL

.PHONY: config
config:	$(KCONFIG_EXECFILE)
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR);if [ -f $(SPEC_DIR)/Kconfig ]; then WORK_DIR=$(WORK_DIR) SPEC_CFG=$(SPEC_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_TOOL) $(WORK_DIR)/buildsys/Kconfig; else WORK_DIR=$(WORK_DIR) SPEC_CFG=$(WORK_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_TOOL) $(WORK_DIR)/buildsys/Kconfig;fi
	#echo "[3A                                                         "
	#echo "                                                         [3A"
	if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: menuconfig
menuconfig: config

.PHONY: oldconfig
oldconfig: $(KCONFIG_EXECFILE)
	$(Q)if [ ! -f $(CONFIG) ]; then echo No configuration file! Please see the \'Configuration\' section of the manual!; false; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR);if [ -f $(SPEC_DIR)/Kconfig ]; then WORK_DIR=$(WORK_DIR) SPEC_CFG=$(SPEC_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --oldconfig $(WORK_DIR)/buildsys/Kconfig  $(SUP2); else WORK_DIR=$(WORK_DIR) SPEC_CFG=$(WORK_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --oldconfig $(WORK_DIR)/buildsys/Kconfig  $(SUP2);fi
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: randconfig
randconfig:	$(KCONFIG_EXECFILE)
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR);if [ -f $(SPEC_DIR)/Kconfig ]; then WORK_DIR=$(WORK_DIR) SPEC_CFG=$(SPEC_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --randconfig $(WORK_DIR)/buildsys/Kconfig; else WORK_DIR=$(WORK_DIR) SPEC_CFG=$(WORK_DIR)/Kconfig KCONFIG_CONFIG=$(CONFIG) $(OLDCONFIG_TOOL) --randconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2);fi
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

%_defconfig:
	$(Q)echo No such configuration!; false

endif

%::
	@:
