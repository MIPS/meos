/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	MRF24G driver
*
*************************************************************************/

#ifndef MRF_TIMER_H
#define MRF_TIMER_H

#include "mrf_com.h"

#include "MEOS.h"

static inline uint32_t MRF_count_get(void)
{
    return _mips_mfc0(C0_COUNT);
}

static inline uint32_t MRF_usdif(const uint32_t counta,
    const uint32_t countb)
{
    return (counta - countb) / ((SYS_CLK / 2) / 1000000);
}

static inline uint32_t MRF_usadd(const uint32_t count, const uint32_t us)
{
    return count + (us * ((SYS_CLK / 2) / 1000000));
}

static inline int MRF_timed_out(const uint32_t start_count,
    const uint32_t timeout)
{
    return (MRF_usdif(MRF_count_get(), start_count) >= timeout) ? 1 : 0;
}

#endif
