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
#   Description:	Automated distribution library build rules
#
##########################################################################

SUPPORT_DIR = $(ABUILD_DIR)/support
CURL ?= $(shell which curl 2>/dev/null||echo false)
WGET ?= $(shell which wget 2>/dev/null||echo false)

ifeq (false,$(CURL))
ifeq (false,$(WGET))
	$(error Install curl or wget!)
endif
endif

LWIP_VERSION = lwip-2.0.1
LWIP_FILE = $(LWIP_VERSION).zip
CONFIG_LWIP_SRC ?= http://www.mirrorservice.org/sites/download.savannah.gnu.org/releases/lwip/
LWIP_URL = $(CONFIG_LWIP_SRC)$(LWIP_FILE)
LWIP_EXISTFILE := $(SUPPORT_DIR)/$(LWIP_VERSION)/README
$(LWIP_EXISTFILE):
	$(Q)mkdir -p $(SUPPORT_DIR)
	$(Q)echo $(call BANNER,DL,$(LWIP_VERSION))
	$(Q)rm -fr $(SUPPORT_DIR)/$(LWIP_VERSION) $(SUP)
	$(Q)mkdir -p $(SUPPORT_DIR) $(SUP)
	$(Q)$(CURL) -L $(LWIP_URL) -o $(SUPPORT_DIR)/$(LWIP_FILE) || $(WGET) $(LWIP_URL) -O $(SUPPORT_DIR)/$(LWIP_FILE)
	$(Q)echo $(call BANNER,EX,$(LWIP_VERSION))
	$(Q)unzip -d $(SUPPORT_DIR) $(SUPPORT_DIR)/$(LWIP_FILE) $(SUP)

LWIP_DIR = $(SUPPORT_DIR)/$(LWIP_VERSION)
LWIP_ARCH = $(WORK_DIR)/support/3/lwip

API_SRC = \
	$(LWIP_DIR)/src/api/api_lib.c \
	$(LWIP_DIR)/src/api/api_msg.c \
	$(LWIP_DIR)/src/api/err.c \
	$(LWIP_DIR)/src/api/netbuf.c \
	$(LWIP_DIR)/src/api/netdb.c \
	$(LWIP_DIR)/src/api/netifapi.c \
	$(LWIP_DIR)/src/api/sockets.c \
	$(LWIP_DIR)/src/api/tcpip.c

CORE_SRC = \
	$(LWIP_DIR)/src/core/def.c \
	$(LWIP_DIR)/src/core/dns.c \
	$(LWIP_DIR)/src/core/inet_chksum.c \
	$(LWIP_DIR)/src/core/init.c \
	$(LWIP_DIR)/src/core/ip.c \
	$(LWIP_DIR)/src/core/mem.c \
	$(LWIP_DIR)/src/core/memp.c \
	$(LWIP_DIR)/src/core/netif.c \
	$(LWIP_DIR)/src/core/pbuf.c \
	$(LWIP_DIR)/src/core/raw.c \
	$(LWIP_DIR)/src/core/stats.c \
	$(LWIP_DIR)/src/core/sys.c \
	$(LWIP_DIR)/src/core/tcp.c \
	$(LWIP_DIR)/src/core/tcp_in.c \
	$(LWIP_DIR)/src/core/tcp_out.c \
	$(LWIP_DIR)/src/core/timeouts.c \
	$(LWIP_DIR)/src/core/udp.c


CORE4_SRC = \
	$(LWIP_DIR)/src/core/ipv4/autoip.c \
	$(LWIP_DIR)/src/core/ipv4/dhcp.c \
	$(LWIP_DIR)/src/core/ipv4/etharp.c \
	$(LWIP_DIR)/src/core/ipv4/icmp.c \
	$(LWIP_DIR)/src/core/ipv4/igmp.c \
	$(LWIP_DIR)/src/core/ipv4/ip4_addr.c \
	$(LWIP_DIR)/src/core/ipv4/ip4.c \
	$(LWIP_DIR)/src/core/ipv4/ip4_frag.c

NETIF_SRC = \
	$(LWIP_DIR)/src/netif/ethernet.c \
	$(LWIP_DIR)/src/netif/ethernetif.c \
	$(LWIP_DIR)/src/netif/lowpan6.c \
	$(LWIP_DIR)/src/netif/slipif.c

