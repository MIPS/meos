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
*   Description:        FatFs/MASS shim
*
*************************************************************************/

#include "MEOS.h"
#include "diskio.h"

DSTATUS disk_initialize(BYTE pdrv) __attribute__ ((alias("disk_combo")));
DSTATUS disk_status(BYTE pdrv) __attribute__ ((alias("disk_combo")));
DSTATUS disk_combo(BYTE pdrv)
{

	MASS_T *mass = MASS_device(pdrv);
	int32_t status = MASS_stat(mass, NULL, NULL, NULL);
	return (((status & MSF_REMOVED) ==
		 MSF_REMOVED) ? STA_NODISK : 0) | (((status & MSF_RO) ==
						    MSF_RO) ? STA_PROTECT : 0);
}

DRESULT disk_read(BYTE pdrv, BYTE * buff, DWORD sector, UINT count)
{
	MASS_T *mass = MASS_device(pdrv);
	uint64_t size;
	MASS_lb(mass, 0, NULL, &size);
	int32_t stat = MASS_stat(mass, NULL, NULL, NULL);
	if (((size == 512) || (size == 1024) || (size == 2048)
	     || (size == 4096)) && ((stat & MSF_L_ISO) == MSF_L_ISO)) {
		/* Hard sectored */
		if (MASS_read(mass, buff, sector, 0, size * count) < 0)
			return RES_ERROR;
		else
			return RES_OK;
	} else {
		/* Simulate 512 byte sectors */
		if (MASS_read(mass, buff, 0, sector * 512, count * 512) < 0)
			return RES_ERROR;
		else
			return RES_OK;
	}
}

/* THIS KILLS FLASH: but you accepted that happens when you chose FAT */
static int32_t _disk_partialErase(MASS_T * mass,
				  uint64_t unaligned_lba_to_erase,
				  size_t offset_in_unaligned_lba_to_erase,
				  size_t bytes_to_erase)
{
	int32_t r = 1;
	uint64_t first_eba_to_erase, offset_in_first_eba_to_erase,
	    first_lba_to_erase, offset_in_first_lba_to_erase;
	uint64_t last_eba_to_erase, offset_in_last_eba_to_erase,
	    last_lba_to_erase, offset_in_last_lba_to_erase;
	uint64_t erase_block_size;

	/* Convert logical blocks to erase blocks */
	MASS_lbaToEba(mass, unaligned_lba_to_erase,
		      offset_in_unaligned_lba_to_erase, &first_eba_to_erase,
		      &offset_in_first_eba_to_erase);
	MASS_lbaToEba(mass, unaligned_lba_to_erase,
		      offset_in_unaligned_lba_to_erase + bytes_to_erase,
		      &last_eba_to_erase, &offset_in_last_eba_to_erase);

	/* Convert back for R/W */
	if (MASS_ebaToLba
	    (mass, first_eba_to_erase, 0, &first_lba_to_erase,
	     &offset_in_first_lba_to_erase) < 0)
		goto err;
	if (MASS_ebaToLba
	    (mass, last_eba_to_erase, offset_in_last_eba_to_erase + 1,
	     &last_lba_to_erase, &offset_in_last_lba_to_erase) < 0)
		goto err;
	if (MASS_eb(mass, last_eba_to_erase, NULL, &erase_block_size) < 0)
		goto err;

	/* Allocate erase buffer */
	uint8_t *ebuf =
	    alloca(((offset_in_first_eba_to_erase - 1) >
		    (erase_block_size - offset_in_last_eba_to_erase))
		   ? (offset_in_first_eba_to_erase - 1) : (erase_block_size -
							   offset_in_last_eba_to_erase));

	/* Handle partial first block */
	if (offset_in_first_eba_to_erase) {
		/* Read data to preserve from erase block */
		if (MASS_read
		    (mass, ebuf, first_lba_to_erase,
		     offset_in_first_lba_to_erase,
		     offset_in_first_eba_to_erase - 1) < 0)
			goto err;
		/* Erase erase block */
		if (MASS_erase(mass, first_eba_to_erase, first_eba_to_erase) <
		    0)
			r = -1;
		/* Write back partial data */
		if (MASS_write
		    (mass, ebuf, first_lba_to_erase,
		     offset_in_first_lba_to_erase,
		     offset_in_first_eba_to_erase - 1) < 0)
			goto err;
		first_eba_to_erase++;
	}

	/* Handle partial last block */
	if ((r > 0)
	    && (offset_in_last_eba_to_erase != erase_block_size - 1)) {
		/* Read erase block */
		if (MASS_read
		    (mass, ebuf, last_lba_to_erase,
		     offset_in_last_lba_to_erase + offset_in_last_eba_to_erase,
		     erase_block_size - offset_in_last_eba_to_erase) < 0)
			goto err;
		/* Erase erase block */
		if (MASS_erase(mass, last_eba_to_erase, last_eba_to_erase) < 0)
			r = -1;
		/* Write back partial data */
		if (MASS_write
		    (mass, ebuf, last_lba_to_erase,
		     offset_in_last_lba_to_erase + offset_in_last_eba_to_erase,
		     erase_block_size - offset_in_last_eba_to_erase) < 0)
			goto err;
		last_eba_to_erase--;
	}

	/* If there's anything left */
	if ((r > 0) && (first_eba_to_erase < last_eba_to_erase)) {
		/* Erase intervening blocks */
		if (MASS_erase(mass, first_eba_to_erase, last_eba_to_erase) < 0)
			goto err;
	}
	return r;
      err:
	return -1;
}

