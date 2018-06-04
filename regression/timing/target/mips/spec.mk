ifdef CONFIG_ARCH_MIPS_BASELINE
include $(SPEC_DIR)/target/mips/baseline/spec.mk
endif
ifdef CONFIG_ARCH_MIPS_GIC
include $(SPEC_DIR)/target/mips/gic/spec.mk
endif
ifdef CONFIG_ARCH_MIPS_PIC
include $(SPEC_DIR)/target/mips/pic32/spec.mk
endif