# LWIP_SRC: All the above.
LWIP_SRC=$(API_SRC) $(CORE_SRC) $(CORE4_SRC) $(NETIF_SRC)
LWIP_OBJ:=$(patsubst $(LWIP_DIR)/%.c,$(ABUILD_DIR)/obj/lwip/%.o,$(filter %.c,$(LWIP_SRC)))

# LWIP_ARCH_SRC: Architecture specific files
LWIP_ARCH_SRC = \
	$(LWIP_ARCH)/sys_arch.c

LWIP_ARCH_OBJ:=$(patsubst $(LWIP_ARCH)/%.c,$(ABUILD_DIR)/obj/lwip/%.o,$(filter %.c,$(LWIP_ARCH_SRC)))

LWIP_LIBRARY_SHORT = $(ABUILD_DIR)/lib/liblwip.a
LWIP_LIBRARY = $(LWIP_LIBRARY_SHORT)($(LWIP_OBJ) $(LWIP_ARCH_OBJ))

LWIP_INCLUDES = \
	-I$(WORK_DIR)/drivers/include/ \
	-I$(LWIP_DIR)/src/include/ \
	-I$(LWIP_DIR)/src/include/ipv4/ \
	-I$(LWIP_ARCH) \
	-I$(LWIP_ARCH)/include/ \
	-I$(LWIP_ARCH)/include/arch/

LWIP_CFLAGS = -Wno-address

.PRECIOUS: $(ABUILD_DIR)/obj/lwip/%.o
$(ABUILD_DIR)/obj/lwip/%.o: $(LWIP_DIR)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) $(LWIP_CFLAGS) $(LWIP_INCLUDES) -DLWIP_DEBUG=1 -c $< -o $@
$(ABUILD_DIR)/obj/lwip/%.o: $(LWIP_ARCH)/%.c
	$(Q)mkdir -p $(dir $@)
	$(Q)echo $(call BANNER,CC,$<)
	$(Q)cd $(dir $@);$(CC) $(PRIVATE_CFLAGS) $(CFLAGS) $(INC) $(LWIP_CFLAGS) $(LWIP_INCLUDES) -DLWIP_DEBUG=1 -c $< -o $@

.PHONY: lwip
lwip: $(LWIP_EXISTFILE) $(LWIP_LIBRARY)
	$(Q)echo $(call BANNER,LWIP,$@)

.PRECIOUS: $(LWIP_DIR)/%
$(LWIP_DIR)/%: $(LWIP_EXISTFILE)
	$(Q)mkdir -p $(dir $@)


$(ABUILD_DIR)/include/%.h: $(LWIP_DIR)/src/include/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)

$(ABUILD_DIR)/include/%.h: $(LWIP_ARCH)/include/%.h
	$(Q)echo $(call BANNER,INSH,$(<))
	$(Q)mkdir -p $(dir $@)
	$(Q)cp -f $< $@ $(SUP)
	$(Q)chmod gou+x $@ $(SUP)


