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
*   Description:	Mass storage partition implementation
*
*************************************************************************/

#include "MEOS.h"

#ifdef CONFIG_MASS_PARTITION

/* Implementation where 1 logical block = 1 erase block = 1 byte */

int32_t MASS_partStat(MASS_T * mass, uint64_t * lbs, uint64_t * ebs,
		      uint64_t * size)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	int32_t stat = MASS_stat(part->parent, NULL, NULL, NULL);

	if (lbs)
		*lbs = part->nLbs;
	if (ebs)
		*ebs = part->nEbs;
	if (size)
		*size = part->size;

	return stat & (MSF_L_ISO | MSF_E_ISO | MSF_EL_ISO | MSF_EB4W |
		       MSF_SYNCABLE);
}

int32_t MASS_partSync(MASS_T * mass)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	return MASS_sync(part->parent);
}

int32_t MASS_partEject(MASS_T * mass)
{
	/* Meaningless - always fail */
	return -1;
}

int32_t MASS_partRead(MASS_T * mass, uint8_t * buffer, uint64_t lba,
		      uint64_t offset, uint64_t bytes)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	uint64_t fb, start;

	if (MASS_lb(part->parent, lba + part->firstLba, &start, NULL) < 0)
		return -1;
	fb = (start - part->firstByte) + part->firstOffset + offset;
	if ((fb > part->size)
	    || (fb + bytes > part->size))
		return -1;

	return MASS_read(mass, buffer, lba + part->firstLba,
			 offset + part->firstOffset, bytes);
}

int32_t MASS_partWrite(MASS_T * mass, const uint8_t * buffer, uint64_t lba,
		       uint64_t offset, uint64_t bytes)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	uint64_t fb, start;

	if (MASS_lb(part->parent, lba + part->firstLba, &start, NULL) < 0)
		return -1;
	fb = (start - part->firstByte) + part->firstOffset + offset;
	if ((fb > part->size)
	    || (fb + bytes > part->size))
		return -1;

	return MASS_write(mass, buffer, lba + part->firstLba,
			  offset + part->firstOffset, bytes);
}

int32_t MASS_partLbaToEba(MASS_T * mass, uint64_t lba,
			  uint64_t offset, uint64_t * eba, uint64_t * eOffset)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	uint64_t fb, start;

	if (MASS_lb(part->parent, lba + part->firstLba, &start, NULL) < 0)
		return -1;
	fb = (start - part->firstByte) + offset;
	if (fb > part->size)
		return -1;

	return MASS_lbaToEba(part->parent, lba + part->firstLba,
			     offset + part->firstOffset, eba, eOffset);
}

int32_t MASS_partEbaToLba(MASS_T * mass, uint64_t eba,
			  uint64_t offset, uint64_t * lba, uint64_t * lOffset)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	uint64_t fb, start;

	if (MASS_eb(part->parent, eba + part->firstEba, &start, NULL) < 0)
		return -1;
	fb = (start - part->firstByte) + offset;
	if (fb > part->size)
		return -1;

	return MASS_ebaToLba(part->parent, eba + part->firstEba, offset, lba,
			     lOffset);
}

int32_t MASS_partLb(MASS_T * mass, uint64_t lba, uint64_t * offset,
		    uint64_t * size)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	int32_t r;
	if (lba >= part->nLbs)
		return -1;

	r = MASS_eb(part->parent, lba + part->firstLba, offset, size);
	*offset -= part->firstByte;
	return r;
}

int32_t MASS_partEb(MASS_T * mass, uint64_t eba, uint64_t * offset,
		    uint64_t * size)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	int32_t r;
	if (eba >= part->nEbs)
		return -1;

	r = MASS_eb(part->parent, eba + part->firstEba, offset, size);
	*offset -= part->firstByte;
	return r;
}

int32_t MASS_partErase(MASS_T * mass, uint64_t firstEba, uint64_t lastEba)
{
	MASSPART_T *part = (MASSPART_T *) mass;
	if ((firstEba >= part->nEbs) || (lastEba >= part->nEbs))
		return -1;
	return MASS_erase(part->parent, firstEba + part->firstEba,
			  lastEba + part->firstEba);
}

const MASS_IMPL_T _MASSPART_impl = {
	.stat = &MASS_partStat,
	.sync = &MASS_partSync,
	.eject = &MASS_partEject,
	.read = &MASS_partRead,
	.write = &MASS_partWrite,
	.lbaToEba = &MASS_partLbaToEba,
	.ebaToLba = &MASS_partEbaToLba,
	.lb = &MASS_partLb,
	.eb = &MASS_partEb,
	.erase = &MASS_partErase
};

int32_t MASS_initPartition(MASSPART_T * part, MASS_T * parent,
			   uint64_t firstLba, uint64_t firstOffset,
			   uint64_t lastLba, uint64_t lastOffset)
{
	uint64_t fEba, lEba, eOffset, fStart, lStart;

	/* Clean it up */
	MASS_normaliseLba(parent, &firstLba, &firstOffset);
	MASS_normaliseLba(parent, &lastLba, &lastOffset);

	/* Check it's legit */
	if (MASS_lbaToEba(parent, firstLba, firstOffset, &fEba, &eOffset) < 0)
		goto err;
	DBG_insist(eOffset == 0,
		   "Partition extent must resolve to complete erase blocks!\n");
	if (eOffset)
		goto err;
	if (MASS_lbaToEba(parent, lastLba, lastOffset, &lEba, &eOffset) < 0)
		goto err;
	DBG_insist(eOffset == 0,
		   "Partition extent must resolve to complete erase blocks!\n");
	if (eOffset)
		goto err;

	/* Fetch byte offsets */
	MASS_lb(parent, firstLba, &fStart, NULL);
	MASS_lb(parent, lastLba, &lStart, NULL);

	/* Stash working data */
	part->mass.impl = &_MASSPART_impl;
	part->parent = parent;
	part->firstEba = fEba;
	part->firstLba = firstLba;
	part->firstOffset = firstOffset;
	part->firstByte = fStart + firstOffset;
	part->nLbs = lastLba - firstLba;
	part->nEbs = lEba - fEba;
	part->size = (lStart + lastOffset) - (fStart + firstOffset);

	return 0;
      err:
	return -1;
}

#endif
