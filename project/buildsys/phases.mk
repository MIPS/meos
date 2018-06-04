

include buildsys/init.mk

ifeq ($(_BUILD_PHASE),)

.PHONY: _internal
_internal:
	$(Q)$(MAKE) --no-print-directory _BUILD_PHASE=CLEAN $(filter clean,$(MAKECMDGOALS)) silent
	$(Q)$(MAKE) --no-print-directory _BUILD_PHASE=CONFIG $(filter %config,$(MAKECMDGOALS)) silent
	$(if $(filter clean %config,$(MAKECMDGOALS)),$(if $(filter-out clean %config,$(MAKECMDGOALS)),$(Q)$(MAKE) --no-print-directory _BUILD_PHASE=AUTORECONF $(filter-out clean %config,$(MAKECMDGOALS)),@:),$(Q)$(MAKE) --no-print-directory _BUILD_PHASE=AUTORECONF $(filter-out clean %config,$(MAKECMDGOALS)))

.PHONY: $(firstword $(MAKECMDGOALS))
$(firstword $(MAKECMDGOALS)): _internal

%::
	@:
endif

ifeq ($(_BUILD_PHASE),CLEAN)
include buildsys/clean.mk
endif
ifeq ($(_BUILD_PHASE),CONFIG)
include buildsys/config.mk
endif
ifeq ($(_BUILD_PHASE),AUTORECONF)
include buildsys/autoreconf.mk
endif
ifeq ($(_BUILD_PHASE),BUILD)
include buildsys/build.mk
endif
ifeq ($(_BUILD_PHASE),RUN)
include buildsys/run.mk
endif
