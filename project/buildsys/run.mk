# Pull in the configuration file generated in the previous phases
sinclude $(CONFIG)
include $(WORK_DIR)/buildsys/targets.mk
ifndef OVERRIDERUN
sinclude $(WORK_DIR)/buildsys/target/$(TARGET)/target.mk
endif

ifndef DISABLE
ifdef OVERRIDERUN
run: $(OVERRIDERUN)
	@:
else
run: $(LOG)
	@:
endif
endif

%::
	@:
