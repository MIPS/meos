####(C)2012###############################################################
#
# Copyright (C) 2012 MIPS Tech, LLC
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
####(C)2012###############################################################
#
#   Description:	Master test Makefile
#
##########################################################################

# Tests that should run on any target
TESTS_BASIC += $(call ADDTEST, regression/lsttest)
TESTS_BASIC += $(call ADDTEST, regression/dqtest)
TESTS_BASIC += $(call ADDTEST, regression/tretest)
TESTS_BASIC += $(call ADDTEST, regression/cppbuild)
TESTS_BASIC += $(call ADDTEST, regression/warning)
TESTS_BASIC += $(call ADDTEST, regression/noisy_stack)
TESTS_BASIC += $(call ADDTEST, regression/stack_overflow)
TESTS_BASIC += $(call ADDTEST, regression/infwait)
TESTS_BASIC += $(call ADDTEST, regression/timisrorder)
TESTS_BASIC += $(call ADDTEST, regression/takepool_int)
TESTS_BASIC += $(call ADDTEST, regression/error)
TESTS_BASIC += $(call ADDTEST, regression/coverage)
TESTS_BASIC += $(call ADDTEST, regression/lazy)
TESTS_BASIC += $(call ADDTEST, regression/isr_schedule)
TESTS_BASIC += $(call ADDTEST, regression/unaligned_task)
TESTS_BASIC += $(call ADDTEST, regression/raiseipl_timeout)
TESTS_BASIC += $(call ADDTEST, regression/intsynth)
TESTS_BASIC += $(call ADDTEST, regression/softint)
TESTS_BASIC += $(call ADDTEST, regression/sync)

TESTS_FPU += $(call ADDTEST, regression/FPU_paranoia)
TESTS_FPU += $(call ADDTEST, regression/FPU_paranoia_single)
TESTS_FPU += $(call ADDTEST, regression/mtmath)

TESTS_IMPEXP += $(call ADDTEST, regression/interThread)
TESTS_IMPEXP += $(call ADDTEST, regression/mbxstress)

TESTS_METRICS += $(call ADDTEST, regression/timing)
TESTS_METRICS += $(call ADDTEST, regression/size)
TESTS_METRICS += $(call ADDTEST, regression/usage)

ifdef CONFIG_FEATURE_FLOAT
ALL_TESTS += $(TESTS_FPU)
endif

ifdef CONFIG_FEATURE_VRINGS
ALL_TESTS += $(TESTS_VRINGS)
endif

ifdef CONFIG_FEATURE_IMPEXP
ALL_TESTS += $(TESTS_IMPEXP)
endif

ALL_TESTS += $(TESTS_BASIC) $(TESTS_METRICS)
