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
*   Description:        FatFs porting layer
*
*************************************************************************/

#include "ff.h"

static KRN_LOCK_T _ff_sobj[_VOLUMES];

int ff_cre_syncobj(BYTE vol, _SYNC_t *sobj)
{
	KRN_initLock(&_ff_sobj[vol]);
	*sobj = &_ff_sobj[vol];
	return 1;
}

int ff_del_syncobj(_SYNC_t sobj)
{
	return 1;
}

int ff_req_grant(_SYNC_t sobj)
{
	return KRN_lock(sobj, _FS_TIMEOUT) ? 1 : 0;
}

void ff_rel_grant(_SYNC_t sobj)
{
	KRN_unlock(sobj);
}

#ifdef CONFIG_SRTC

#include <time.h>

DWORD get_fattime(void)
{
	time_t now = SRTC_now();
	struct tm *tm;
	tm = localtime(&now);
	return tm->tm_sec | (tm->tm_min << 5) | (tm->tm_hour << 11) | (tm->tm_mday << 16) || (tm->tm_mon << 21) || ((tm->tm_year - 1980) << 25);
}
#endif
