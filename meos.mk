####(C)2010###############################################################
#
# Copyright (C) 2010 MIPS Tech, LLC
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
####(C)2010###############################################################
#
#   Description:	MEOS Makefile
#
##########################################################################

###############################################################################
# Version identification:
#
# MAJOR_ID - increment when major incompatible API or behavioural changes are
#            released
# MINOR ID - increment when minor compatible API changes or bugfixes are
#            released. Reset to 0 when the above changes
# ID_QUALIFIER - increment as appropriate through the following sequence. Reset
#                to X when any of the above change
#                X (not a release),
#                A (Alpha),
#                B (Beta),
#                RC (Release Candidate),
#                MR (Manufacturing Release)
# ID_PATCH -     Increment when anything changes, reset to 0 when any of the
#                above are incremented.
#
###############################################################################
MAJOR_ID = 3
MINOR_ID = 0
ID_QUALIFIER = MR
ID_PATCH = 0

# Version
ifeq ($(ID_QUALIFIER), MR)
 ifeq ($(ID_PATCH), 0)
  VERSION := $(MAJOR_ID).$(MINOR_ID)
  SYMVER := MEOS_VERSION_SYMBOL_$(MAJOR_ID)_$(MINOR_ID)
 else
  VERSION := $(MAJOR_ID).$(MINOR_ID)p$(ID_PATCH)
  SYMVER := MEOS_VERSION_SYMBOL_$(MAJOR_ID)_$(MINOR_ID)p$(ID_PATCH)
 endif
else
  VERSION := $(MAJOR_ID).$(MINOR_ID)$(ID_QUALIFIER)$(ID_PATCH)
  SYMVER := MEOS_VERSION_SYMBOL_$(MAJOR_ID)_$(MINOR_ID)$(ID_QUALIFIER)$(ID_PATCH)
endif

# CONFIG VOODOO START

# This allows us to modify the configuration file before including it

#
# Utility functions
#

# $(call UPPER,<string>): Convert <string> to uppercase
UPPER=$(strip $(subst a,A,$(subst b,B,$(subst c,C,$(subst d,D,$(subst e,E,\
      $(subst f,F,$(subst g,G,$(subst h,H,$(subst i,I,$(subst j,J,$(subst k,K,\
      $(subst l,L,$(subst m,M,$(subst n,N,$(subst o,O,$(subst p,P,$(subst q,Q,\
      $(subst r,R,$(subst s,S,$(subst t,T,$(subst u,U,$(subst v,V,$(subst w,W,\
      $(subst x,X,$(subst y,Y,$(subst z,Z,$(1))))))))))))))))))))))))))))
LOWER=$(strip $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,\
      $(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,\
      $(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,\
      $(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,\
      $(subst X,x,$(subst Y,y,$(subst Z,z,$(1))))))))))))))))))))))))))))

SPACE:=
SPACE+=
GREEN=[32m
NORMAL=[39m
BANNER="$(GREEN)[$(NORMAL)$(1)$(GREEN)]$(NORMAL)	$2"
ifndef V
SUP=>$(ABUILD_DIR)/.lasterr 2>&1 || (cat $(ABUILD_DIR)/.lasterr; false;)
SUP2=2>$(ABUILD_DIR)/.lasterr || (cat $(ABUILD_DIR)/.lasterr; false;)
Q=@
else
Q=
endif
ADDTEST=$(if $(CONFIG_TEST_$(notdir $(strip $(1)))),$(strip $(1)))
ADDDEMO=$(if $(CONFIG_DEMO_$(notdir $(strip $(1)))),$(strip $(1)))

# Working directory
WORK_DIR = $(CURDIR)
MEOS_SRC ?= $(WORK_DIR)

# Where to store the generated files
BUILD_DIR ?= ./build
ABUILD_DIR = $(abspath $(shell echo $(BUILD_DIR)))
TEST_DIR ?= $(ABUILD_DIR)/tests
DEMO_DIR ?= $(ABUILD_DIR)/demos

# Configuration file
CONFIG ?= $(ABUILD_DIR)/meos.cfg

define DEFCONFIG
.PHONY: $(1)_defconfig
$(1)_defconfig: $(KCONFIG_EXECFILE)
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cp -f $(2) $(CONFIG)
	$(Q)chmod gou+w $(CONFIG)
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_OLDEXECFILE) --olddefconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi
endef

## 1: Start submaking process
ifeq ($(_MEOS_MAGIC_DO),)

.PHONY: _internal
_internal: base
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)$(MAKE) --no-print-directory _MEOS_MAGIC_DO=CLEAN $(filter clean cleaner testclean democlean %_test,$(MAKECMDGOALS)) silent
	$(Q)$(MAKE) --no-print-directory _MEOS_MAGIC_DO=CONFIG $(filter oldconfig olddefconfig randconfig config menuconfig %defconfig %vagrant %_test,$(MAKECMDGOALS))
	$(if $(filter clean cleaner testclean democlean oldconfig olddefconfig randconfig config menuconfig %defconfig %vagrant,$(MAKECMDGOALS)),$(if $(filter-out clean cleaner testclean democlean oldconfig olddefconfig randconfig config menuconfig %defconfig %vagrant,$(MAKECMDGOALS)),$(Q)$(MAKE) --no-print-directory _MEOS_MAGIC_DO=AUTORECONF $(filter-out clean cleaner testclean democlean oldconfig olddefconfig randconfig config menuconfig %defconfig %vagrant,$(MAKECMDGOALS)),@:),$(Q)$(MAKE) --no-print-directory _MEOS_MAGIC_DO=AUTORECONF $(filter-out clean cleaner testclean democlean oldconfig olddefconfig randconfig config menuconfig %defconfig %vagrant,$(MAKECMDGOALS)))

