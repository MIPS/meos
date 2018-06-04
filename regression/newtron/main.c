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
 *   Description:	Newtron test code
 *
 *************************************************************************/

/*
 * This test checks that the Newtron module correctly reads from an archive
 */

#include "MEOS.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEOS.h"

#define TSTACKSIZE 2000		/* MEOS timer task stack size */
#define STACKSIZE 1000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

#define SECTOR_SIZE 512
#define SECTOR_COUNT 32

static KRN_TASK_T *bgtask;
static uint32_t timestack[TSTACKSIZE];
uint32_t istack[STACKSIZE];

static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];

unsigned char filedata0[] =
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test,\n"
    "this is a test, this is a test, this is a test, this is a test\n";

unsigned char filedata1[] =
    "this is test2, this is test2, this is test2,\n"
    "this is test2, this is test2, this is test2,\n"
    "this is test2, this is test2, this is test2,\n"
    "this is test2, this is test2, this is test2,\n"
    "this is test2, this is test2, this is test2\n";

unsigned char fs_buf[SECTOR_SIZE * SECTOR_COUNT];
MASSRAM_T ram;

struct nffs_file files[100];
struct nffs_inode_entry inodes[100];
struct nffs_hash_entry blocks[4];
struct nffs_cache_inode cache_inodes[4];
struct nffs_cache_block cache_blocks[64];
struct nffs_dir dirs[4];

/*
 ** FUNCTION:      main
 **
 ** DESCRIPTION:   C main program for thread 1
 **
 ** RETURNS:       int
 */

int main()
{
	DBG_logF("Newtron Test\n");

	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	bgtask = KRN_startOS("Background Task");
	KRN_startTimerTask("Timer Task", timestack, TSTACKSIZE);

	MASS_initRam(&ram, fs_buf, SECTOR_SIZE * SECTOR_COUNT, 0);
	if (NEWTRON_init
	    ((MASS_T *) & ram, files, 100, inodes, 100, blocks, 4, cache_inodes,
	     4, cache_blocks, 64, dirs, 4) != 0) {
		DBG_logF("Test failed, init error\n");
		return -1;
	}

	struct nffs_area_desc descs[] = {
		{0, 8192},
		{8192, 2048},
		{8192 + 2048, 2048},
		{0, 0},
	};

	/* format */
	if (nffs_format(descs) != 0) {
		DBG_logF("Test failed, format error\n");
		return -1;
	}

	struct fs_file *test_file;
	struct fs_file *test_file2;

	/* open file 1 */
	if (fs_open("/test.txt", FS_ACCESS_READ | FS_ACCESS_WRITE, &test_file)
	    != 0) {
		DBG_logF("Test failed, file open error\n");
		return -1;
	}

	if (fs_write(test_file, filedata0, strlen((char *)filedata0)) != 0) {
		DBG_logF("Test failed, file write error\n");
		return -1;
	}

	if (fs_seek(test_file, 0) != 0) {
		DBG_logF("Test failed, file seek error\n");
		return -1;
	}

	uint8_t out_data[sizeof(filedata0)];
	uint32_t out_len;

	if (fs_read(test_file, sizeof(filedata0), out_data, &out_len) != 0) {
		DBG_logF("Test failed, file read error\n");
		return -1;
	}

	if (out_len != strlen((char *)filedata0)) {
		DBG_logF("Test failed, file read length error\n");
		return -1;
	}

	out_data[strlen((char *)filedata0)] = 0;
	if (strcmp((char *)out_data, (char *)filedata0) != 0) {
		DBG_logF("Test failed, file read data error\n");
		return -1;
	}
	/* open file 2 */
	if (fs_open("/test2.txt", FS_ACCESS_READ | FS_ACCESS_WRITE, &test_file2)
	    != 0) {
		DBG_logF("Test failed, file open error\n");
		return -1;
	}

	if (fs_write(test_file2, filedata1, strlen((char *)filedata1)) != 0) {
		DBG_logF("Test failed, file write error\n");
		return -1;
	}

	if (fs_seek(test_file2, 0) != 0) {
		DBG_logF("Test failed, file seek error\n");
		return -1;
	}

	if (fs_read(test_file2, sizeof(filedata0), out_data, &out_len) != 0) {
		DBG_logF("Test failed, file read error\n");
		return -1;
	}

	if (out_len != strlen((char *)filedata1)) {
		DBG_logF("Test failed, file read length error\n");
		return -1;
	}

	out_data[strlen((char *)filedata1)] = 0;
	if (strcmp((char *)out_data, (char *)filedata1) != 0) {
		DBG_logF("Test failed, file read data error\n");
		return -1;
	}

	if (fs_close(test_file2) != 0) {
		DBG_logF("Test failed, file close error\n");
		return -1;
	}

	if (fs_close(test_file) != 0) {
		DBG_logF("Test failed, file close error\n");
		return -1;
	}

	if (nffs_detect(descs) == FS_ECORRUPT) {
		DBG_logF("Test failed, filesystem not detected\n");
		return -1;
	}

	DBG_logF("Newtron Test passed \n");
	return 0;
}
