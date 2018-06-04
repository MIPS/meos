####(C)2016###############################################################
#
# Copyright (C) 2016 MIPS Tech, LLC
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
####(C)2016###############################################################

MRF24WG_SRC += \
	support/1/drivers/mrf24g/mrf_com.c \
	support/1/drivers/mrf24g/mrf_connection.c \
	support/1/drivers/mrf24g/mrf_mgmt.c \
	support/1/drivers/mrf24g/mrf_raw.c \
	support/1/drivers/mrf24g/mrf_reg.c \
	support/1/drivers/mrf24g/mrf.c \
	support/1/drivers/mrf24g/mrfnetif.c

ifdef CONFIG_DRIVER_MRF24G
SRC += $(MRF24WG_SRC)

$(ABUILD_DIR)/include/meos/mrf24g/%.h: $(MEOS_SRC)/support/1/drivers/mrf24g/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

INSTALLED_HEADERS += \
	$(ABUILD_DIR)/include/meos/mrf24g/mrf_const.h \
	$(ABUILD_DIR)/include/meos/mrf24g/mrf_raw.h \

XMLDOCS += \
	support/1/drivers/mrf24g/mrf.xml \
	support/1/drivers/mrf24g/mrfnetif.xml \
	support/1/drivers/mrf24g/mrf_connection.xml

endif