.PHONY: $(firstword $(MAKECMDGOALS))
$(firstword $(MAKECMDGOALS)): _internal

%::
	@:

include buildsys/tools.mk

endif

## 2: Handle clean targets
ifeq ($(_MEOS_MAGIC_DO),CLEAN)
.PHONY: all
all:
	@:

# Ensure variables for clean targets are populated
sinclude $(CONFIG)
AR = $(CONFIG_TOOLS_AR)
CC = $(CONFIG_TOOLS_CC)
CXX = $(CONFIG_TOOLS_CXX)
OBJCOPY = $(CONFIG_TOOLS_OBJCOPY)
DTWALK_TOOL = $(ABUILD_DIR)/bin/dtwalk
SRC :=
ASMSRC :=
CLEAN_EXTRA :=
include buildsys/tools.mk
include targets/*/module.mk
MODULES := boards debug mq ipm irq mem tmr kernel ctx dqueues lists trees rings support
include $(patsubst %, %/module.mk,$(MODULES))
include regression/module.mk
COBJ := $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(filter %.c,$(SRC)))
CPPOBJ := $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(filter %.cpp,$(SRC)))
SOBJ := $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(filter %.s,$(ASMSRC))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(filter %.S,$(ASMSRC)))
OBJ := $(COBJ) $(CPPOBJ) $(SOBJ) $(XOBJ)
DEPFILES = $(patsubst $(ABUILD_DIR)/obj/%.o,$(ABUILD_DIR)/dep/%.d,$(COBJ) $(CPPOBJ))
CONFIG_TOOL = $(ABUILD_DIR)/bin/meos-config
MEOS_TOOLS = $(CONFIG_TOOL) $(DTWALK_TOOL)
GENERATED_HEADERS += $(subst .xml,.h,$(foreach XMLDOC,$(XMLDOCS),$(ABUILD_DIR)/include/gen/meos/$(subst |,$(SPACE),$(lastword $(subst /,$(SPACE),$(subst $(SPACE),|,$(dir $(XMLDOC)))))/$(notdir $(XMLDOC)))))
COMPAT_MEOS_HEADER = $(ABUILD_DIR)/include/MeOS.h
MEOS_HEADER = $(A(ABUILD_DIR)/include/MEOS.h
CONFIGFILE_HEADER = $(ABUILD_DIR)/include/meos/config.h
INTTYPES_HEADER = $(ABUILD_DIR)/include/meos/inttypes.h
TARGET_LIBRARY_SHORT = $(ABUILD_DIR)/lib/libMEOS.a
SUBDIRS += $(ABUILD_DIR)/bin $(ABUILD_DIR)/include $(ABUILD_DIR)/include/meos $(ABUILD_DIR)/dep/ $(ABUILD_DIR)/obj/ $(ABUILD_DIR)/lib/ $(ABUILD_DIR)/doc/ $(ABUILD_DIR)/wrap

.PHONY: clean
clean: testclean democlean
	$(Q)echo $(call BANNER,CLEAN,$(@))
	$(Q)-rm -f $(UNILOG)
	$(Q)-rm -f $(DEPFILES)
	$(Q)-rm -fr $(ABUILD_DIR)/dep
	$(Q)-rm -fr $(ABUILD_DIR)/wrap
	$(Q)-rm -f $(ABUILD_DIR)/doc/manual.html
	$(Q)-rm -f $(ABUILD_DIR)/doc/manual.pdf
	$(Q)-rm -f $(MEOS_TOOLS)
	$(Q)-rm -f $(GENERATED_HEADERS)
	$(Q)-rm -f $(COMPAT_MEOS_HEADER)
	$(Q)-rm -f $(MEOS_HEADER)
	$(Q)-rm -f $(CONFIGFILE_HEADER)
	$(Q)-rm -f $(INTTYPES_HEADER)
	$(Q)-rm -fr $(ABUILD_DIR)/include
	$(Q)-rm -f $(TARGET_LIBRARY_SHORT)
	$(Q)-rm -f $(OBJ)
	$(Q)-rm -fr $(ABUILD_DIR)/obj
	$(Q)-rm -fr $(ABUILD_DIR)/walks
	$(Q)-rm -fr $(CLEAN_EXTRA)
	$(Q)-rmdir --ignore-fail-on-non-empty $(SUBDIRS) >/dev/null 2>&1 || true

.PHONY: cleaner
cleaner: clean
	$(Q)-rm -fr $(TOOLS_DIR)

.PHONY: testclean
testclean:
	$(Q)-rm -rf $(TEST_DIR)

.PHONY: democlean
democlean:
	$(Q)-rm -rf $(DEMO_DIR)

.PHONY: %_test
%_test: clean
	@:

%::
	@:

endif

## 3: Handle config targets
ifeq ($(_MEOS_MAGIC_DO),CONFIG)
.PHONY: all
all:
	@:

sinclude $(CONFIG)
include buildsys/tools.mk
include targets/*/module.mk

CONFIG_VER_EXPECTED=mainmenu "MEOS $(VERSION) Configuration"
CONFIG_VER_FOUND=$(shell grep ^mainmenu $(WORK_DIR)/buildsys/Kconfig)

.PHONY: config
config:	$(KCONFIG_EXECFILE)
	$(Q)if [ '$(CONFIG_VER_EXPECTED)' != '$(CONFIG_VER_FOUND)' ]; then chmod u+w $(WORK_DIR)/buildsys/Kconfig; sed -e 's/^mainmenu.*/$(CONFIG_VER_EXPECTED)/g' <$(WORK_DIR)/buildsys/Kconfig >$(WORK_DIR)/buildsys/Kconfig.new; mv $(WORK_DIR)/buildsys/Kconfig.new $(WORK_DIR)/buildsys/Kconfig; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_EXEC) $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)echo "[3A                                                         "
	$(Q)echo "                                                         [3A"
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: menuconfig
menuconfig: config

