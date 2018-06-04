####(C)2015###############################################################
#
# Copyright (C) 2015 MIPS Tech, LLC
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
####(C)2015###############################################################
#
#          File:	$File: //meta/fw/meos2/DEV/LISA.PARRATT/Makefile $
# Revision date:	$Date: 2015/12/08 $
#   Description:	MVZ Makefile
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
MAJOR_ID = 0
MINOR_ID = 0
ID_QUALIFIER = A
ID_PATCH = 1

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

include $(MEOS_CONFIG)
AR = $(CONFIG_TOOLS_AR)
CC = $(CONFIG_TOOLS_CC)
CXX = $(CONFIG_TOOLS_CXX)
OBJCOPY = $(CONFIG_TOOLS_OBJCOPY)
STRIP = $(CONFIG_TOOLS_STRIP)
TOOLS_DIR=$(MEOS_BUILD)/tools
_MEOS_MAGIC_DO=BUILD
PYTHON_VERSION = $(shell python -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
include $(MEOS_SRC)/buildsys/tools.mk
include $(MEOS_SRC)/targets/*/module.mk
MEOS_ABUILD = $(abspath $(shell echo $(MEOS_BUILD)))
GUEST_MEOS_BUILD ?= $(MEOS_BUILD)
GUEST_MEOS_ABUILD = $(abspath $(shell echo $(GUEST_MEOS_BUILD)))
CFLAGS+=$(shell $(MEOS_ABUILD)/bin/meos-config --cflags) -g3 -O0
LFLAGS+=$(shell $(MEOS_ABUILD)/bin/meos-config --libs) -O0

# Working directory
WORK_DIR = $(CURDIR)

# Where to store the generated files
BUILD_DIR ?= /tmp/mvz.build/
ABUILD_DIR = $(abspath $(shell echo $(BUILD_DIR)))

#
# Source
#
XMLDOCS=
SRC=
ASMSRC=
include $(WORK_DIR)/mvz/module.mk
include $(WORK_DIR)/vio/module.mk

COBJ := $(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(filter %.c,$(SRC)))
CPPOBJ := $(patsubst %.cpp,$(ABUILD_DIR)/obj/%.o,$(filter %.cpp,$(SRC)))
SOBJ := $(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(filter %.s,$(ASMSRC))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(filter %.S,$(ASMSRC)))
OBJ := $(COBJ) $(CPPOBJ) $(SOBJ)
DEPFILES = $(patsubst $(ABUILD_DIR)/obj/%.o,$(ABUILD_DIR)/dep/%.d,$(COBJ) $(CPPOBJ))
CONFIG_TOOL = $(ABUILD_DIR)/bin/mvz-config
DOCS = $(ABUILD_DIR)/doc/mvzmanual.html $(ABUILD_DIR)/doc/mvzmanual.pdf
MVZ_HEADER = $(ABUILD_DIR)/include/MVZ.h
GENERATED_HEADERS = $(addprefix $(ABUILD_DIR)/include/,$(XMLDOCS:.xml=.h))
TARGET_LIBRARY = $(ABUILD_DIR)/lib/libMVZ.a($(OBJ))
TARGET_LIBRARY_SHORT = $(ABUILD_DIR)/lib/libMVZ.a
SUBDIRS = $(ABUILD_DIR)/bin $(ABUILD_DIR)/include/mvz/ $(ABUILD_DIR)/dep/mvz/ $(ABUILD_DIR)/obj/mvz/ $(ABUILD_DIR)/lib/ $(ABUILD_DIR)/doc/
INDENT = $(INDENT_EXEC) -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8

.PHONY: all
all: mvz

$(CONFIG_TOOL): $(MEOS_SRC)/buildsys/meos-config.template
	$(Q)echo $(call BANNER,CFGT,$(@))
	$(Q)mkdir -p $(dir $(CONFIG_TOOL))
	$(Q)sed -e 's%FF%%g' -e 's%LL%%g' -e 's%meos%mvz%g' -e 's%MEOS%MVZ%g' -e 's%PP%$(ABUILD_DIR)%g' -e 's/XX/$(MAJOR_ID)/g' -e 's/YY/$(MINOR_ID)/g' -e 's/ZZ/$(ID_PATCH)/g' < $< > $@ $(SUP2)
	$(Q)chmod gou+x $@

#
# Auto generated doc rules
#
$(ABUILD_DIR)/doc/%.html: mvz/mvz.xml $(XSLTPROC_EXECFILE) $(RST2HTML_EXECFILE)
	$(Q)echo $(call BANNER,HTML,$@)
	$(Q)mkdir -p $(ABUILD_DIR)/doc
	$(Q)$(XSLTPROC_EXEC) --path $(MEOS_SRC)/doc --xinclude $(MEOS_SRC)/doc/doc.xsl $< > $@.rst $(SUP2)
	$(Q)cp -f doc/mvzmanual.rst $(dir $@) $(SUP)
	$(Q)chmod gou+w $(dir $@)mvzmanual.rst
	$(Q)cd $(dir $@); $(RST2HTML_EXEC) --file-insertion-enabled --exit-status=2 mvzmanual.rst $@ $(SUP)
	$(Q)rm -f $@.rst $(SUP)
	$(Q)rm -f $(dir $@)/mvzmanual.rst $(SUP)

$(ABUILD_DIR)/doc/%.pdf: $(ABUILD_DIR)/doc/%.html $(XSLTPROC_EXECFILE) $(FOP_EXECFILE) $(DTD_LOCFILE)
	$(Q)echo $(call BANNER,PDF,$@)
	$(Q)XML_CATALOG_FILES="$(DTD_CAT)" $(XSLTPROC_EXEC) --path $(MEOS_SRC)/doc --stringparam main-title "MVZ" --stringparam sub-title "User Manual" --stringparam revision "$(VERSION)" --stringparam file-name "$(notdir $@)" --stringparam issue-date "$(shell date)" $(MEOS_SRC)/doc/xhtml2fo.xsl $< > $@.fo
	$(Q)$(FOP_EXEC) $@.fo $@ $(SUP)
	$(Q)rm $@.fo $(SUP)

.PHONY: docs
docs: $(DOCS)

$(MVZ_HEADER): $(GENERATED_HEADERS)
	$(Q)echo $(call BANNER,GENH,$@)
	$(Q)echo "#ifndef MVZPRIME_H" > $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#define MVZPRIME_H 1" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#define MVZ_MAJOR_ID $(MAJOR_ID)" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#define MVZ_MINOR_ID $(MINOR_ID)" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#define MVZ_ID_STRING \"$(VERSION)\"" >> $(MVZ_HEADER) $(SUP2)
	$(Q)cat $(MEOS_SRC)/buildsys/header.txt >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#ifdef __cplusplus" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "extern \"C\" {" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#include \"MEOS.h\"" >> $(MVZ_HEADER) $(SUP2);
	$(Q)for f in $(subst $(ABUILD_DIR)/include/,,$(filter-out .c,$(GENERATED_HEADERS))) ; do \
		echo "#include \"$${f}\"" >> $(MVZ_HEADER) $(SUP2); \
	done
	$(Q)echo "#ifdef __cplusplus" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "}" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#endif" >> $(MVZ_HEADER) $(SUP2)
	$(Q)echo "#endif /* not defined MVZPRIME_H */" >> $(MVZ_HEADER) $(SUP2)

$(ABUILD_DIR)/include/%.h: %.xml $(XSLTPROC_EXECFILE) $(CONFIG)
	$(Q)echo $(call BANNER,GENH,$(<))
	$(Q)$(XSLTPROC_EXEC) --path $(MEOS_SRC)/doc -o $@ $(MEOS_SRC)/buildsys/code.xsl $(WORK_DIR)/$< $(SUP)

$(SUBDIRS):
		$(Q)mkdir -p $@

# Rules of how to build the dependencies
$(ABUILD_DIR)/dep/%.d: %.c $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(MVZ_HEADER) $(CONFIGFILE_HEADER) $(INTTYPES_HEADER)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(INC) -MM -MF $@ -c $*.c -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp


$(ABUILD_DIR)/dep/%.d: %.cpp $(GENERATED_HEADERS) $(INSTALLED_HEADERS) $(COMPAT_MEOS_HEADER) $(MEOS_HEADER) $(MVZ_HEADER) $(CONFIGFILE_HEADER) $(INTTYPES_HEADER)
	$(Q)echo $(call BANNER,DEP,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(INC) -MM -MF $@ -c $*.cpp -o $@
	$(Q)mv -f $@ $@.tmp
	$(Q)sed -e 's|.*:|$*.o:|' < $@.tmp > $@
	$(Q)rm -f $@.tmp

# Rules of how to build the objects, and where they go
$(ABUILD_DIR)/obj/%.o: %.c $(INDENT_EXEC)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo $(call BANNER,FIXC,$(<));chmod u+w $*.c;$(INDENT) $*.c;$(INDENT) $*.c ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKC,$(<));$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo "$*.c breaks indentation policy: run $(INDENT) $*.c"; false ))
	$(Q)echo $(call BANNER,CC,$(<))
	$(Q)cd $(dir $@);$(CC) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.c -o $@

