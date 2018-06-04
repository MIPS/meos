.PHONY: clean
ifdef OVERRIDECLEAN
clean: $(OVERRIDECLEAN)
else
clean: bannerclean elfclean restclean
endif

# Calculate the next CPU number for chained builds
PLUSONE := $(shell expr $(CPU) + 1)

.PHONY: bannerclean
bannerclean:
	$(Q)echo $(call BANNER,CLEAN,)

# Chain through to clean all ELFs
.PHONY: elfclean
elfclean:
	$(Q)-rm -f $(ELF$(CPU))
	$(Q)$(if $(ELF$(PLUSONE)),$(MAKE) --no-print-directory _BUILD_PHASE=CLEAN CPU=$(PLUSONE) elfclean,)

.PHONY: restclean
restclean:
	$(Q)-rm -fr $(ABUILD_DIR)/dep
	$(Q)-rm -fr $(ABUILD_DIR)/include
	$(Q)-rm -fr $(ABUILD_DIR)/obj
	$(Q)-rm -fr $(ABUILD_DIR)/bin
	$(Q)-rm -fr $(ABUILD_DIR)/doc
	$(Q)-rm -fr $(ABUILD_DIR)/logs
	$(Q)-rm -fr $(LDR)

%::
	@:
