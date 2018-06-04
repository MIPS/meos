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
*   Description:        Newtron porting layer
*
*************************************************************************/


#include "MEOS.h"

#include <errno.h>
#include <sys/types.h>
#include <string.h>

#include <string.h>
#include <errno.h>
#include "hal/hal_flash_int.h"
#include "fs/fs_if.h"

#include "MEOS.h"

extern void *nffs_file_mem;
extern void *nffs_block_entry_mem;
extern void *nffs_inode_mem;
extern void *nffs_cache_inode_mem;
extern void *nffs_cache_block_mem;
extern void *nffs_dir_mem;

void nffs_config_init(void);
void nffs_cache_clear(void);
int nffs_stats_init(void);
int nffs_misc_reset(void);

static int _NEWTRON_flash_read(uint32_t address, void *dst, uint32_t num_bytes);
static int _NEWTRON_flash_write(uint32_t address, const void *src,
				uint32_t num_bytes);
static int _NEWTRON_flash_erase_sector(uint32_t sector_address);
static int _NEWTRON_flash_sector_info(int idx, uint32_t * address,
				      uint32_t * sz);
static int _NEWTRON_flash_init(void);

static const struct hal_flash_funcs _NEWTRON_flashFuncs = {
	.hff_read = _NEWTRON_flash_read,
	.hff_write = _NEWTRON_flash_write,
	.hff_erase_sector = _NEWTRON_flash_erase_sector,
	.hff_sector_info = _NEWTRON_flash_sector_info,
	.hff_init = _NEWTRON_flash_init
};

static struct hal_flash _NEWTRON_flashDev = {
	.hf_itf = &_NEWTRON_flashFuncs,
	.hf_base_addr = 0,
	.hf_sector_cnt = 0xffffffff,
	.hf_size = 0xffff,
	.hf_align = 1
};

MASS_T *_NEWTRON_mass;
extern KRN_LOCK_T nffs_mutex;
extern const struct fs_ops nffs_ops;

const struct hal_flash *hal_bsp_flash_dev(uint8_t id)
{
	uint64_t eba, size;
	if ((id == 0) && _NEWTRON_mass
	    && (MASS_stat(_NEWTRON_mass, NULL, &eba, &size) >= 0)) {
		_NEWTRON_flashDev.hf_sector_cnt = eba;
		_NEWTRON_flashDev.hf_size = size;
	} else {

		return NULL;
	}
	return &_NEWTRON_flashDev;
}

static int
_NEWTRON_flash_write(uint32_t address, const void *src, uint32_t num_bytes)
{
	return MASS_write(_NEWTRON_mass, src, 0, address, num_bytes) < 0 ? -1 : 0;
}

static int _NEWTRON_flash_read(uint32_t address, void *dst, uint32_t num_bytes)
{
	return MASS_read(_NEWTRON_mass, dst, 0, address, num_bytes) < 0 ? -1 : 0;
}

static int _NEWTRON_flash_erase_sector(uint32_t sector_address)
{
	return MASS_erase(_NEWTRON_mass, sector_address, sector_address) < 0 ? -1 : 0;
}

static int _NEWTRON_flash_sector_info(int idx, uint32_t * address,
				      uint32_t * sz)
{
	uint64_t offset, size;
	if (MASS_eb(_NEWTRON_mass, idx, &offset, &size) < 0)
	return -1;
	*address = (uint32_t) offset;
	*sz = (uint32_t) size;
	return 0;
}

static int _NEWTRON_flash_init(void)
{
	return 0;
}

int32_t NEWTRON_init(MASS_T * mass, struct nffs_file files[], size_t n_files, struct nffs_inode_entry inodes[], size_t n_inodes, struct nffs_hash_entry blocks[], size_t n_blocks, struct nffs_cache_inode cache_inodes[], size_t n_cache_inodes, struct nffs_cache_block cache_blocks[], size_t n_cache_blocks, struct nffs_dir dirs[], size_t n_dirs)
{
	int32_t rc;

	_NEWTRON_mass = mass;


	nffs_config_init();
	nffs_cache_clear();
	rc = nffs_stats_init();
	if (rc != 0) {
	    return rc;
	}

	KRN_initLock(&nffs_mutex);

	nffs_file_mem = files;
	nffs_config.nc_num_files = n_files;
	nffs_inode_mem = inodes;
	nffs_config.nc_num_inodes = n_inodes;
	nffs_block_entry_mem = blocks;
	nffs_config.nc_num_blocks = n_blocks;
	nffs_cache_inode_mem = cache_inodes;
	nffs_config.nc_num_cache_inodes = n_cache_inodes;
	nffs_cache_block_mem = cache_blocks;
	nffs_config.nc_num_cache_blocks = n_cache_blocks;
	nffs_dir_mem = dirs;
	nffs_config.nc_num_dirs = n_dirs;
	rc = (int32_t)nffs_misc_reset();
	if (rc != 0)
		return rc;

	fs_register(&nffs_ops);

	return 0;
}
