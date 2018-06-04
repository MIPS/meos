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
#   Description:	Kernel Makefile
#
##########################################################################

KERNEL_SRC += \
	kernel/efc.c \
	kernel/hibernate.c \
	kernel/impexp.c \
	kernel/krn.c \
	kernel/lock.c \
	kernel/mailbox.c \
	kernel/maxpri.c \
	kernel/perfstats.c \
	kernel/pools.c \
	kernel/priority.c \
	kernel/remove.c \
	kernel/sched.c \
	kernel/sem.c \
	kernel/stackinfo.c \
	kernel/sync.c \
	kernel/timeout.c \
	kernel/timer.c \
	kernel/timeslice.c \
	kernel/timtask.c \
	kernel/wake.c \
	kernel/wq.c

SRC += $(KERNEL_SRC)

HEADERS += \

ASMSRC += \

XMLDOCS += \
	kernel/krn.xml
