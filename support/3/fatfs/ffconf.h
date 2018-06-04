#include "MEOS.h"

#ifndef FFCONF_H
#define FFCONF_H

#define _FFCONF			68300
#ifdef CONFIG_FATFS_RO
#define _FS_READONLY		1
#else
#define _FS_READONLY		0
#endif
#ifdef CONFIG_FATFS_MIN_0
#define _FS_MINIMIZE		0
#endif
#ifdef CONFIG_FATFS_MIN_1
#define _FS_MINIMIZE		1
#endif
#ifdef CONFIG_FATFS_MIN_2
#define _FS_MINIMIZE		2
#endif
#ifdef CONFIG_FATFS_MIN_3
#define _FS_MINIMIZE		3
#endif
#ifdef CONFIG_FATFS_STR_0
#define _USE_STRFUNC		0
#endif
#ifdef CONFIG_FATFS_STR_1
#define _USE_STRFUNC		1
#endif
#ifdef CONFIG_FATFS_STR_2
#define _USE_STRFUNC		2
#endif
#ifdef CONFIG_FATFS_FIND_0
#define _USE_FIND		0
#endif
#ifdef CONFIG_FATFS_FIND_1
#define _USE_FIND		1
#endif
#ifdef CONFIG_FATFS_FIND_2
#define _USE_FIND		2
#endif
#ifdef CONFIG_FATFS_MKFS
#define _USE_MKFS		1
#else
#define _USE_MKFS		0
#endif
#ifdef CONFIG_FATFS_FASTSEEK
#define _USE_FASTSEEK		1
#else
#define _USE_FASTSEEK		0
#endif
#ifdef CONFIG_FATFS_EXPAND
#define _USE_EXPAND		1
#else
#define _USE_EXPAND		0
#endif
#ifdef CONFIG_FATFS_CHMOD
#define _USE_CHMOD		1
#else
#define _USE_CHMOD		0
#endif
#ifdef CONFIG_FATFS_LABEL
#define _USE_LABEL		1
#else
#define _USE_LABEL		0
#endif
#ifdef CONFIG_FATFS_FORWARD
#define _USE_FORWARD		1
#else
#define _USE_FORWARD		0
#endif
#define _CODE_PAGE		1
#define _USE_LFN		0
#define _MAX_LFN		255
#define _LFN_UNICODE		0
#define _STRF_ENCODE		0
#ifdef CONFIG_FATFS_RPATH
#define _FS_RPATH		1
#else
#define _FS_RPATH		0
#endif
#define _VOLUMES		1
#define _STR_VOLUME_ID		0
#ifdef CONFIG_FATFS_MPART
#define _MULTI_PARTITION	1
#else
#define _MULTI_PARTITION	0
#endif
#define _MIN_SS			512
#define _MAX_SS			512
#ifdef CONFIG_FATFS_TRIM
#define _USE_TRIM		1
#else
#define _USE_TRIM		0
#endif
#ifdef CONFIG_FSINFO_0
#define _FS_NOFSINFO		0
#endif
#ifdef CONFIG_FSINFO_1
#define _FS_NOFSINFO		1
#endif
#ifdef CONFIG_FSINFO_2
#define _FS_NOFSINFO		2
#endif
#ifdef CONFIG_FSINFO_3
#define _FS_NOFSINFO		3
#endif
#define _FS_TINY		1
#define _FS_EXFAT		0
#ifdef CONFIG_SRTC
#define	_FS_NORTC		1
#else
#define	_FS_NORTC		0
#endif
#define _NORTC_MON		1
#define _NORTC_MDAY		1
#define _NORTC_YEAR		2016
#define _FS_LOCK		CONFIG_FATFS_LOCKS
#define _FS_REENTRANT		0
#define _FS_TIMEOUT		KRN_INFWAIT
#define _SYNC_t			KRN_LOCK_T*

#endif
