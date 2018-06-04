# This file ensures that build targets are invoked in the appropriate order,
# and that configuration changes propagate into the build
.PHONY: all
all: mbuild
	@:

# Pull in the configuration file generated in the previous phases
sinclude $(CONFIG)
include $(WORK_DIR)/buildsys/targets.mk
sinclude buildsys/target/$(TARGET)/target.mk

TCS ?= 1

ifndef DISABLE
# include/config.h
CONFIGFILE_HEADER := $(ABUILD_DIR)/include/config.h
$(CONFIGFILE_HEADER): $(CONFIG)
	$(Q)echo $(call BANNER,CFGH,$@)
	$(Q)mkdir -p $(dir $(CONFIGFILE_HEADER))
	$(Q)sed -e '$$a#endif' -e '1i#ifndef CONFIG_H\n#define CONFIG_H' -e 's/#.*//' -e '/^$$/ d' -e 's/\([^=]*\)=\(.*\)/#define \1 \2/g' < $(CONFIG) > $(CONFIGFILE_HEADER) $(SUP2)

# $(XMLDOCS)->include/gen/*.h
GENERATED_HEADERS := $(subst .xml,.h,$(foreach XMLDOC,$(XMLDOCS),$(ABUILD_DIR)/include/gen/$(subst |,$(SPACE),$(lastword $(subst /,$(SPACE),$(subst $(SPACE),|,$(dir $(XMLDOC)))))/$(notdir $(XMLDOC)))))
$(ABUILD_DIR)/include/gen/%.h: $(SPEC_DIR)/%.xml $(XSLTPROC_EXECFILE) $(CONFIG)
	$(Q)echo $(call BANNER,GENH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(XSLTPROC_EXEC) -o $@ buildsys/code.xsl $< $(SUP)

# $(HEADERS)->include/*.h
$(ABUILD_DIR)/include/meos/%: $(SPEC_DIR)/% $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

# Deps
$(ABUILD_DIR)/dep/%.d: %.c $(CONFIGFILE_HEADER) $(GENERATED_HEADERS) $(HEADERS)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(MCFLAGS) $(INC) -MM -MF $@ -c $(<) -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp

$(ABUILD_DIR)/dep/%.d: %.cpp $(CONFIGFILE_HEADER) $(GENERATED_HEADERS) $(HEADERS)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(MCFLAGS) $(INC) -MM -MF $@ -c $(<) -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp

$(ABUILD_DIR)/dep/%.d: %.C $(CONFIGFILE_HEADER) $(GENERATED_HEADERS) $(HEADERS)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(MCFLAGS) $(INC) -MM -MF $@ -c $(<) -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp

# Objects
$(ABUILD_DIR)/obj/%.o: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call CHECKINDENT,$(<))
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(MCFLAGS) -D_PROCESSOR=$(CPU) -c $(<) -o $@

$(ABUILD_DIR)/obj/%.o: %.cpp
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call CHECKINDENT,$(<))
	$(Q)echo $(call BANNER,CXX,$(<))
	$(Q)cd $(dir $@);$(CXX) $(MCFLAGS) -D_PROCESSOR=$(CPU) -c $(<) -o $@

$(ABUILD_DIR)/obj/%.o: %.C
	$(Q)mkdir -p $(dir $@)
	$(Q)$(call CHECKINDENT,$(<))
	$(Q)echo $(call BANNER,CXX,$(<))
	$(Q)cd $(dir $@);$(CXX) $(MCFLAGS) -D_PROCESSOR=$(CPU) -c $(<) -o $@

$(ABUILD_DIR)/obj/%.o: %.s
	$(Q)mkdir -p $(dir $@)
	$(Q)$(Q)$(call CHECKSINDENT,$(<))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(MCFLAGS) -D_PROCESSOR=$(CPU) -c $(<) -o $@

$(ABUILD_DIR)/obj/%.o: %.S
	$(Q)mkdir -p $(dir $@)
	$(Q)$(Q)$(call CHECKSINDENT,$(<))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(MCFLAGS) -D_PROCESSOR=$(CPU) -c $(<) -o $@