.PHONY: oldconfig
oldconfig: $(KCONFIG_EXECFILE)
	$(Q)if [ '$(CONFIG_VER_EXPECTED)' != '$(CONFIG_VER_FOUND)' ]; then chmod u+w $(WORK_DIR)/buildsys/Kconfig; sed -e 's/^mainmenu.*/$(CONFIG_VER_EXPECTED)/g' <$(WORK_DIR)/buildsys/Kconfig >$(WORK_DIR)/buildsys/Kconfig.new; mv $(WORK_DIR)/buildsys/Kconfig.new $(WORK_DIR)/buildsys/Kconfig; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)if [ ! -f $(CONFIG) ]; then echo No configuration file! Please see the \'Configuration\' section of the manual!; false; fi
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_OLDEXECFILE) --oldconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: olddefconfig
olddefconfig: $(KCONFIG_EXECFILE)
	$(Q)if [ '$(CONFIG_VER_EXPECTED)' != '$(CONFIG_VER_FOUND)' ]; then chmod u+w $(WORK_DIR)/buildsys/Kconfig; sed -e 's/^mainmenu.*/$(CONFIG_VER_EXPECTED)/g' <$(WORK_DIR)/buildsys/Kconfig >$(WORK_DIR)/buildsys/Kconfig.new; mv $(WORK_DIR)/buildsys/Kconfig.new $(WORK_DIR)/buildsys/Kconfig; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)if [ ! -f $(CONFIG) ]; then echo No configuration file! Please see the \'Configuration\' section of the manual!; false; fi
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_OLDEXECFILE) --olddefconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: randconfig
randconfig:	$(KCONFIG_EXECFILE)
	$(Q)if [ '$(CONFIG_VER_EXPECTED)' != '$(CONFIG_VER_FOUND)' ]; then chmod u+w $(WORK_DIR)/buildsys/Kconfig; sed -e 's/^mainmenu.*/$(CONFIG_VER_EXPECTED)/g' <$(WORK_DIR)/buildsys/Kconfig >$(WORK_DIR)/buildsys/Kconfig.new; mv $(WORK_DIR)/buildsys/Kconfig.new $(WORK_DIR)/buildsys/Kconfig; fi
	$(Q)mkdir -p $(ABUILD_DIR)
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_OLDEXECFILE) --randconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

%_defconfig:
	$(Q)echo No such configuration!; false

.PHONY: %_test
%_test: %_defconfig
	@:

%::
	@:

endif

## 4: Automatically invoke oldconfig if out of date
ifeq ($(_MEOS_MAGIC_DO),AUTORECONF)
.PHONY: _internal
_internal: $(CONFIG)
	$(Q)$(MAKE) --no-print-directory _MEOS_MAGIC_DO=BUILD $(MAKECMDGOALS)

sinclude $(CONFIG)
include buildsys/tools.mk

CONFIG_VER_EXPECTED=mainmenu "MEOS $(VERSION) Configuration"
CONFIG_VER_FOUND=$(shell grep ^mainmenu $(WORK_DIR)/buildsys/Kconfig)

$(CONFIG): $(KCONFIG_EXECFILE) buildsys/Kconfig regression/Kconfig targets/*/Kconfig*
	$(Q)if [ '$(CONFIG_VER_EXPECTED)' != '$(CONFIG_VER_FOUND)' ]; then chmod u+w $(WORK_DIR)/buildsys/Kconfig; sed -e 's/^mainmenu.*/$(CONFIG_VER_EXPECTED)/g' <$(WORK_DIR)/buildsys/Kconfig >$(WORK_DIR)/buildsys/Kconfig.new; mv $(WORK_DIR)/buildsys/Kconfig.new $(WORK_DIR)/buildsys/Kconfig; fi
	$(Q)if [ ! -f $(CONFIG) ]; then echo No configuration file! Please see the \'Configuration\' section of the manual!; false; fi
	$(Q)cd $(ABUILD_DIR); WORK_DIR=$(WORK_DIR) KCONFIG_CONFIG=$(CONFIG) $(KCONFIG_OLDEXECFILE) --oldconfig $(WORK_DIR)/buildsys/Kconfig $(SUP2)
	$(Q)if [ -f $(CONFIG) ]; then touch $(CONFIG); fi

.PHONY: $(firstword $(MAKECMDGOALS))
$(firstword $(MAKECMDGOALS)): _internal

%:: ;

endif

## 5: Normal build process
ifeq ($(_MEOS_MAGIC_DO),BUILD)
# CONFIG VOODOO END

PRIVATE_CFLAGS += -DMEOS_VERSION=$(SYMVER)

.PHONY: all
# By default, build meos
all: meos

.PHONY: %_test
%_test: test
	@:

sinclude $(CONFIG)
include buildsys/tools.mk

# Utilises an 'inclusive' make system whereby fragments of Makefiles are pulled
# in from all the module directories to build a 'complete' Makefile that covers
# the whole project.
# Files generated are in the build directory and segregated into trees by their
# CORE type.
#

###############################################################################
# Targets:
#
# config:
#  Shows a menu allowing MEOS to be configured
#
# all/(default):
#  Builds the configured MEOS
#
# clean:
#  Removes all generated files
#
# testbuild:
#  Builds, but does not run, all tests for the currently set CORE
#
# test:
#  Builds and runs all tests for the currently set CORE and BOARD and TARGET
#
# testclean:
#  Removes whole built test directory
#
# democlean:
#  Removes whole built demo directory
#
###############################################################################

