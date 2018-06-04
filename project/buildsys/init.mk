# Utility functions

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
define NEWLINE


endef
GREEN:=[32m
NORMAL:=[39m
BANNER="$(GREEN)[$(NORMAL)$(1)$(GREEN)]$(NORMAL)	$2"
ifndef V
SUP = >$(ABUILD_DIR)/.lasterr 2>&1 || (cat $(ABUILD_DIR)/.lasterr; false;)
SUP2 = 2>$(ABUILD_DIR)/.lasterr || (cat $(ABUILD_DIR)/.lasterr; false;)
Q := @
else
Q :=
endif
CHECKINDENT = $(if $(INDENT_POLICY),$(if $(FIX_INDENT), ( $(INDENT_TOOL) $(INDENT_POLICY) $(1) -st|$(INDENT_TOOL) $(INDENT_POLICY) -st -|diff $(1) - > /dev/null || ( echo $(call BANNER,FIXC,$(1));chmod u+w $(1);$(INDENT_TOOL) $(INDENT_POLICY) $(1);$(INDENT_TOOL) $(INDENT_POLICY) $(1) ) ) , true) ; ( $(if $(LIBERAL_INDENT),true,echo $(call BANNER,CHKC,$(1));$(INDENT_TOOL) $(INDENT_POLICY) $(1) -st|$(INDENT_TOOL) $(INDENT_POLICY) -st -|diff $(1) - > /dev/null || ( echo "$(1) breaks indentation policy: run $(INDENT_TOOL) $(INDENT_POLICY) $(1)"; false ) ) ) )
CHECKSINDENT = $(if $(INDENT_POLICY),$(if $(FIX_INDENT), ( unexpand $(1)|tr -d "\r"|diff $(1) - > /dev/null || ( echo $(call BANNER,FIXS,$(1));chmod u+w $(1);unexpand $(1)|tr -d "\r">$(1).2;mv $(1).2 $(1) ) ) , true) ; ( $(if $(LIBERAL_INDENT),true,echo $(call BANNER,CHKS,$(1));unexpand $(1)|tr -d "\r"|diff $(1) - > /dev/null || ( echo "$(1) breaks indentation policy: run unexpand $(1)|tr -d \"\\r\">out;mv out $(1)"; false ) ) ) )

# Working directory
WORK_DIR = $(CURDIR)

# Where to store the generated files
BUILD_DIR ?= $(WORK_DIR)
ABUILD_DIR := $(abspath $(shell echo $(BUILD_DIR)))

# Configuration file
CONFIG ?= $(WORK_DIR)/build.cfg

# MEOS
ifeq ($(MEOS_DIR),)
ifneq ($(shell which meos-config 2>/dev/null||echo bad),bad)
MEOS_DIR = $(shell meos-config --prefix)
endif
endif

# Tools
ifneq ($(wildcard $(MEOS_DIR)/bin/meos-config),)
AR := $(shell $(MEOS_DIR)/bin/meos-config --ar)
CC := $(shell $(MEOS_DIR)/bin/meos-config --cc)
CXX := $(shell $(MEOS_DIR)/bin/meos-config --cxx)
MCFLAGS = $(CFLAGS) $(shell $(MEOS_DIR)/bin/meos-config --cflags)
MLFLAGS = $(LFLAGS) $(shell $(MEOS_DIR)/bin/meos-config --libs)
TARGET := $(shell $(MEOS_DIR)/bin/meos-config --target)
KCONFIG_TOOL := $(MEOS_DIR)/bin/kconfig-mconf
OLDCONFIG_TOOL := $(MEOS_DIR)/bin/kconfig-conf
DTWALK_TOOL := $(MEOS_DIR)/bin/dtwalk
INDENT_TOOL := $(MEOS_DIR)/bin/indent
DTC_TOOL := $(MEOS_DIR)/bin/dtc
else
MCFLAGS = $(ICFLAGS) $(CFLAGS)
MLFLAGS = $(ILFLAGS) $(LFLAGS)
endif
INDENT := $(INDENT_TOOL) $(INDENT_POLICY)

# Specialisation
SPEC_DIR ?= $(WORK_DIR)