ifdef CONFIG_LWIP
LFLAGS += -llwip
EXTRA_TARGETS += lwip
MISC_PREP += $(LWIP_EXISTFILE)
INSTALLED_HEADERS += \
	$(ABUILD_DIR)/include/lwip/api.h \
	$(ABUILD_DIR)/include/lwip/icmp6.h \
	$(ABUILD_DIR)/include/lwip/ip_addr.h \
	$(ABUILD_DIR)/include/lwip/raw.h \
	$(ABUILD_DIR)/include/lwip/arch.h \
	$(ABUILD_DIR)/include/lwip/icmp.h \
	$(ABUILD_DIR)/include/lwip/ip.h \
	$(ABUILD_DIR)/include/lwip/sio.h \
	$(ABUILD_DIR)/include/lwip/autoip.h \
	$(ABUILD_DIR)/include/lwip/igmp.h \
	$(ABUILD_DIR)/include/lwip/mem.h \
	$(ABUILD_DIR)/include/lwip/snmp.h \
	$(ABUILD_DIR)/include/lwip/debug.h \
	$(ABUILD_DIR)/include/lwip/inet_chksum.h \
	$(ABUILD_DIR)/include/lwip/memp.h \
	$(ABUILD_DIR)/include/lwip/sockets.h \
	$(ABUILD_DIR)/include/lwip/def.h \
	$(ABUILD_DIR)/include/lwip/inet.h \
	$(ABUILD_DIR)/include/lwip/mld6.h \
	$(ABUILD_DIR)/include/lwip/stats.h \
	$(ABUILD_DIR)/include/lwip/dhcp6.h \
	$(ABUILD_DIR)/include/lwip/init.h \
	$(ABUILD_DIR)/include/lwip/nd6.h \
	$(ABUILD_DIR)/include/lwip/sys.h \
	$(ABUILD_DIR)/include/lwip/dhcp.h \
	$(ABUILD_DIR)/include/lwip/ip4_addr.h \
	$(ABUILD_DIR)/include/lwip/netbuf.h \
	$(ABUILD_DIR)/include/lwip/tcp.h \
	$(ABUILD_DIR)/include/lwip/dns.h \
	$(ABUILD_DIR)/include/lwip/ip4_frag.h \
	$(ABUILD_DIR)/include/lwip/netdb.h \
	$(ABUILD_DIR)/include/lwip/tcpip.h \
	$(ABUILD_DIR)/include/lwip/err.h \
	$(ABUILD_DIR)/include/lwip/ip4.h \
	$(ABUILD_DIR)/include/lwip/netifapi.h \
	$(ABUILD_DIR)/include/lwip/timeouts.h \
	$(ABUILD_DIR)/include/lwip/errno.h \
	$(ABUILD_DIR)/include/lwip/ip6_addr.h \
	$(ABUILD_DIR)/include/lwip/netif.h \
	$(ABUILD_DIR)/include/lwip/udp.h \
	$(ABUILD_DIR)/include/lwip/etharp.h \
	$(ABUILD_DIR)/include/lwip/ip6_frag.h \
	$(ABUILD_DIR)/include/lwip/opt.h \
	$(ABUILD_DIR)/include/lwip/ethip6.h \
	$(ABUILD_DIR)/include/lwip/ip6.h \
	$(ABUILD_DIR)/include/lwip/pbuf.h \
	$(ABUILD_DIR)/include/lwip/priv/api_msg.h \
	$(ABUILD_DIR)/include/lwip/priv/memp_priv.h \
	$(ABUILD_DIR)/include/lwip/priv/memp_std.h \
	$(ABUILD_DIR)/include/lwip/priv/nd6_priv.h \
	$(ABUILD_DIR)/include/lwip/priv/tcpip_priv.h \
	$(ABUILD_DIR)/include/lwip/priv/tcp_priv.h \
	$(ABUILD_DIR)/include/lwip/prot/autoip.h \
	$(ABUILD_DIR)/include/lwip/prot/dhcp.h \
	$(ABUILD_DIR)/include/lwip/prot/dns.h \
	$(ABUILD_DIR)/include/lwip/prot/etharp.h \
	$(ABUILD_DIR)/include/lwip/prot/ethernet.h \
	$(ABUILD_DIR)/include/lwip/prot/icmp6.h \
	$(ABUILD_DIR)/include/lwip/prot/icmp.h \
	$(ABUILD_DIR)/include/lwip/prot/igmp.h \
	$(ABUILD_DIR)/include/lwip/prot/ip4.h \
	$(ABUILD_DIR)/include/lwip/prot/ip6.h \
	$(ABUILD_DIR)/include/lwip/prot/ip.h \
	$(ABUILD_DIR)/include/lwip/prot/mld6.h \
	$(ABUILD_DIR)/include/lwip/prot/nd6.h \
	$(ABUILD_DIR)/include/lwip/prot/tcp.h \
	$(ABUILD_DIR)/include/lwip/prot/udp.h \
	$(ABUILD_DIR)/include/lwipopts.h \
	$(ABUILD_DIR)/include/arch/cc.h \
	$(ABUILD_DIR)/include/arch/perf.h \
	$(ABUILD_DIR)/include/arch/sys_arch.h
CLEAN_EXTRA += $(LWIP_LIBRARY_SHORT)
endif