#
# Variables
#

# This file
MAKEFILE_NAME := $(lastword $(MAKEFILE_LIST))

# Configuration header
CONFIGFILE_HEADER = $(ABUILD_DIR)/include/meos/config.h
INTTYPES_HEADER = $(ABUILD_DIR)/include/meos/inttypes.h

# Include configuration
sinclude $(CONFIG)

# Tools
AR = $(CONFIG_TOOLS_AR)
CC = $(CONFIG_TOOLS_CC)
CXX = $(CONFIG_TOOLS_CXX)
OBJCOPY = $(CONFIG_TOOLS_OBJCOPY)
DTWALK_TOOL = $(ABUILD_DIR)/bin/dtwalk

#
# Tools setup
#
INDENT = $(INDENT_EXEC) -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

# The sub-directories that contain Makefile fragments
MODULES := boards debug mq ipm irq mem tmr kernel ctx dqueues lists trees rings support

# Include paths to cover all sub-parts
INC ?= -I $(WORK_DIR)/ -I $(ABUILD_DIR)/include/ -I $(ABUILD_DIR)/include/gen/

# These gets populated by the sub-includes
SRC :=
ASMSRC :=
CLEAN_EXTRA :=

# The list of things that need building, filled out by the fragments.
# Some tests are generic, other are core specific
TEST_ALL_TARGETS :=
TEST_$(CORE)_TARGETS :=
TEST_OBJS :=
TESTS :=
DEMO_ALL_TARGETS :=
DEMO_TARGETS :=
DEMO_$(CORE)_TARGETS :=
DEMO_OBJS :=
DEMOS :=