$(ABUILD_DIR)/obj/%.o: %.s
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),unexpand $*.s|tr -d "\r"|diff $*.s - > /dev/null || ( echo $(call BANNER,FIXS,$(<));chmod u+w $*.s;unexpand $*.s|tr -d "\r">$*.s.2;mv $*.s.2 $*.s ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKS,$(<));unexpand $*.s|tr -d "\r"|diff $*.s - > /dev/null || ( echo "$*.s breaks indentation policy: run unexpand $*.s|tr -d \"\\r\">out;mv out $*.s"; false ))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.s -o $@

$(ABUILD_DIR)/obj/%.o: %.S
	$(Q)mkdir -p $(dir $@)
	$(Q)$(if $(FIX_INDENT),unexpand $*.S|tr -d "\r"|diff $*.S - > /dev/null || ( echo $(call BANNER,FIXS,$(<));chmod u+w $*.S;unexpand $*.S|tr -d "\r">$*.S.2;mv $*.S.2 $*.S ))
	$(Q)$(if $(LIBERAL_INDENT),,echo $(call BANNER,CHKS,$(<));unexpand $*.S|tr -d "\r"|diff $*.S - > /dev/null || ( echo "$*.S breaks indentation policy: run unexpand $*.S|tr -d \"\\r\">out;mv out $*.S"; false ))
	$(Q)echo $(call BANNER,AS,$(<))
	$(Q)cd $(dir $@);$(CC) $(CFLAGS) $(INC) -c $(WORK_DIR)/$*.S -o $@