DRESULT disk_write(BYTE pdrv, const BYTE * buff, DWORD sector, UINT count)
{
	MASS_T *mass = MASS_device(pdrv);
	uint64_t size;
	MASS_lb(mass, 0, NULL, &size);
	int32_t stat = MASS_stat(mass, NULL, NULL, NULL);
	if (((size == 512) || (size == 1024) || (size == 2048)
	     || (size == 4096))
	    && ((stat & MSF_L_ISO) == MSF_L_ISO)) {
		/* Hard sectored */
		if ((stat & MSF_EB4W) == MSF_EB4W) {
			/* Erase required */
			if (_disk_partialErase(mass, sector, 0, count * size) <
			    0)
				return RES_ERROR;
		}
		/* Write */
		if (MASS_write(mass, buff, sector, 0, count * size) < 0)
			return RES_ERROR;
		else
			return RES_OK;
	} else {
		/* Simulate 512 byte sectors */
		if ((stat & MSF_EB4W) == MSF_EB4W) {
			/* Erase required */
			if (_disk_partialErase
			    (mass, 0, sector * 512, count * 512) < 0)
				return RES_ERROR;
		}
		/* Write */
		if (MASS_write(mass, buff, 0, sector * 512, count * 512) < 0)
			return RES_ERROR;
		else
			return RES_OK;
	}
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buf)
{
	MASS_T *mass = MASS_device(pdrv);
	uint64_t size;
	MASS_lb(mass, 0, NULL, &size);
	uint64_t lbs;
	switch (cmd) {
	case CTRL_SYNC:
		MASS_sync(mass);
		return RES_OK;
	case GET_SECTOR_COUNT:
		if (((size == 512)
		     || (size == 1024)
		     || (size == 2048)
		     || (size == 4096))
		    && ((MASS_stat(mass, &lbs, NULL, NULL) & MSF_L_ISO) ==
			MSF_L_ISO))
			/* Hard sectored */
			*(uint32_t *) buf = lbs;
		else
			/* Simulate 512 byte sectors */
			MASS_stat(mass, NULL, NULL, &size);
		*(uint32_t *) buf = size / 512;
		return RES_OK;
	case GET_SECTOR_SIZE:
		if (((size == 512)
		     || (size == 1024)
		     || (size == 2048)
		     || (size == 4096))
		    && ((MASS_stat(mass, &lbs, NULL, NULL) & MSF_L_ISO) ==
			MSF_L_ISO))
			/* Hard sectored */
			*(uint32_t *) buf = size;
		else
			/* Simulate sectors */
			*(uint32_t *) buf = 512;
		return RES_OK;
	case GET_BLOCK_SIZE:
		/* This is only used for partitioning */
		if ((MASS_stat(mass, NULL, NULL, NULL) & MSF_EL_ISO) ==
		    MSF_EL_ISO) {
			MASS_eb(mass, 0, NULL, &size);
			*(uint32_t *) buf = size;
		} else {
			*(uint32_t *) buf = 1;
		}
		return RES_OK;
	case CTRL_TRIM:
		/* This is only a hint, and only meaningful on hard drive like devices */
		if (((size == 512)
		     || (size == 1024)
		     || (size == 2048)
		     || (size == 4096))
		    && ((MASS_stat(mass, &lbs, NULL, NULL) &
			 (MSF_L_ISO | MSF_EB4W))
			== MSF_L_ISO)) {
			uint64_t first, last;
			uint64_t foff, loff, lsize, esize;
			/* Convert sectors to erase blocks */
			MASS_lb(mass, ((uint32_t *) buf)[1], NULL, &lsize);
			MASS_lbaToEba(mass, ((uint32_t *)
					     buf)[0], 0, &first, &foff);
			MASS_lbaToEba(mass, ((uint32_t *)
					     buf)[1], lsize - 1, &last, &loff);
			/* Avoid partially used blocks */
			if (foff)
				first++;
			MASS_eb(mass, last, NULL, &esize);
			if (loff != esize - 1)
				last--;
			/* If there's anything left, erase */
			if (first < last) {
				if (MASS_erase(mass, first, last) < 0)
					return RES_PARERR;
				else
					return RES_OK;
			}

			return RES_OK;
		} else {
			/* Don't bother, probably not meaningful */
			return RES_OK;
		}
	default:
		return RES_ERROR;
	}

}