# Pull in the fragments from the targets/subdirs
include targets/*/module.mk

# Pull in the fragments from the modules/subdirs
include $(patsubst %, %/module.mk,$(MODULES))

# Build a list of directories we need to create before we start building
PYTHON_VERSION = $(shell python -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
SUBDIRS += $(ABUILD_DIR)/bin $(ABUILD_DIR)/include $(ABUILD_DIR)/include/meos $(ABUILD_DIR)/dep/ $(ABUILD_DIR)/obj/ $(ABUILD_DIR)/lib/ $(ABUILD_DIR)/doc/
ifdef CONFIG_DEBUG_WRAPPER
SUBDIRS += $(ABUILD_DIR)/wrap
endif

COMPAT_MEOS_HEADER = $(ABUILD_DIR)/include/MeOS.h
MEOS_HEADER = $(ABUILD_DIR)/include/MEOS.h
GENERATED_HEADERS += $(subst .xml,.h,$(foreach XMLDOC,$(XMLDOCS),$(ABUILD_DIR)/include/gen/meos/$(subst |,$(SPACE),$(lastword $(subst /,$(SPACE),$(subst $(SPACE),|,$(dir $(XMLDOC)))))/$(notdir $(XMLDOC)))))
INSTALLED_HEADERS += $(addprefix $(ABUILD_DIR)/include/meos/,$(HEADERS))

# Determine the target objects
# Mark them as non-deleted intermediates
COBJ := $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(filter %.c,$(SRC)))
CPPOBJ := $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(filter %.cpp,$(SRC)))
SOBJ := $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(filter %.s,$(ASMSRC))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(filter %.S,$(ASMSRC)))
OBJ := $(COBJ) $(CPPOBJ) $(SOBJ) $(XOBJ)
.SECONDARY: $(OBJ)

# Work out the dependency files
DEPFILES = $(patsubst $(ABUILD_DIR)/obj/%.o,$(ABUILD_DIR)/dep/%.d,$(COBJ) $(CPPOBJ))

#
# Ultimate targets
#
ifdef CONFIG_DEBUG_WRAPPER
WRAPPER_MODE :=

ifdef CONFIG_DEBUG_DIAGNOSTICS_PREPOST
WRAPPER_MODE += c
endif

ifdef CONFIG_DEBUG_EXTRA_PARANOIA
WRAPPER_MODE += v
endif

ifdef CONFIG_DEBUG_TRACE_API_SOFT
WRAPPER_MODE += t
endif

ifdef CONFIG_DEBUG_TRACE_API_HARD
WRAPPER_MODE += h
endif

ifdef CONFIG_DEBUG_PER_OBJECT_TRACE
WRAPPER_MODE += o
endif

WRAPPER_MODULES :=
NOT_WRAPPER_MODULES :=
ifdef CONFIG_DEBUG_WRAP_CTX
WRAPPER_MODULES += ctx
else
NOT_WRAPPER_MODULES += ctx
endif
ifdef CONFIG_DEBUG_WRAP_DQ
WRAPPER_MODULES += dq
else
NOT_WRAPPER_MODULES += dq
endif
ifdef CONFIG_DEBUG_WRAP_IPM
WRAPPER_MODULES += ipm
else
ifdef CONFIG_FEATURE_VRINGS
NOT_WRAPPER_MODULES += ipm
endif
endif
ifdef CONFIG_DEBUG_WRAP_IRQ
WRAPPER_MODULES += irq
else
NOT_WRAPPER_MODULES += irq
endif
ifdef CONFIG_DEBUG_WRAP_KRN
WRAPPER_MODULES += krn
else
NOT_WRAPPER_MODULES += krn
endif
ifdef CONFIG_DEBUG_WRAP_LST
WRAPPER_MODULES += lst
else
NOT_WRAPPER_MODULES += lst
endif
ifdef CONFIG_DEBUG_WRAP_MEM
WRAPPER_MODULES += mem
else
NOT_WRAPPER_MODULES += mem
endif
ifdef CONFIG_DEBUG_WRAP_RING
WRAPPER_MODULES += ring
else
NOT_WRAPPER_MODULES += ring
endif
ifdef CONFIG_DEBUG_WRAP_TMR
WRAPPER_MODULES += tmr
else
NOT_WRAPPER_MODULES += tmr
endif
ifdef CONFIG_DEBUG_WRAP_TRE
WRAPPER_MODULES += tre
else
NOT_WRAPPER_MODULES += tre
endif
ifeq ($(WRAPPER_MODULES),)
WRAPPER_MODULES := $(NOT_WRAPPER_MODULES)
endif

$(ABUILD_DIR)/wrap/symbol.rename: doc/manual.xml $(XMLDOCS) $(XSLTPROC_EXECFILE)
	$(Q)$(XSLTPROC_EXEC) --stringparam modules "$(WRAPPER_MODULES)" --stringparam fns "$(CONFIG_DEBUG_WRAPPER_FNS)" --xinclude -o $@ buildsys/symwrap.xsl $< $(SUP)

$(ABUILD_DIR)/wrap/wrap.c: doc/manual.xml $(ABUILD_DIR)/wrap/libUMEOS.a($(OBJ)) $(XMLDOCS) $(XSLTPROC_EXECFILE) $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(CONFIGFILE_HEADER) $(INTTYPES_HEADER)
	$(Q)$(XSLTPROC_EXEC) --stringparam modules "$(WRAPPER_MODULES)" --stringparam fns "$(CONFIG_DEBUG_WRAPPER_FNS)" --stringparam mode "$(WRAPPER_MODE)" --xinclude -o $@ buildsys/debug.xsl $< $(SUP)

$(ABUILD_DIR)/wrap/libWMEOS.a: $(ABUILD_DIR)/wrap/libUMEOS.a($(OBJ)) $(ABUILD_DIR)/wrap/symbol.rename $(ABUILD_DIR)/wrap/wrap.c
	$(Q)$(OBJCOPY) --redefine-syms=$(ABUILD_DIR)/wrap/symbol.rename $(ABUILD_DIR)/wrap/libUMEOS.a $@ $(SUP)

$(ABUILD_DIR)/lib/libMEOS.a: $(ABUILD_DIR)/wrap/libWMEOS.a $(ABUILD_DIR)/wrap/wrap.o
	$(Q)echo $(call BANNER,WRAP,$(<))
	$(Q)mkdir -p $(dir $(@))
	$(Q)cp $(ABUILD_DIR)/wrap/libWMEOS.a $@
	$(Q)$(AR) rv $@ $(ABUILD_DIR)/wrap/wrap.o $(SUP)

TARGET_LIBRARY = $(ABUILD_DIR)/lib/libMEOS.a
else
TARGET_LIBRARY = $(ABUILD_DIR)/lib/libMEOS.a($(OBJ))
endif
# Need the short (non a(obj)) names for packaging and cleaning
TARGET_LIBRARY_SHORT = $(ABUILD_DIR)/lib/libMEOS.a

CONF_TOOL = $(ABUILD_DIR)/bin/kconfig-conf
MCONF_TOOL = $(ABUILD_DIR)/bin/kconfig-mconf
CONFIG_TOOL = $(ABUILD_DIR)/bin/meos-config
INDENT_TOOL = $(ABUILD_DIR)/bin/indent
DTC_TOOL = $(ABUILD_DIR)/bin/dtc
GNUPLOT_TOOL = $(ABUILD_DIR)/bin/gnuplot
MEOS_TOOLS = $(CONFIG_TOOL) $(DTWALK_TOOL) $(CONF_TOOL) $(MCONF_TOOL) $(INDENT_TOOL) $(DTC_TOOL) $(GNUPLOT_TOOL)
DOCS = $(ABUILD_DIR)/doc/manual.html $(ABUILD_DIR)/doc/manual.pdf

.PHONY: docs
docs: $(DOCS)

.PHONY: meos
# By default, build the libraries
meos: base $(CONFIGFILE_HEADER) $(MISC_PREP) $(INTTYPES_HEADER) $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(DEPFILES) $(EXTRA_TARGETS) $(TARGET_LIBRARY) $(DOCS) $(MEOS_TOOLS)

$(CONF_TOOL): $(KCONFIG_OLDEXECFILE)
	$(Q)ln -fs $< $@

$(MCONF_TOOL): $(KCONFIG_EXECFILE)
	$(Q)ln -fs $< $@

$(INDENT_TOOL): $(INDENT_EXECFILE)
	$(Q)ln -fs $< $@

$(DTC_TOOL): $(DTC_EXECFILE)
	$(Q)ln -fs $< $@

$(GNUPLOT_TOOL): $(GNUPLOT_EXECFILE)
	$(Q)ln -fs $< $@

ifdef CONFIG_ARCH_MIPS_VZ
# Trick uhi32.ld into giving us enough space
# This can be removed when the new hal vector code is integrated.
WALK_VARS+=-DvectorCount=36
endif

$(CONFIG_TOOL): $(WORK_DIR)/buildsys/meos-config.template
	$(Q)echo $(call BANNER,CFGT,$(@))
	$(Q)mkdir -p $(dir $(CONFIG_TOOL))
	$(Q)sed -e 's%WV%$(WALK_VARS)%g' -e 's%CC%$(CC)%g' -e 's%CXX%$(CXX)%g' -e 's%ART%$(AR)%g' -e 's%TT%$(TARGET_NAME)%g' -e 's%PP%$(ABUILD_DIR)%g' -e 's/XX/$(MAJOR_ID)/g' -e 's/YY/$(MINOR_ID)/g' -e 's/ZZ/$(ID_PATCH)/g' -e 's%FF%$(CFLAGS)%g' -e 's%LL%$(LFLAGS)%g' < $< > $@ $(SUP2)
	$(Q)chmod gou+x $@

$(DTWALK_TOOL): $(WORK_DIR)/buildsys/dtwalk.template $(DTC_EXECFILE)
	$(Q)echo $(call BANNER,DTWT,$(@))
	$(Q)mkdir -p $(dir $(DTWALK_TOOL))
	$(Q)sed -e 's%PP%$(DTC_PATH)%g'< $< > $@ $(SUP2)
	$(Q)chmod gou+x $@

$(CONFIGFILE_HEADER): $(CONFIG)
	$(Q)echo $(call BANNER,CFGH,$@)
	$(Q)mkdir -p $(dir $(CONFIGFILE_HEADER))
	$(Q)sed -e '$$a#endif' -e '1i#ifndef CONFIG_H\n#define CONFIG_H' -e 's/#.*//' -e '/^$$/ d' -e 's/\([^=]*\)=\(.*\)/#define \1 \2/g' < $(CONFIG) > $(CONFIGFILE_HEADER) $(SUP2)