# Rules of how to check headers
.PHONY: %.h.indent
%.h.indent: %.h $(INDENT_EXEC)
	$(Q)echo $(call BANNER,CHKH,$<)
	$(Q)$(if $(FIX_INDENT),echo $(call BANNER,FIXH,$(<));$(INDENT) $*.h;$(INDENT) $*.h)
	$(Q)$(if $(LIBERAL_INDENT),,$(INDENT) $*.h -st|$(INDENT) -st -|diff $*.h - > /dev/null || ( echo "$*.h breaks indentation policy: run $(INDENT) $$*.h"; false ))

.PHONY: %.c.indent
%.c.indent: %.c $(INDENT_EXEC)
	$(Q)echo $(call BANNER,CHKC,$<)
	$(Q)$(if $(FIX_INDENT),echo $(call BANNER,FIXC,$(<));$(INDENT) $*.c;$(INDENT) $*.c)
	$(Q)$(if $(LIBERAL_INDENT),,$(INDENT) $*.c -st|$(INDENT) -st -|diff $*.c - > /dev/null || ( echo "$*.c breaks indentation policy: run $(INDENT) $$*.c"; false ))

(%.o): %.o
	$(Q)echo $(call BANNER,AR,$<)
	$(Q)$(AR) rv $@ $% $(SUP)

.PHONY: clean
clean:
	$(Q)echo $(call BANNER,CLEAN,$(@))
	$(Q)-rm -f $(DEPFILES)
	$(Q)-rm -fr $(ABUILD_DIR)/dep/mvz
	$(Q)-rm -f $(ABUILD_DIR)/doc/mvzmanual.html
	$(Q)-rm -f $(ABUILD_DIR)/doc/mvzmanual.pdf
	$(Q)-rm -f $(CONFIG_TOOL)
	$(Q)-rm -f $(GENERATED_HEADERS)
	$(Q)-rm -f $(MVZ_HEADER)
	$(Q)-rm -fr $(ABUILD_DIR)/include/mvz/
	$(Q)-rm -f $(ABUILD_DIR)/walks
	$(Q)-rm -f $(TARGET_LIBRARY_SHORT)
	$(Q)-rm -f $(OBJ)

.PHONY: $(ABUILD_DIR)/walks
$(ABUILD_DIR)/walks:
	$(Q)mkdir -p $(ABUILD_DIR)/walks
	$(Q)cp -r $(WORK_DIR)/walks/* $(ABUILD_DIR)/walks/ $(SUP2);

#include demo/module.mk
include linuxharness/module.mk

.PHONY: mvz
mvz: $(SUBDIRS) $(GENERATED_HEADERS) $(MVZ_HEADER) $(DEPFILES) $(TARGET_LIBRARY) $(DOCS) $(CONFIG_TOOL)