# Per TC
define TCBUILDRULE
# Deps
$$(ABUILD_DIR)/dep/pertc_$(CPU)_$(1)/%.d: %.c $$(CONFIGFILE_HEADER) $$(GENERATED_HEADERS) $$(HEADERS)
	$$(Q)echo $$(call BANNER,DEP,$$(<))
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(CC) $$(MCFLAGS) $$(INC) -MM -MF $$@ -c $$(<) -o $$@
	$$(Q)mv -f $$@ $$@.tmp
	$$(Q)sed -e 's|.*:|$$*.o:|' < $$@.tmp > $$@
	$$(Q)rm -f $$@.tmp

$$(ABUILD_DIR)/dep/pertc_$(CPU)_$(1)/%.d: %.cpp $$(CONFIGFILE_HEADER) $$(GENERATED_HEADERS) $$(HEADERS)
	$$(Q)echo $$(call BANNER,DEP,$$(<))
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(CC) $$(MCFLAGS) $$(INC) -MM -MF $$@ -c $$(<) -o $$@
	$$(Q)mv -f $$@ $$@.tmp
	$$(Q)sed -e 's|.*:|$$*.o:|' < $$@.tmp > $$@
	$$(Q)rm -f $$@.tmp

$$(ABUILD_DIR)/dep/pertc_$(CPU)_$(1)/%.d: %.C $$(CONFIGFILE_HEADER) $$(GENERATED_HEADERS) $$(HEADERS)
	$$(Q)echo $$(call BANNER,DEP,$$(<))
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(CC) $$(MCFLAGS) $$(INC) -MM -MF $$@ -c $$(<) -o $$@
	$$(Q)mv -f $$@ $$@.tmp
	$$(Q)sed -e 's|.*:|$$*.o:|' < $$@.tmp > $$@
	$$(Q)rm -f $$@.tmp

# Objects
$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%.o: %.c
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(call CHECKINDENT,$$(<))
	$$(Q)echo $$(call BANNER,CC,$$(<))
	$$(Q)cd $$(dir $$@);$$(CC) $$(MCFLAGS) -D_PROCESSOR=$(CPU) -c $$(<) -o $$@

$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%.o: %.cpp
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(call CHECKINDENT,$$(<))
	$$(Q)echo $$(call BANNER,CXX,$$(<))
	$$(Q)cd $$(dir $$@);$$(CXX) $$(MCFLAGS) -D_PROCESSOR=$(CPU) -c $$(<) -o $$@

$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%.o: %.C
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(call CHECKINDENT,$$(<))
	$$(Q)echo $$(call BANNER,CXX,$$(<))
	$$(Q)cd $$(dir $$@);$$(CXX) $$(MCFLAGS) -D_PROCESSOR=$(CPU) -c $$(<) -o $$@

$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%.o: %.s
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(Q)$$(call CHECKSINDENT,$$(<))
	$$(Q)echo $$(call BANNER,AS,$$(<))
	$$(Q)cd $$(dir $$@);$$(CC) $$(MCFLAGS) -D_PROCESSOR=$(CPU) -c $$(<) -o $$@

$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%.o: %.S
	$$(Q)mkdir -p $$(dir $$@)
	$$(Q)$$(Q)$$(call CHECKSINDENT,$$(<))
	$$(Q)echo $$(call BANNER,AS,$$(<))
	$$(Q)cd $$(dir $$@);$$(CC) $$(MCFLAGS) -D_PROCESSOR=$(CPU) -c $$(<) -o $$@

# TC library
$$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1).a: $(patsubst $(ABUILD_DIR)/obj/%,$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%,$(PTCOBJ))
	$$(Q)echo $$(call BANNER,PTC,$$(<))
	$$(Q)mkdir -p $(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/
	$$(Q)$$(AR) rus $$@ $(patsubst $(ABUILD_DIR)/obj/%,$(ABUILD_DIR)/obj/pertc_$(CPU)_$(1)/%,$(PTCOBJ)) $(SUP)
	$$(Q)$$(NM) $$@|sed -rn "s/^[[:xdigit:]]+[[:space:]]+[^Un][[:space:]]+(.*)/\1 pertc_$(CPU)_$(1)_\1/p"|sort|uniq > $$@.rename
	$$(Q)$$(OBJCOPY) --redefine-syms=$$@.rename $$@

endef
$(foreach TC, $(shell seq $(TCS)), $(eval $(call TCBUILDRULE,$(TC))))

# DTB
DTSI := $(ABUILD_DIR)/obj/$(notdir $(ELF)).dtsi
$(DTSI): $(DTS) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,BSPI,$<)
	$(Q)$(CC) -E -nostdinc -undef -D__DTS__ -x assembler-with-cpp -I $(dir %<) $< -o $@

