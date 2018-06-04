####(C)2011###############################################################
#
# Copyright (C) 2011 MIPS Tech, LLC
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
####(C)2011###############################################################
#
#   Description:        ISR entry test core
#
##########################################################################

#define CTX_AT          0
#define CTX_EPC         (CTX_AT+124)
	lw	t1, epcStow
	sw	t1, CTX_EPC(k0)
	la	t1, isrStamp
	mtc0	zero, C0_CAUSE	  	# Enough to get us out of the ISR - user space will fix this up
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	mfc0 	end0, C0_PERFCNT, 1	# Cycles
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	sw	end0, 0(t1)
