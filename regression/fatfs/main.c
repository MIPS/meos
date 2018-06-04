#include <stdio.h>
#include <string.h>
#include "MEOS.h"
#include "ff.h"

#define STACKSIZE 2000
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)

static KRN_TASK_T *bgtask;
static KRN_SCHEDULE_T sched;
static KRN_TASKQ_T schedQueues[PRIORITIES];
static uint32_t istack[STACKSIZE];
static uint32_t tstack[STACKSIZE];

void report(FRESULT res)
{
	const char *errors[] = {
		"FR_OK ([0] Succeeded)",
		"FR_DISK_ERR ([1] A hard error occurred in the low level disk I/O layer)",
		"FR_INT_ERR ([2] Assertion failed)",
		"FR_NOT_READY ([3] The physical drive cannot work)",
		"FR_NO_FILE ([4] Could not find the file)",
		"FR_NO_PATH ([5] Could not find the path)",
		"FR_INVALID_NAME ([6] The path name format is invalid)",
		"FR_DENIED ([7] Access denied due to prohibited access or directory full)",
		"FR_EXIST ([8] Access denied due to prohibited access)",
		"FR_INVALID_OBJECT ([9] The file/directory object is invalid)",
		"FR_WRITE_PROTECTED ([10] The physical drive is write protected)",
		"FR_INVALID_DRIVE ([11] The logical drive number is invalid)",
		"FR_NOT_ENABLED ([12] The volume has no work area)",
		"FR_NO_FILESYSTEM ([13] There is no valid FAT volume)",
		"FR_MKFS_ABORTED ([14] The f_mkfs() aborted due to any parameter error)",
		"FR_TIMEOUT ([15] Could not get a grant to access the volume within defined period)",
		"FR_LOCKED ([16] The operation is rejected according to the file sharing policy)",
		"FR_NOT_ENOUGH_CORE ([17] LFN working buffer could not be allocated)",
		"FR_TOO_MANY_OPEN_FILES ([18] Number of open files > _FS_LOCK)",
		"FR_INVALID_PARAMETER ([19] Given parameter is invalid)"
	};
	if (res == 0)
		return;
	DBG_logF("%s", errors[res]);

	abort();
}

void dir(FATFS * fs, char *path)
{
	FRESULT res;
	DIR dir;
	FILINFO file;
	uint64_t files = 0, dirs = 0, bytes = 0;
	char buf[32];
	DWORD cls;

	report(f_opendir(&dir, path));

	DBG_logF(" Directory of %s\n\n", path);
	for (;;) {
		res = f_readdir(&dir, &file);
		if ((res != FR_OK) || (file.fname[0] == 0))
			break;
		if (file.fattrib & AM_DIR) {
			buf[7] = 0x20;
			buf[7] = 0x20;
			buf[7] = 0x20;
			buf[7] = 0x20;
			buf[7] = 0x20;
			buf[7] = 0x20;
			buf[6] = 0;
			dirs++;
		} else {
			sprintf(buf, "%6u", (unsigned int)file.fsize);
			files++;
			bytes += file.fsize;
		}
		DBG_logF("%04u/%02u/%02u\t%02u:%02u\t%s\t%s\t%s\n",
			 (unsigned int)(file.fdate > 9) + 1980,
			 (unsigned int)(file.fdate >> 5) & 0xf,
			 (unsigned int)file.fdate & 0x1f,
			 (unsigned int)file.ftime >> 11,
			 (unsigned int)(file.ftime >> 5) & 0x3f,
			 (file.fattrib & AM_DIR) ? "<DIR>" : "     ", buf,
			 file.fname);
	}
	f_closedir(&dir);
	report(f_getfree(path, &cls, &fs));
	DBG_logF("\t%4u File(s)\t%u bytes\n\t%4u Dir(s)\t%u bytes free\n",
		 (unsigned int)files, (unsigned int)bytes, (unsigned int)dirs,
		 (unsigned int)cls * fs->csize);

}

int main(int argc, char *argv[])
{
	KRN_TASKQ_T queue;
	FATFS fs;
	BYTE work[_MAX_SS];
	FIL fp;
	char *test = "This is a test\n";
	char rb[128];
	UINT transfer;

/* Initialise MeOS */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE,
		  NULL, 0);
	bgtask = KRN_startOS("Background Task");
	DQ_init(&queue);

	KRN_startTimerTask("timerTask", tstack, STACKSIZE);
	BSP_init();

	DBG_logF("0:> mount\n");
	report(f_mount(&fs, "0:", 0));
	DBG_logF("0:> format\n");
	report(f_mkfs("0:", FM_FAT, 0, work, sizeof(work)));
	DBG_logF("0:> dir\n");
	dir(&fs, "0:");
	DBG_logF("0:> echo test > test.txt\n");
	report(f_open(&fp, "0:\\test.txt", FA_CREATE_NEW | FA_WRITE));
	report(f_write(&fp, test, strlen(test), &transfer));
	report(f_close(&fp));
	DBG_logF("0:> type test.txt\n");
	report(f_open(&fp, "test.txt", FA_READ));
	report(f_read(&fp, rb, sizeof(rb) - 1, &transfer));
	DBG_logF("%s", rb);
	report(f_close(&fp));
	DBG_logF("0:> dir\n");
	dir(&fs, "0:");
	return 0;
}