$(INTTYPES_HEADER): buildsys/inttypes.h
	$(Q)echo $(call BANNER,ITH,$@)
	$(Q)mkdir -p $(dir $(INTTYPES_HEADER))
	$(Q)cp buildsys/inttypes.h $(INTTYPES_HEADER)

# If the Makefile changes then rebuild all the objects
$(OBJ):	$(MAKEFILE_NAME) $(CONFIG)

.PHONY: docs
docs:	$(DOCS)

#
# Auto generated header rules
#
.PHONY: generated_headers
generated_headers: $(GENERATED_HEADERS)

$(ABUILD_DIR)/include/gen/meos/%.h: %.xml $(XSLTPROC_EXECFILE) $(CONFIG)
	$(Q)echo $(call BANNER,GENH,$(<))
	$(Q)$(XSLTPROC_EXEC) -o $@ buildsys/code.xsl $< $(SUP)

$(ABUILD_DIR)/include/gen/meos/%.h: $(XSLTPROC_EXECFILE) $(CONFIG)
	$(Q)echo $(call BANNER,GNMH,$(filter %$(subst .h,.xml,$(subst |,$(SPACE),$(lastword $(subst /,$(SPACE),$(subst $(SPACE),|,$(dir $@))))/$(notdir $@))),$(XMLDOCS)))
	$(Q)$(XSLTPROC_EXEC) -o $@ buildsys/code.xsl $(filter %$(subst .h,.xml,$(subst |,$(SPACE),$(lastword $(subst /,$(SPACE),$(subst $(SPACE),|,$(dir $@))))/$(notdir $@))),$(XMLDOCS)) $(SUP)

$(ABUILD_DIR)/include/meos/%: % $(CONFIG)
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

.PHONY: subdirs
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(Q)mkdir -p $@

# Rules of how to build the dependencies
$(ABUILD_DIR)/dep/%.d: %.c $(TC_TARGET) $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(CONFIGFILE_HEADER) $(INTTYPES_HEADER)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -MM -MF $@ -c $*.c -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp


$(ABUILD_DIR)/dep/%.d: %.cpp $(TC_TARGET) $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(CONFIGFILE_HEADER) $(INTTYPES_HEADER)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -MM -MF $@ -c $*.cpp -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp

# Rules of how to build the objects, and where they go
$(ABUILD_DIR)/obj/%.o: %.c $(TC_TARGET) $(INDENT_EXEC)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo $(call BANNER,FIXC,$(<));chmod u+w $*.c;$(INDENT) $*.c;$(INDENT) $*.c ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKC,$(<));$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo "$*.c breaks indentation policy: run $(INDENT) $(CURDIR)/$*.c"; false ))
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.c -o $@

$(ABUILD_DIR)/wrap/%.o: $(ABUILD_DIR)/wrap/%.c $(TC_TARGET)
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -c $(ABUILD_DIR)/wrap/$*.c -o $@

$(ABUILD_DIR)/obj/%.o: %.s $(TC_TARGET)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),unexpand $*.s|tr -d "\r"|diff $*.s - > /dev/null || ( echo $(call BANNER,FIXS,$(<));chmod u+w $*.s;unexpand $*.s|tr -d "\r">$*.s.2;mv $*.s.2 $*.s ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKS,$(<));unexpand $*.s|tr -d "\r"|diff $*.s - > /dev/null || ( echo "$*.s breaks indentation policy: run unexpand $(CURDIR)/$*.s|tr -d \"\\r\">out;mv out $(CURDIR)/$*.s"; false ))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.s -o $@

$(ABUILD_DIR)/obj/%.o: %.S $(TC_TARGET)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),unexpand $*.S|tr -d "\r"|diff $*.S - > /dev/null || ( echo $(call BANNER,FIXS,$(<));chmod u+w $*.S;unexpand $*.S|tr -d "\r">$*.S.2;mv $*.S.2 $*.S ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKS,$(<));unexpand $*.S|tr -d "\r"|diff $*.S - > /dev/null || ( echo "$*.S breaks indentation policy: run unexpand $(CURDIR)/$*.S|tr -d \"\\r\">out;mv out $(CURDIR)/$*.S"; false ))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.S -o $@

# Rules of how to check headers
.PHONY: %.h.indent
%.h.indent: %.h $(INDENT_EXEC)
	$(Q)echo $(call BANNER,CHKH,$<)
	$(Q)$(if $(FIX_INDENT),echo $(call BANNER,FIXH,$(<));$(INDENT) $*.h;$(INDENT) $*.h)
	$(Q)$(if $(LIBERAL_INDENT),,$(INDENT) $*.h -st|$(INDENT) -st -|diff $*.h - > /dev/null || ( echo "$*.h breaks indentation policy: run $(INDENT) $(CURDIR)/$*.h"; false ))

