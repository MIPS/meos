/***(C)2016***************************************************************
*
* Copyright (C) 2016 MIPS Tech, LLC
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
****(C)2016**************************************************************/

/*************************************************************************
*
*   Description:	Mass storage RAM implementation
*
*************************************************************************/

#include "MEOS.h"

#ifdef CONFIG_MASS_RAM

/* Implementation where 1 logical block = 1 erase block = 1 byte */

int32_t MASS_ramStat(MASS_T * mass, uint64_t * lbs, uint64_t * ebs,
		     uint64_t * size)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (lbs)
		*lbs = ram->size;
	if (ebs)
		*ebs = ram->size;
	if (size)
		*size = ram->size;
	return MSF_L_ISO | MSF_E_ISO | MSF_EL_ISO | ram->ro;
}

int32_t MASS_ramSync(MASS_T * mass)
{
	/* Meaningless - always succeed */
	return 0;
}

int32_t MASS_ramEject(MASS_T * mass)
{
	/* Meaningless - always fail */
	return -1;
}

int32_t MASS_ramRead(MASS_T * mass, uint8_t * buffer, uint64_t lba,
		     uint64_t offset, uint64_t bytes)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if ((lba + offset > ram->size - 1)
	    || (lba + offset + bytes > ram->size))
		return -1;
	memcpy(buffer, ram->data + lba + offset, bytes);
	return 0;
}

int32_t MASS_ramWrite(MASS_T * mass, const uint8_t * buffer, uint64_t lba,
		      uint64_t offset, uint64_t bytes)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if ((lba + offset > ram->size - 1)
	    || (lba + offset + bytes > ram->size - 1))
		return -1;
	memcpy(ram->data + lba + offset, buffer, bytes);
	return 0;
}

int32_t MASS_ramLbaToEba(MASS_T * mass, uint64_t lba,
			 uint64_t offset, uint64_t * eba, uint64_t * eOffset)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (eba)
		*eba = lba;
	if (eOffset)
		*eOffset = offset;
	return lba < ram->size ? 1 : 0;
}

int32_t MASS_ramEbaToLba(MASS_T * mass, uint64_t eba,
			 uint64_t offset, uint64_t * lba, uint64_t * lOffset)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (lba)
		*lba = eba;
	if (lOffset)
		*lOffset = offset;
	return eba < ram->size ? 1 : 0;
}

int32_t MASS_ramLb(MASS_T * mass, uint64_t lba, uint64_t * offset,
		   uint64_t * size)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (offset)
		*offset = lba;
	if (size)
		*size = 1;
	return lba < ram->size ? (ram->ro ? MSF_RO : 0) : -1;
}

int32_t MASS_ramEb(MASS_T * mass, uint64_t eba, uint64_t * offset,
		   uint64_t * size)
{
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (offset)
		*offset = eba;
	if (size)
		*size = 1;
	return eba < ram->size ? 0 : -1;
}

int32_t MASS_ramErase(MASS_T * mass, uint64_t firstEba, uint64_t lastEba)
{
	/* Fill with 0xff so it looks like freshly erased flash */
	MASSRAM_T *ram = (MASSRAM_T *) mass;
	if (lastEba >= ram->size)
		return -1;
	memset(ram->data + firstEba, 0xff, 1 + lastEba - firstEba);
	return 0;
}

const MASS_IMPL_T _MASSRAM_impl = {
	.stat = &MASS_ramStat,
	.sync = &MASS_ramSync,
	.eject = &MASS_ramEject,
	.read = &MASS_ramRead,
	.write = &MASS_ramWrite,
	.lbaToEba = &MASS_ramLbaToEba,
	.ebaToLba = &MASS_ramEbaToLba,
	.lb = &MASS_ramLb,
	.eb = &MASS_ramEb,
	.erase = &MASS_ramErase
};

int32_t MASS_initRam(MASSRAM_T * ram, void *data, uint64_t size,
		     int32_t readonly)
{
	ram->mass.impl = &_MASSRAM_impl;
	ram->data = data;
	ram->size = size;
	ram->ro = readonly ? MSF_RO : 0;
	return 0;
}

#endif
