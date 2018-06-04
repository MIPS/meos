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
#   Description:	Automated tool dependency build rules
#
##########################################################################

TOOLS_DIR ?= $(ABUILD_DIR)/tools
CURL ?= $(shell which curl 2>/dev/null||echo false)
WGET ?= $(shell which wget 2>/dev/null||echo false)

ifeq (false,$(CURL))
ifeq (false,$(WGET))
	$(error Install curl or wget!)
endif
endif

.PHONY: silent
silent:
	@:

UNAME=$(call LOWER, $(shell uname))
ifeq ($(UNAME),linux)
LSB_RELEASE ?= $(shell which lsb_release 2> /dev/null||echo false)
ifeq (false,$(LSB_RELEASE))
DISTRO=$(call LOWER,$(firstword $(shell ls -d /etc/[A-Za-z]*[_-][rv]e[lr]* | grep -v "lsb" | cut -d'/' -f3 | cut -d'-' -f1 | cut -d'_' -f1)))
else
DISTRO=$(call LOWER,$(firstword $(shell lsb_release -i | cut -d: -f2 | sed s/'^\t'//)))
endif
endif
HOSTARCH=$(shell uname -m)
DPKGS=build-essential python python-setuptools m4 unzip
JPKG=$(shell (apt-cache show openjdk-9-jre >/dev/null 2>&1 && echo openjdk-9-jre) || (apt-cache show openjdk-8-jre >/dev/null 2>&1 && echo openjdk-8-jre) || (apt-cache show openjdk-7-jre >/dev/null 2>&1 && echo openjdk-7-jre))
ifeq ($(DISTRO),ubuntu)
base:
	$(Q)(apt-get install -s $(DPKGS) $(JPKG)| grep "0 [^,]* newly" > /dev/null) || (echo Base tools required, executing sudo apt-get install -y $(DPKGS)  $(JPKG); sudo apt-get install -y $(DPKGS) $(JPKG))
else
ifeq ($(DISTRO),debian)
base:
	$(Q)(apt-get install -s $(DPKGS) $(JPKG)| grep "0 [^,]* newly" > /dev/null) || (echo Base tools required, executing sudo apt-get install -y $(DPKGS)  $(JPKG); sudo apt-get install -y $(DPKGS)  $(JPKG))
else
ifeq ($(DISTRO),fedora)
FEDORA_LE18=$(shell [ `sed -rn 's/[^0-9]*([0-9]*)[^0-9].*/0\1/p' /etc/fedora-release` -le 18 ]&&echo true)
FEDORA_LE21=$(shell [ `sed -rn 's/[^0-9]*([0-9]*)[^0-9].*/0\1/p' /etc/fedora-release` -le 21 ]&&echo true)
FEDORA_LE24=$(shell [ `sed -rn 's/[^0-9]*([0-9]*)[^0-9].*/0\1/p' /etc/fedora-release` -le 24 ]&&echo true)
RPMS=autoconf automake binutils bison flex gcc gcc-c++ gettext libtool make openssl-devel patch pkgconfig python-devel redhat-rpm-config rpm-build byacc cscope ctags cvs diffstat doxygen elfutils gcc-gfortran git indent intltool patchutils rcs subversion swig systemtap time
ifeq ($(FEDORA_LE18),true)
RPMS+=java-1.7.0-openjdk
else
RPMS+=java-1.8.0-openjdk
endif
ifeq ($(FEDORA_LE24),true)
RPMS+=python-setuptools
else
RPMS+=python2-setuptools
endif
ifeq ($(FEDORA_LE21),true)
base:
	$(Q)checkyums () { for YUM in "$$@"; do (yum info $$YUM|grep ": installed" > /dev/null) || return 1; done; return 0; }; checkyums $(RPMS) || (echo "Base tools required, executing sudo yum install $(RPMS) -y"; sudo yum install $(RPMS) -y)
else
base:
	$(Q)checkdnfs () { for DNF in "$$@"; do (dnf info $$DNF|grep "Installed Packages" > /dev/null) || return 1; done; return 0; }; checkdnfs $(RPMS) || (echo "Base tools required, executing sudo dnf install $(RPMS) -y"; sudo dnf install $(RPMS) -y)
endif
else
RPMS=autoconf automake binutils bison flex gcc gcc-c++ gettext libtool make openssl-devel  patch pkgconfig python-devel python-setuptools redhat-rpm-config rpm-build byacc cscope ctags cvs diffstat doxygen elfutils gcc-gfortran git indent intltool patchutils rcs subversion swig systemtap time java-1.8.0-openjdk
ifeq ($(DISTRO),rhel)
base:
	$(Q)checkyums () { for YUM in "$$@"; do (yum info $$YUM|grep ": installed" > /dev/null) || return 1; done; return 0; }; checkyums $(RPMS) || (echo "Base tools required, executing sudo yum install $(RPMS) -y"; sudo yum install $(RPMS) -y)
else
ifeq ($(DISTRO),centos)
base:
	$(Q)checkyums () { for YUM in "$$@"; do (yum info $$YUM|grep ": installed" > /dev/null) || return 1; done; return 0; }; checkyums $(RPMS) || (echo "Base tools required, executing sudo yum install $(RPMS) -y"; sudo yum install $(RPMS) -y)
else
base:
	@:
endif
endif
endif
endif
endif

BZ2_VERSION = bzip2-1.0.6
BZ2_FILE = $(BZ2_VERSION).tar.gz
CONFIG_TOOLS_BZ2_SRC ?= http://www.bzip.org/1.0.6/
BZ2_URL = $(CONFIG_TOOLS_BZ2_SRC)$(BZ2_FILE)
BZ2_EXEC ?= $(BZ2_EXECFILE)
ifeq (,$(BZ2_EXECFILE))
BZ2_EXECFILE := $(wildcard $(shell which bunzip2 2>/dev/null))
endif
ifeq (,$(BZ2_EXECFILE))
BZ2_EXECFILE := $(TOOLS_DIR)/$(BZ2_VERSION)/meosinstall/bin/bunzip2
endif
$(BZ2_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(BZ2_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(BZ2_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(BZ2_URL) -o $(TOOLS_DIR)/$(BZ2_FILE) || $(WGET) $(BZ2_URL) -O $(TOOLS_DIR)/$(BZ2_FILE)
	$(Q)echo $(call BANNER,EX,$(BZ2_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(BZ2_FILE) $(SUP)
	$(Q)echo $(call BANNER,MK,$(BZ2_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(BZ2_VERSION) install PREFIX=`echo $(TOOLS_DIR)/$(BZ2_VERSION)/meosinstall` $(SUP)

GPERF_VERSION = gperf-3.0.4
GPERF_FILE = $(GPERF_VERSION).tar.gz
CONFIG_TOOLS_GPERF_SRC ?= http://www.mirrorservice.org/sites/ftp.gnu.org/gnu/gperf/
GPERF_URL = $(CONFIG_TOOLS_GPERF_SRC)$(GPERF_FILE)
GPERF_EXEC ?= $(GPERF_EXECFILE)
ifeq (,$(GPERF_EXECFILE))
GPERF_EXECFILE := $(wildcard $(shell which gperf 2>/dev/null))
endif
ifeq (,$(GPERF_EXECFILE))
GPERF_EXECFILE := $(TOOLS_DIR)/$(GPERF_VERSION)/meosinstall/bin/gperf
endif
$(GPERF_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(GPERF_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(GPERF_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(GPERF_URL) -o $(TOOLS_DIR)/$(GPERF_FILE) || $(WGET) $(GPERF_URL) -O $(TOOLS_DIR)/$(GPERF_FILE)
	$(Q)echo $(call BANNER,EX,$(GPERF_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(GPERF_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(GPERF_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(GPERF_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(GPERF_VERSION); unset CFLAGS;unset LDFLAGS;./configure --prefix=`echo $(TOOLS_DIR)/$(GPERF_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(GPERF_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(GPERF_VERSION) install $(SUP)

BISON_VERSION = bison-3.0
BISON_FILE = $(BISON_VERSION).tar.gz
CONFIG_TOOLS_BISON_SRC ?= http://www.mirrorservice.org/sites/ftp.gnu.org/gnu/bison/
BISON_URL = $(CONFIG_TOOLS_BISON_SRC)$(BISON_FILE)
BISON_EXEC ?= $(BISON_EXECFILE)
ifeq (,$(BISON_EXECFILE))
BISON_EXECFILE := $(wildcard $(shell which bison 2>/dev/null))
endif
ifeq (,$(BISON_EXECFILE))
BISON_EXECFILE := $(TOOLS_DIR)/$(BISON_VERSION)/meosinstall/bin/bison
endif
$(BISON_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(BISON_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(BISON_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(BISON_URL) -o $(TOOLS_DIR)/$(BISON_FILE) || $(WGET) $(BISON_URL) -O $(TOOLS_DIR)/$(BISON_FILE)
	$(Q)echo $(call BANNER,EX,$(BISON_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(BISON_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(BISON_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(BISON_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(BISON_VERSION); unset CFLAGS;unset LDFLAGS;./configure --prefix=`echo $(TOOLS_DIR)/$(BISON_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(BISON_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(BISON_VERSION) install $(SUP)

FLEX_VERSION = flex-2.6.1
FLEX_FILE = $(FLEX_VERSION).tar.gz
CONFIG_TOOLS_FLEX_SRC ?= https://github.com/westes/flex/releases/download/v2.6.1/
FLEX_URL = $(CONFIG_TOOLS_FLEX_SRC)$(FLEX_FILE)
FLEX_EXEC ?= $(FLEX_EXECFILE)
ifeq (,$(FLEX_EXECFILE))
FLEX_EXECFILE := $(wildcard $(shell which flex 2>/dev/null))
endif
ifeq (,$(FLEX_EXECFILE))
FLEX_EXECFILE := $(TOOLS_DIR)/$(FLEX_VERSION)/meosinstall/bin/flex
endif
$(FLEX_EXECFILE): $(BISON_EXECFILE)
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(FLEX_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(FLEX_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) -L $(FLEX_URL) -o $(TOOLS_DIR)/$(FLEX_FILE) || $(WGET) $(FLEX_URL) -O $(TOOLS_DIR)/$(FLEX_FILE)
	$(Q)echo $(call BANNER,EX,$(FLEX_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(FLEX_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(FLEX_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(FLEX_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(FLEX_VERSION); unset CFLAGS;unset LDFLAGS;YACC="$(BISON_EXECFILE) -y" ./configure --prefix=`echo $(TOOLS_DIR)/$(FLEX_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(FLEX_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(FLEX_VERSION) install $(SUP)

NCURSES_VERSION = ncurses-6.0
NCURSES_FILE = $(NCURSES_VERSION).tar.gz
CONFIG_TOOLS_NCURSES_SRC ?= http://www.mirrorservice.org/sites/ftp.gnu.org/gnu/ncurses/
NCURSES_URL = $(CONFIG_TOOLS_NCURSES_SRC)$(NCURSES_FILE)
ifeq (,$(NCURSES_EXECFILE))
NCURSES_EXECFILE := $(wildcard $(shell which ncurses6-config 2>/dev/null))
endif
ifeq (,$(NCURSES_EXECFILE))
NCURSES_EXECFILE := $(TOOLS_DIR)/$(NCURSES_VERSION)/meosinstall/bin/ncurses6-config
else
ifeq (,$(wildcard $(shell $(NCURSES_EXECFILE) --libdir)/*ncurses*))
NCURSES_EXECFILE := $(TOOLS_DIR)/$(NCURSES_VERSION)/meosinstall/bin/ncurses6-config
endif
endif
NCURSES_EXEC = $(NCURSES_EXECFILE)
$(NCURSES_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(NCURSES_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(NCURSES_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(NCURSES_URL) -o $(TOOLS_DIR)/$(NCURSES_FILE) || $(WGET) $(NCURSES_URL) -O $(TOOLS_DIR)/$(NCURSES_FILE)
	$(Q)echo $(call BANNER,EX,$(NCURSES_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(NCURSES_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(NCURSES_VERSION))
	$(Q)patch $(TOOLS_DIR)/$(NCURSES_VERSION)/ncurses/base/MKlib_gen.sh < $(WORK_DIR)/buildsys/ncurses_gcc5p.patch $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR)/$(NCURSES_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(NCURSES_VERSION); unset CFLAGS;unset LDFLAGS;./configure --prefix=`echo $(TOOLS_DIR)/$(NCURSES_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(NCURSES_VERSION))
	$(Q)MAKEFLAGS= unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(NCURSES_VERSION) install $(SUP)

KCONFIG_VERSION = kconfig-frontends-3.12.0.0
KCONFIG_FILE = $(KCONFIG_VERSION).tar.bz2
CONFIG_TOOLS_KCONFIG_SRC ?= http://ymorin.is-a-geek.org/download/kconfig-frontends/
KCONFIG_URL = $(CONFIG_TOOLS_KCONFIG_SRC)$(KCONFIG_FILE)
KCONFIG_EXECFILE ?= $(TOOLS_DIR)/$(KCONFIG_VERSION)/meosinstall/bin/kconfig-mconf
KCONFIG_OLDEXECFILE ?= $(TOOLS_DIR)/$(KCONFIG_VERSION)/meosinstall/bin/kconfig-conf
KCONFIG_EXEC ?= $(KCONFIG_EXECFILE)
$(KCONFIG_EXECFILE): $(BZ2_EXECFILE) $(GPERF_EXECFILE) $(FLEX_EXECFILE) $(BISON_EXECFILE) $(NCURSES_EXECFILE) | silent
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(KCONFIG_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(KCONFIG_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(KCONFIG_URL) -o $(TOOLS_DIR)/$(KCONFIG_FILE) || $(WGET) $(KCONFIG_URL) -O $(TOOLS_DIR)/$(KCONFIG_FILE)
	$(Q)echo $(call BANNER,EX,$(KCONFIG_VERSION))
	$(Q)$(BZ2_EXEC) -c $(TOOLS_DIR)/$(KCONFIG_FILE) | tar -C $(TOOLS_DIR) -xv $(SUP)
	$(Q)echo $(call BANNER,CF,$(KCONFIG_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(KCONFIG_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(KCONFIG_VERSION); (unset CFLAGS;unset LDFLAGS;GPERF=$(GPERF_EXECFILE) LEX=$(FLEX_EXECFILE) YACC="$(BISON_EXECFILE) -y" ./configure --prefix=`echo $(TOOLS_DIR)/$(KCONFIG_VERSION)/meosinstall` --enable-nconf|| CFLAGS=`$(NCURSES_EXEC) --cflags` LDFLAGS=`$(NCURSES_EXEC) --libs` GPERF=$(GPERF_EXECFILE) LEX=$(FLEX_EXECFILE) YACC="$(BISON_EXECFILE) -y" ./configure --prefix=`echo $(TOOLS_DIR)/$(KCONFIG_VERSION)/meosinstall` --enable-nconf --disable-werror;) $(SUP)
	$(Q)echo $(call BANNER,MK,$(KCONFIG_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(KCONFIG_VERSION) install $(SUP)

ifeq ($(_MEOS_MAGIC_DO),BUILD)

INDENT_VERSION = indent-2.2.10
INDENT_FILE = $(INDENT_VERSION).tar.gz
INDENT_URL = $(CONFIG_TOOLS_INDENT_SRC)$(INDENT_FILE)
INDENT_EXECFILE ?= $(TOOLS_DIR)/$(INDENT_VERSION)/meosinstall/bin/indent
INDENT_EXEC ?= $(INDENT_EXECFILE)
$(INDENT_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(INDENT_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(INDENT_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(INDENT_URL) -o $(TOOLS_DIR)/$(INDENT_FILE) || $(WGET) $(INDENT_URL) -O $(TOOLS_DIR)/$(INDENT_FILE)
	$(Q)echo $(call BANNER,EX,$(INDENT_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(INDENT_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(INDENT_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(INDENT_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(INDENT_VERSION); unset CFLAGS;unset LDFLAGS;./configure --prefix=`echo $(TOOLS_DIR)/$(INDENT_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(INDENT_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(INDENT_VERSION) install $(SUP)

XML2_VERSION = libxml2-2.9.1
XML2_FILE = $(XML2_VERSION).tar.gz
XML2_URL = $(CONFIG_TOOLS_XML2_SRC)$(XML2_FILE)
XML2_LIB = $(TOOLS_DIR)/$(XML2_VERSION)/meosinstall/lib/libxml2.a
$(XML2_LIB):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(XML2_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(XML2_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(XML2_URL) -o $(TOOLS_DIR)/$(XML2_FILE) || $(WGET) $(XML2_URL) -O $(TOOLS_DIR)/$(XML2_FILE)
	$(Q)echo $(call BANNER,EX,$(XML2_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(XML2_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(XML2_VERSION))
	$(Q)mkdir $(TOOLS_DIR)/$(XML2_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(XML2_VERSION); unset CFLAGS;unset LDFLAGS;./configure --without-python --prefix=`echo $(TOOLS_DIR)/$(XML2_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(XML2_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(XML2_VERSION) install $(SUP)

XSLTPROC_VERSION = libxslt-1.1.28
XSLTPROC_FILE = $(XSLTPROC_VERSION).tar.gz
XSLTPROC_URL = $(CONFIG_TOOLS_XSLTPROC_SRC)$(XSLTPROC_FILE)
ifeq (,$(XSLTPROC_EXECFILE)$(XSLTPROC_EXEC))
#ifeq (,$(wildcard $(shell which xsltproc 2>/dev/null)))
XSLTPROC_EXECFILE ?= $(TOOLS_DIR)/$(XSLTPROC_VERSION)/meosinstall/bin/xsltproc
XSLTPROC_EXEC ?= $(XSLTPROC_EXECFILE)
#else
#XSLTPROC_EXECFILE ?= $(shell which xsltproc 2>/dev/null)
#XSLTPROC_EXEC ?= $(XSLTPROC_EXECFILE)
#XML2_LIB=
#endif
endif
$(XSLTPROC_EXECFILE): $(XML2_LIB)
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(XSLTPROC_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(XSLTPROC_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(XSLTPROC_URL) -o $(TOOLS_DIR)/$(XSLTPROC_FILE) || $(WGET) $(XSLTPROC_URL) -O $(TOOLS_DIR)/$(XSLTPROC_FILE)
	$(Q)echo $(call BANNER,EX,$(XSLTPROC_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(XSLTPROC_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(XSLTPROC_VERSION))
	$(Q)mkdir $(TOOLS_DIR)/$(XSLTPROC_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(XSLTPROC_VERSION); unset CFLAGS;unset LDFLAGS;./configure --with-libxml-prefix=`echo $(TOOLS_DIR)/$(XML2_VERSION)/meosinstall` --prefix=`echo $(TOOLS_DIR)/$(XSLTPROC_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(XSLTPROC_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(XSLTPROC_VERSION) install $(SUP)

PYGMENTS_VERSION = Pygments-1.6
PYGMENTS_FILE = $(PYGMENTS_VERSION).tar.gz
PYGMENTS_URL = $(CONFIG_TOOLS_PYGMENTS_SRC)$(PYGMENTS_FILE)
PYGMENTS_EXECFILE := $(wildcard $(TOOLS_DIR)/python/usr/local/bin/pygmentize)$(wildcard $(TOOLS_DIR)/python/usr/bin/pygmentize)
ifeq (,$(PYGMENTS_EXECFILE))
PYGMENTS_EXECFILE := $(TOOLS_DIR)/python/bin/pygmentize
endif
$(PYGMENTS_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(PYGMENTS_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(PYGMENTS_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/site-packages $(SUP)
	$(Q)$(CURL) $(PYGMENTS_URL) -o $(TOOLS_DIR)/$(PYGMENTS_FILE) || $(WGET) --no-check-certificate $(PYGMENTS_URL) -O $(TOOLS_DIR)/$(PYGMENTS_FILE)
	$(Q)echo $(call BANNER,EX,$(PYGMENTS_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(PYGMENTS_FILE) $(SUP)
	$(Q)echo $(call BANNER,MK,$(PYGMENTS_VERSION))
	$(Q)cd $(TOOLS_DIR)/$(PYGMENTS_VERSION); PYTHONPATH=$(TOOLS_DIR)/python/usr/local/lib/python$(PYTHON_VERSION)/site-packages:$(TOOLS_DIR)/python/usr/local/lib/python$(PYTHON_VERSION)/dist-packages python ./setup.py install --root=$(TOOLS_DIR)/python/ --prefix=. $(SUP)

DOCUTILS_VERSION = docutils-0.11
DOCUTILS_FILE = $(DOCUTILS_VERSION).tar.gz
DOCUTILS_URL = $(CONFIG_TOOLS_DOCUTILS_SRC)$(DOCUTILS_FILE)
RST2HTML_EXECFILE := $(TOOLS_DIR)/python/bin/rst2html.py
ifeq (,$(RST2HTML_EXECFILE))
RST2HTML_EXECFILE := $(TOOLS_DIR)/bin/rst2html.py
endif
RST2HTML_EXEC ?= PYTHONPATH=$(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/site-packages:$(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/dist-packages python $(RST2HTML_EXECFILE)
$(RST2HTML_EXECFILE):  $(PYGMENTS_EXECFILE)
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(DOCUTILS_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(DOCUTILS_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/site-packages $(SUP)
	$(Q)$(CURL) $(DOCUTILS_URL) -o $(TOOLS_DIR)/$(DOCUTILS_FILE) || $(WGET) $(DOCUTILS_URL) -O $(TOOLS_DIR)/$(DOCUTILS_FILE)
	$(Q)echo $(call BANNER,EX,$(DOCUTILS_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(DOCUTILS_FILE) $(SUP)
	$(Q)echo $(call BANNER,MK,$(DOCUTILS_VERSION))
	$(Q)cd $(TOOLS_DIR)/$(DOCUTILS_VERSION); PYTHONPATH=$(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/site-packages:$(TOOLS_DIR)/python/lib/python$(PYTHON_VERSION)/dist-packages python ./setup.py install --root=$(TOOLS_DIR)/python/ --prefix=. $(SUP)

# Use system Java if available
JRE_VERSION = jre1.7.0_40
JAVA_HOME = $(TOOLS_DIR)/$(JRE_VERSION)
ifeq (,$(wildcard $(JRE_EXECFILE)))
ifeq (,$(wildcard $(shell which java 2>/dev/null)))
# Last file in archive, ensures it unpacked ok
JRE_EXECFILE = $(TOOLS_DIR)/$(JRE_VERSION)/README
else
JAVA_HOME :=
JRE_EXECFILE :=
endif
endif

JRE_FILE ?= $(CONFIG_TOOLS_JRE_BIN_FN)
JRE_URL = $(CONFIG_TOOLS_JRE_BIN)$(JRE_FILE)
$(JRE_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(JRE_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(FOP_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) -L $(JRE_URL) -o $(TOOLS_DIR)/$(JRE_FILE) || $(WGET) $(JRE_URL) -O $(TOOLS_DIR)/$(JRE_FILE)
	$(Q)echo $(call BANNER,EX,$(JRE_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(JRE_FILE) $(SUP)

FOP_VERSION = fop-1.1
FOP_FILE = $(FOP_VERSION)-bin.tar.gz
FOP_URL = $(CONFIG_TOOLS_FOP_BIN)$(FOP_FILE)
# Last file in archive, ensures it unpacked ok
FOP_EXECFILE ?= $(TOOLS_DIR)/$(FOP_VERSION)/status.xml
FOP_EXEC ?= JAVA_HOME=$(JAVA_HOME) $(TOOLS_DIR)/$(FOP_VERSION)/fop
$(FOP_EXECFILE): $(JRE_EXECFILE)
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(FOP_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(FOP_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL)  $(FOP_URL) -o $(TOOLS_DIR)/$(FOP_FILE) || $(WGET) $(FOP_URL) -O $(TOOLS_DIR)/$(FOP_FILE)
	$(Q)echo $(call BANNER,EX,$(FOP_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(FOP_FILE) $(SUP)
	$(Q)touch $(FOP_EXECFILE)

DTD_VERSION = 1
DTD_FILE = xhtml$(DTD_VERSION).tgz
DTD_URL = $(CONFIG_TOOLS_DTD_SRC)$(DTD_FILE)
DTD_LOCFILE ?= $(TOOLS_DIR)/xhtml$(DTD_VERSION)-20020801/DTD/xhtml$(DTD_VERSION)-transitional.dtd
DTD_CAT ?= $(TOOLS_DIR)/xhtml$(DTD_VERSION)-20020801/DTD/catalog.xml
$(DTD_LOCFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,xhtml$(DTD_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/xhtml$(DTD_VERSION)-20020801 $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) $(DTD_URL) -o $(TOOLS_DIR)/$(DTD_FILE) || $(WGET) $(DTD_URL) -O $(TOOLS_DIR)/$(DTD_FILE)
	$(Q)echo $(call BANNER,EX,xhtml$(DTD_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(DTD_FILE) $(SUP)
	$(Q)echo $(call BANNER,CAT,xhtml$(DTD_VERSION))
	$(Q)echo '<?xml version="1.0"?><catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog"><system systemId="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" uri="file://$(DTD_LOCFILE)"/></catalog>' > $(DTD_CAT)

GNUPLOT_VERSION = gnuplot-4.6.4
GNUPLOT_FILE = $(GNUPLOT_VERSION).tar.gz
GNUPLOT_URL = $(CONFIG_TOOLS_GNUPLOT_SRC)$(GNUPLOT_FILE)
GNUPLOT_EXECFILE ?= $(TOOLS_DIR)/$(GNUPLOT_VERSION)/meosinstall/bin/gnuplot
GNUPLOT_EXEC ?= $(GNUPLOT_EXECFILE)
$(GNUPLOT_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,$(GNUPLOT_VERSION))
	$(Q)rm -fr $(TOOLS_DIR)/$(GNUPLOT_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(CURL) -L $(GNUPLOT_URL) -o $(TOOLS_DIR)/$(GNUPLOT_FILE) || $(WGET) $(GNUPLOT_URL) -O $(TOOLS_DIR)/$(GNUPLOT_FILE)
	$(Q)echo $(call BANNER,EX,$(GNUPLOT_VERSION))
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(GNUPLOT_FILE) $(SUP)
	$(Q)echo $(call BANNER,CF,$(GNUPLOT_VERSION))
	$(Q)mkdir -p $(TOOLS_DIR)/$(GNUPLOT_VERSION)/meosinstall
	$(Q)cd $(TOOLS_DIR)/$(GNUPLOT_VERSION); unset CFLAGS;unset LDFLAGS;./configure --without-lua --without-x --prefix=`echo $(TOOLS_DIR)/$(GNUPLOT_VERSION)/meosinstall` $(SUP)
	$(Q)echo $(call BANNER,MK,$(GNUPLOT_VERSION))
	$(Q)unset CFLAGS;unset LDFLAGS;make -C $(TOOLS_DIR)/$(GNUPLOT_VERSION) install $(SUP)

endif

DTC_VERSION = b238f77a0a7cb2ef814672e7f7258c4f5e594cc3
DTC_DL = $(DTC_VERSION).tar.gz
DTC_URL = $(CONFIG_TOOLS_DTC_SRC)$(DTC_DL)
DTC_EXECFILE ?= $(TOOLS_DIR)/$(DTC_VERSION)/meosinstall/bin/dtc
DTC_EXEC ?= $(DTC_EXECFILE)
DTC_PATH ?= $(TOOLS_DIR)/$(DTC_VERSION)/meosinstall/bin
$(DTC_EXECFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,DTC)
	$(Q)rm -fr $(TOOLS_DIR)/$(DTC_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(Q)$(CURL) -L $(DTC_URL) -o $(TOOLS_DIR)/$(DTC_DL) || $(WGET) $(DTC_URL) -O $(TOOLS_DIR)/$(DTC_DL)
	$(Q)echo $(call BANNER,EX,DTC)
	$(Q)tar -C $(TOOLS_DIR) -xvzf $(TOOLS_DIR)/$(DTC_DL) $(SUP)
	$(Q)echo $(call BANNER,MK,DTC)
	$(Q)mkdir -p $(TOOLS_DIR)/$(DTC_VERSION)/meosinstall
	$(Q)unset CFLAGS;unset LDFLAGS;HOME=`echo $(TOOLS_DIR)/$(DTC_VERSION)/meosinstall` PATH=$(PATH):$(TOOLS_DIR)/$(FLEX_VERSION)/meosinstall/bin:$(TOOLS_DIR)/$(BISON_VERSION)/meosinstall/bin make -C $(TOOLS_DIR)/$(DTC_VERSION) install $(SUP)

P32PS_VERSION = 166578d941dc24c799f9a3c23ba766f98764c12f
P32PS_FILE = $(P32PS_VERSION).zip
P32PS_URL = $(CONFIG_TOOLS_P32PS_SRC)$(P32PS_FILE)
P32PS_LIBFILE ?= $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/lib/libpmgc3451.a
P32PS_PATH ?= $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/
$(P32PS_LIBFILE):
	$(Q)mkdir -p $(TOOLS_DIR)
	$(Q)echo $(call BANNER,DL,P32PS)
	$(Q)rm -fr $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION) $(SUP)
	$(Q)mkdir -p $(TOOLS_DIR) $(SUP)
	$(CURL) -L $(P32PS_URL) -o $(TOOLS_DIR)/$(P32PS_FILE) || $(WGET) $(P32PS_URL) -O $(TOOLS_DIR)/$(P32PS_FILE)
	$(Q)echo $(call BANNER,EX,P32PS)
	$(Q)unzip -d $(TOOLS_DIR) $(TOOLS_DIR)/$(P32PS_FILE)
	$(Q)echo $(call BANNER,MK,P32PS)
	$(Q)mkdir -p $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/lib
	$(Q)mkdir -p $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/include/proc
	$(Q)mkdir -p $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/obj
	$(Q)for path in $(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/proc/*/; do PFX=p$$(basename "$${path}" | tr '[:upper:]' '[:lower:]'); ASM=$$path/$$PFX.S; OBJ=$(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/obj/$$PFX.o; LIB=$(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/lib/lib$$PFX.a; HDRS=$(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/include/proc/$$PFX.h; HDRD=$(TOOLS_DIR)/pic32-part-support-$(P32PS_VERSION)/meosinstall/include/proc/$$PFX.h; cd $$path; $(CC) $(CFLAGS) -c $$ASM -o $$OBJ; $(AR) rv $$LIB $$OBJ; cp $$HDRS $$HDRD; done