.PHONY: %.c.indent
%.c.indent: %.c $(INDENT_EXEC)
	$(Q)echo $(call BANNER,CHKC,$<)
	$(Q)$(if $(FIX_INDENT),echo $(call BANNER,FIXC,$(<));$(INDENT) $*.c;$(INDENT) $*.c)
	$(Q)$(if $(LIBERAL_INDENT),,$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo "$*.c breaks indentation policy: run $(INDENT) $(CURDIR)/$*.c"; false ))

(%.o): %.o
	$(Q)echo $(call BANNER,AR,$<)
	$(Q)mkdir -p $(dir $(@))
	$(Q)$(AR) rv $@ $% $(SUP)

#
# Walks
#
$(ABUILD_DIR)/walks:
	$(Q)mkdir -p $(ABUILD_DIR)/walks/impexp
	$(Q)cp -r $(WORK_DIR)/walks/* $(ABUILD_DIR)/walks/ $(SUP2)
	$(Q)cp -r $(WORK_DIR)/project/buildsys/target/linux/walks $(ABUILD_DIR)/walks/impexp/linux $(SUP2)
	$(Q)cp -r $(WORK_DIR)/project/buildsys/target/mips/walks $(ABUILD_DIR)/walks/impexp/mips $(SUP2)
	$(Q)chmod -R gou+w $(ABUILD_DIR)/walks/ $(SUP)

#
# Auto generated doc rules
#
$(ABUILD_DIR)/doc/manual.html: doc/manual.xml $(XSLTPROC_EXECFILE) $(RST2HTML_EXECFILE)
	$(Q)echo $(call BANNER,HTML,$@)
	$(Q)mkdir -p $(ABUILD_DIR)/doc
	$(Q)$(XSLTPROC_EXEC) --xinclude doc/doc.xsl $< > $@.rst $(SUP2)
	$(Q)$(XSLTPROC_EXEC) --xinclude doc/doc.xsl support/supman.xml > $(ABUILD_DIR)/doc/supman.html.rst $(SUP2)
	$(Q)WORK_DIR=$(WORK_DIR) python doc/cook.py < buildsys/Kconfig > $(ABUILD_DIR)/doc/config.rst $(SUP2)
	$(Q)cp -f doc/rst/*.rst $(dir $@) $(SUP)
	$(Q)chmod gou+w $(dir $@)manual.rst
	$(Q)cd $(dir $@); $(RST2HTML_EXEC) --file-insertion-enabled --exit-status=2 $(notdir $(<:.xml=.rst)) $@ $(SUP)
	$(Q)rm -f $@.rst $(SUP)
	$(Q)rm -f $(ABUILD_DIR)/doc/supman.html.rst $(SUP)
	$(Q)rm -f $(ABUILD_DIR)/doc/config.rst $(SUP)
	$(Q)rm -f $(dir $@)/manual.rst $(SUP)

$(ABUILD_DIR)/doc/%.pdf: $(ABUILD_DIR)/doc/%.html $(XSLTPROC_EXECFILE) $(FOP_EXECFILE) $(DTD_LOCFILE)
	$(Q)echo $(call BANNER,PDF,$@)
	$(Q)XML_CATALOG_FILES="$(DTD_CAT)" $(XSLTPROC_EXEC) --stringparam main-title "MEOS" --stringparam sub-title "User Manual" --stringparam revision "$(VERSION)" --stringparam file-name "$(notdir $@)" --stringparam issue-date "$(shell date)" doc/xhtml2fo.xsl $< > $@.fo
	$(Q)$(FOP_EXEC) $@.fo $@ $(SUP)
	$(Q)rm $@.fo $(SUP)

$(MEOS_HEADER): $(GENERATED_HEADERS) $(MAKEFILE_NAME) $(CONFIG)
	$(Q)echo $(call BANNER,GENH,$@)
	$(Q)echo "#ifndef MEOS_H" > $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_H 1" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_MAJOR_ID $(MAJOR_ID)" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_MINOR_ID $(MINOR_ID)" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_ID_STRING \"$(VERSION)\"" >> $(MEOS_HEADER) $(SUP2)
	$(Q)cat buildsys/header.txt >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#ifdef __cplusplus" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "extern \"C\" {" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#include \"meos/config.h\"" >> $(MEOS_HEADER) $(SUP2);
	$(Q)echo "#include \"meos/inttypes.h\"" >> $(MEOS_HEADER) $(SUP2);
	$(Q)for f in $(subst $(ABUILD_DIR)/include/,,$(filter-out .c,$(GENERATED_HEADERS))) ; do \
		echo "#include \"$${f}\"" >> $(MEOS_HEADER) $(SUP2); \
	done
	$(Q)echo "void BSP_init(void);" >> $(MEOS_HEADER) $(SUP2);
	$(Q)echo "#ifdef __cplusplus" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "}" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif /* not defined MEOS_H */" >> $(MEOS_HEADER) $(SUP2)

