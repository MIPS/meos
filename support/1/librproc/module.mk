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
#   Description:	librproc build rules
#
##########################################################################

LIBRPROC_SRC=support/1/librproc/rproc.c
LIBRPROC_ASMSRC=support/1/librproc/_copro_start.S
LIBRPROC_COBJ:=$(patsubst %.c,$(ABUILD_DIR)/obj/%.o,$(filter %.c,$(LIBRPROC_SRC)))
LIBRPROC_SOBJ:=$(patsubst %.s,$(ABUILD_DIR)/obj/%.o,$(filter %.s,$(LIBRPROC_ASMSRC))) $(patsubst %.S,$(ABUILD_DIR)/obj/%.o,$(filter %.S,$(LIBRPROC_ASMSRC)))
LIBRPROC_OBJ=$(LIBRPROC_COBJ) $(LIBRPROC_SOBJ)

XMLDOCS += \
	support/1/librproc/res.xml

LIBRPROC_LIBRARY_SHORT=$(ABUILD_DIR)/lib/librproc.a
LIBRPROC_LIBRARY=$(LIBRPROC_LIBRARY_SHORT)($(LIBRPROC_OBJ))

.PRECIOUS: $(LIBRPROC_OBJ)

.PHONY: librproc
librproc: $(LIBRPROC_LIBRARY)

ifndef CONFIG_ARCH_LINUX
ifdef CONFIG_FEATURE_VRINGS
EXTRA_TARGETS += librproc
CLEAN_EXTRA += $(LIBRPROC_LIBRARY_SHORT)
endif
endif
