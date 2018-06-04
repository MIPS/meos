/***(C)2013***************************************************************
*
* Copyright (C) 2013 MIPS Tech, LLC
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
****(C)2013**************************************************************/

/*************************************************************************
*
*   Description:	Linux memory abstraction specialisation
*
*************************************************************************/

#include "meos/debug/dbg.h"
#include "meos/mem/mem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

/*
** FUNCTION:	MEM_v2p
**
** DESCRIPTION:	Convert virtual to physical
**
** RETURNS:	uintptr_t
*/
uintptr_t MEM_v2p(void *vaddr)
{
	/* Fail - not meaningful */
	return 0;
}

/*
** FUNCTION:	MEM_p2v
**
** DESCRIPTION:	Convert physical to virtual
**
** RETURNS:	void *
*/
void *MEM_p2v(uintptr_t paddr, MEM_P2V_VIEW_T view)
{
	/* Fail - not meaningful */
	return NULL;
}

/*
** FUNCTION:	MEM_map
**
** DESCRIPTION:	Map physical into virtual
**
** RETURNS:	int32_t
*/
int32_t MEM_map(uintptr_t paddr, size_t length, void *vaddr)
{
	static int memfd = 0;
	int err;
	if (!memfd) {
		memfd = open("/dev/mem", O_RDWR);
		err = errno;
		(void)err;
		DBG_assert(memfd != -1, "Can not open /dev/mem: %s",
			   strerror(err));
	}
	return vaddr == mmap(vaddr, length, PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_FIXED, memfd, paddr);
}

int32_t MEM_revmap(void *vaddr, size_t length, MEM_REVMAPFUNC_T cb, void *cbPar)
{
	/* Fail - not meaningful */
	return 0;
}