$(COMPAT_MEOS_HEADER): $(GENERATED_HEADERS) $(MAKEFILE_NAME) $(CONFIG)
	$(Q)echo $(call BANNER,GENH,$@)
	$(Q)echo "#ifndef MEOS_H" > $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_H 1" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_MAJOR_ID $(MAJOR_ID)" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_MINOR_ID $(MINOR_ID)" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#define MEOS_ID_STRING \"$(VERSION)\"" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)cat buildsys/header.txt >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#ifdef __cplusplus" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "extern \"C\" {" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#include \"meos/config.h\"" >> $(COMPAT_MEOS_HEADER) $(SUP2);
	$(Q)echo "#include \"meos/inttypes.h\"" >> $(COMPAT_MEOS_HEADER) $(SUP2);
	$(Q)for f in $(subst $(ABUILD_DIR)/include/,,$(filter-out .c,$(GENERATED_HEADERS))) ; do \
		echo "#include \"$${f}\"" >> $(COMPAT_MEOS_HEADER) $(SUP2); \
	done
	$(Q)echo "#ifdef __cplusplus" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "}" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(COMPAT_MEOS_HEADER) $(SUP2)
	$(Q)echo "#endif /* not defined MEOS_H */" >> $(COMPAT_MEOS_HEADER) $(SUP2)

include regression/module.mk

# If the Makefile changes then rebuild all the objects
$(TEST_OBJS):	$(MAKEFILE_NAME) $(CONFIG)

.PHONY: testdirs
testdirs: $(TESTDIRS)

$(TESTDIRS):
	$(Q)mkdir -p $@

.PHONY: modfile
modfile:
	$(Q)mkdir -p $(TEST_DIR)
	$(Q)echo "MODULES=$(MODULES)" > $(TEST_DIR)/modules.sh
	$(Q)$(foreach MODULE,$(call UPPER,$(MODULES)),echo export $(MODULE)_SRC=$($(MODULE)_SRC) >> $(TEST_DIR)/modules.sh;)

TEST_TARGETS := $(patsubst %,$(TEST_DIR)/%/test.run,$(ALL_TESTS))
.PHONY: $(TEST_DIR)/%/test.run
$(TEST_DIR)/%/test.run: $(WORK_DIR)/% $(TEST_DIR)/%/test.cfg testdirs modfile
	$(Q)make -f "$(WORK_DIR)/project/Makefile" -C "$(WORK_DIR)/project" CONFIG="$(patsubst %.run,%.cfg,$@)" SPEC_DIR="$(<)" BUILD_DIR="$(patsubst $(WORK_DIR)/%,$(TEST_DIR)/%,$(<))" MEOS_DIR="$(ABUILD_DIR)" CFLAGS="$(PRIVATE_CFLAGS)" UNILOG="$(UNILOG)"

.PRECIOUS: $(TEST_DIR)/%/test.cfg
$(TEST_DIR)/%/test.cfg: $(CONFIG)
	$(Q)mkdir -p $(dir $@)
	$(Q)cp $(CONFIG) $@

.PHONY: testbuild
testbuild: all $(TARGET_LIBRARY) testdirs modfile $(TEST_TARGETS)

ifdef TEST_LOG
.PHONY: $(TEST_LOG)
$(TEST_LOG):
	$(Q)rm -f $(TEST_LOG) $(SUP)
endif

ifdef CONFIG_SIMULATED_TESTING
TEST_ARG_EXTRA?=EMULATE=$(CONFIG_MIPS_MODEL)
endif
ifdef CONFIG_ACQUIRE_FOR_TEST
TEST_ARG_EXTRA?=ACQUIRE=$(CONFIG_MIPS_MODEL)
else
TEST_ARG_EXTRA?=PROBE=$(CONFIG_MIPS_MODEL)
endif

TEST_LOGS := $(patsubst %,$(ABUILD_DIR)/logs/%.log,$(ALL_TESTS))
.PHONY: $(ABUILD_DIR)/logs/%.log
$(ABUILD_DIR)/logs/%.log: $(WORK_DIR)/%
	$(Q)mkdir -p $(dir $@)
	$(Q)make -f "$(WORK_DIR)/project/Makefile" -C "$(WORK_DIR)/project" CONFIG="$(patsubst $(WORK_DIR)/%,$(TEST_DIR)/%, $(<))/test.cfg" SPEC_DIR="$(<)" BUILD_DIR="$(patsubst $(WORK_DIR)/%,$(TEST_DIR)/%, $(<))" MEOS_DIR="$(ABUILD_DIR)" LOG="$@" $(TEST_ARG_EXTRA) run

.PHONY: testbanner
testbanner:
	$(Q)echo $(call BANNER,TEST,$(<))
	$(Q)-rm -f $(TEST_LOGS) $(SUP)

.PHONY: test
test:	testbuild $(TEST_LOG) testbanner $(TEST_LOGS)

.PHONY:	tests
tests: $(TEST_TARGETS)

# If the Makefile changes then rebuild all the objects
$(DEMO_OBJS):	$(MAKEFILE_NAME) $(CONFIG)

.PHONY: demodirs
demodirs: $(DEMODIRS)

$(DEMODIRS):
	$(Q)mkdir -p $@

DEMO_TARGETS := $(patsubst %,$(DEMO_DIR)/%,$(ALL_DEMOS))
.PHONY: $(DEMO_DIR)/%
$(DEMO_DIR)/%: $(WORK_DIR)/%
	$(Q)mkdir -p $@
	$(Q)cp $(CONFIG) $@/demo.cfg
	$(Q)make -f "$(WORK_DIR)/project/Makefile" -C "$(WORK_DIR)/project" CONFIG="$@/demo.cfg" SPEC_DIR="$(<)" BUILD_DIR="$@" MEOS_DIR="$(ABUILD_DIR)" CFLAGS="$(PRIVATE_CFLAGS)"
.PHONY: demobuild
demobuild: all $(TARGET_LIBRARY) demodirs $(DEMO_TARGETS)


.PHONY: demo
demo:	demobuild

.PHONY:	demos
demos:	demobuild

sinclude $(DEPFILES)
endif