DTB := $(ABUILD_DIR)/obj/$(notdir $(ELF)).dtb
$(DTB): $(DTSI) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTC,$<)
	$(Q)$(DTC_TOOL) -o $@ -O dtb $<

DTB_C := $(ABUILD_DIR)/obj/$(notdir $(ELF)).dtb.c
$(DTB_C): $(DTB) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTWC,$<)
	$(Q)$(DTWALK_TOOL) $(WORK_DIR)/buildsys/target/$(TARGET)/walks/c $< -Dprocessor=$(CPU) $(WALK_VARS) > $@

DTB_O := $(ABUILD_DIR)/obj/$(notdir $(ELF)).dtb.o
$(DTB_O): $(DTB_C) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTCC,$(<))
	$(Q)cd $(dir $@);$(CC) $(MCFLAGS) -c $(<) -o $@
ifdef OBJ
OBJ += $(DTB_O)
endif

# LD script
LD_SCRIPT := $(ABUILD_DIR)/obj/$(notdir $(ELF)).ld
ifdef MINUST
MINUST2 := -DT=$(MINUST)
else
MINUST2 :=
endif
$(LD_SCRIPT): $(DTB) $(MAKEFILE_LIST) $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,DTWL,$<)
	$(Q)$(DTWALK_TOOL) $(WORK_DIR)/buildsys/target/$(TARGET)/walks/ld $< -Dprocessor=$(CPU) $(MINUST2) $(WALK_VARS) > $@

# ELF
ifdef ELF
ABDELF:=$(ABUILD_DIR)/$(ELF)
endif
$(ABDELF): $(OBJ) $(PTCOBJ) $(foreach TC, $(shell seq $(TCS)), $(if $(PTCOBJ),$(ABUILD_DIR)/obj/pertc_$(CPU)_$(TC).a,)) $(LD_SCRIPT)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,LD,$@)
	$(Q)$(CXX) $(OBJ) -Wl,--whole-archive $(foreach TC, $(shell seq $(TCS)), $(if $(PTCOBJ),$(ABUILD_DIR)/obj/pertc_$(CPU)_$(TC).a,))	 -Wl,--no-whole-archive @$(LD_SCRIPT) $(MLFLAGS) -o $@


# Mark build products as non-deleted intermediates
.SECONDARY: $(OBJ)
.SECONDARY: $(PTCOBJ)

# Make objects depend on config
$(OBJ): $(MAKEFILE_LIST) $(CONFIG)
$(PTCOBJ): $(MAKEFILE_LIST) $(CONFIG)

.PHONY: docs
docs: $(DOCS)

# Calculate the next CPU number for chained builds
PLUSONE := $(shell expr $(CPU) + 1)

# Put it all together and chain the next build
.PHONY: mbuild
ifdef OVERRIDEBUILD
mbuild: $(OVERRIDEBUILD)
else
mbuild: $(CONFIGFILE_HEADER) $(GENERATED_HEADERS) $(HEADERS) $(DEPFILES) $(OVERRIDEBUILD) $(ABDELF) $(DOCS) $(RUNDEP)
	$(Q)$(if $(SRC$(PLUSONE))$(ASMSRC$(PLUSONE)),$(MAKE) --no-print-directory _BUILD_PHASE=BUILD CPU=$(PLUSONE) $(call TARGET_PROPAGATE) $(MAKECMDGOALS),)
endif

# Chain on to run. Make sure we're built!
.PHONY: run
run: mbuild
	$(Q)$(if $(SRC$(PLUSONE))$(ASMSRC$(PLUSONE)),,$(MAKE) --no-print-directory _BUILD_PHASE=RUN CPU=$(PLUSONE) $(call TARGET_PROPAGATE) $(MAKECMDGOALS))
else
%::
	@:
endif
