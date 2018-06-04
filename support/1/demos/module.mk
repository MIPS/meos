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
#   Description:	Master demo Makefile
#
##########################################################################

# Demos that should run on any target
DEMOS += $(call ADDDEMO, support/1/demos/eliza)
DEMOS += $(call ADDDEMO, support/1/demos/timed_paranoia)
DEMOS += $(call ADDDEMO, support/1/demos/web_page)
DEMOS += $(call ADDDEMO, support/1/demos/tftp_demo)
DEMOS += $(call ADDDEMO, support/1/demos/srtc_demo)

# Demos for specific targets
DEMOS += $(call ADDDEMO, support/1/demos/mipsFPGA_demo)
DEMOS += $(call ADDDEMO, support/1/demos/ckw_gpio)
DEMOS += $(call ADDDEMO, support/1/demos/mrf24g)

DEMOS += $(call ADDDEMO, support/1/demos/mvz_static)
DEMOS += $(call ADDDEMO, support/1/demos/mvz_dynamic)
DEMOS += $(call ADDDEMO, support/1/demos/mvz_comms)
DEMOS += $(call ADDDEMO, support/1/demos/hello)
DEMOS += $(call ADDDEMO, support/1/demos/simplexmit)
DEMOS += $(call ADDDEMO, support/1/demos/simplerecv)

ALL_DEMOS += $(DEMOS)
