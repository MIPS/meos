/***(C)2015***************************************************************
*
* Copyright (C) 2015 MIPS Tech, LLC
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
****(C)2015**************************************************************/

/*************************************************************************
*
*   Description:        CPIO filesystem
*
*************************************************************************/

#include "meos/cpio/cpio.h"

#ifdef CONFIG_ZLIB
#include "zlib.h"
#endif

#include <string.h>

#define CPIO_MAGIC_SIZE_ASCII (6)
#define CPIO_MAGIC_SIZE_BIN (2)
#define CPIO_HEADER_SIZE_ASCII (76)
#define CPIO_HEADER_SIZE_BIN (26)
#ifdef CONFIG_ZLIB
#define CPIO_HEADER_SIZE_GZ (2)
#endif

#define MAX(a, b) ((a < b) ? b : a)
#define MIN(a, b) ((a < b) ? a : b)

enum CPIO_FIELD_T {
	CPIO_FIELD_MAGIC = 0,	/* all fields size 2 unless stated below */
	CPIO_FIELD_DEV,
	CPIO_FIELD_INO,
	CPIO_FIELD_MODE,
	CPIO_FIELD_UID,
	CPIO_FIELD_GID,
	CPIO_FIELD_NLINK,
	CPIO_FIELD_RDEV,
	CPIO_FIELD_MTIME,	/* size 4 */
	CPIO_FIELD_NAMESIZE,	/* includes a null byte at the end of the path */
	CPIO_FIELD_FILESIZE,	/* size 4 */
	CPIO_FIELD_FILENAME	/* technically not in the header */
};

typedef struct CPIO_header_tag {
	uint32_t end;
	 uint32_t(*mod) (const uint32_t, const uint8_t, const uint8_t);
	 uint32_t(*get_field) (const struct CPIO_header_tag * const,
			       const enum CPIO_FIELD_T);
#ifdef CONFIG_ZLIB
	uint8_t buf[MAX(CPIO_HEADER_SIZE_GZ, MAX(CPIO_HEADER_SIZE_BIN,
						 CPIO_HEADER_SIZE_ASCII))];
#else
	uint8_t buf[MAX(CPIO_HEADER_SIZE_BIN, CPIO_HEADER_SIZE_ASCII)];
#endif
} CPIO_HEADER_T;

static uint32_t CPIO_headerAsciiToUint32Mod(const uint32_t val,
					    const uint8_t c, const uint8_t byte)
{
	return (val << 3) | (c & 0x07);
}

static uint32_t CPIO_headerUnalignedReadModBE(const uint32_t val,
					      const uint8_t c,
					      const uint8_t byte)
{
	return (val << 8) | c;
}

static uint32_t CPIO_headerUnalignedReadModLE(const uint32_t val,
					      const uint8_t c,
					      const uint8_t byte)
{
#if CPIO_ASSUME_PDP != 0
	uint32_t b = val | (c << (byte * 8));
	return (byte == 3) ? (b >> 16) | (b << 16) : b;
#else
	return val | (c << (byte * 8));
#endif
}

static uint32_t CPIO_headerRead(const uint8_t * field,
				const uint8_t * const next,
				uint32_t(*mod) (const uint32_t, const uint8_t,
						const uint8_t))
{
	uint32_t ret = 0;
	const uint8_t *const start = field;
	while (field < next) {
		ret = mod(ret, *field, field - start);
		++field;
	}
	return ret;
}

/* returns 2 if gz, 1 if ascii, 0 if binary and -1 if header is not recognised */

static int32_t CPIO_headerCheckMagic(const uint8_t * const header)
{
	const uint8_t magic[] = "070707";
	const unsigned char magic_len = sizeof(magic) - 1;

	/* this must check the magic sequences shortest to longest */
	if (memcmp(header, "\xC7\x71", 2) == 0) {
		return 0;	/* binary LE/PDP */
	} else if (memcmp(header, "\x71\xC7", 2) == 0) {
		return 1;	/* binary BE */
#ifdef CONFIG_ZLIB
	} else if (memcmp(header, "\x1f\x8b", 2) == 0) {
		return 3;	/* gz */
#endif
	} else if (memcmp(header, magic, magic_len) == 0) {
		return 2;	/* ascii */
	}
	return -EFAULT;
}

static uint32_t CPIO_headerGetFieldBin(const CPIO_HEADER_T * const header,
				const enum CPIO_FIELD_T field)
{
	static uint8_t off[] = {
		0, CPIO_MAGIC_SIZE_BIN, 4, 6, 8, 10, 12, 14, 16,
		20, 22, CPIO_HEADER_SIZE_BIN
	};
	return CPIO_headerRead(header->buf + off[field],
			       header->buf + off[field + 1], header->mod);
}

static uint32_t CPIO_headerGetFieldAscii(const CPIO_HEADER_T * const header,
				  const enum CPIO_FIELD_T field)
{
	static uint8_t off[] = {
		0, CPIO_MAGIC_SIZE_ASCII, 12, 18, 24, 30, 36,
		42, 48, 59, 65, CPIO_HEADER_SIZE_ASCII
	};
	return CPIO_headerRead(header->buf + off[field],
			       header->buf + off[field + 1], header->mod);
}

static int32_t CPIO_getMemRange(CPIO_T * const c, uint8_t * buf,
			    const uint32_t start, const uint32_t end)
{
	if (MASS_read(c->mass, buf, 0, start, end - start) < 0)
		return -1;
	else
		return 0;
}

#ifdef CONFIG_ZLIB
/* if this returns -1 CPIO_init will need to be called again to re-allocate.
   the structure in the CPIO struct. */
static int32_t CPIO_getMemRangeGZ(CPIO_T * const c, uint8_t * buf,
			      const uint32_t start, const uint32_t end)
{
	uint8_t sec_buf[1024];
	uint32_t zoutdex = 0;
	uint64_t msize, rsize;
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in = Z_NULL;
	switch (inflateInit2(&stream, 15 | 32)) {	/* 15 is default, 32 turns on */
	default:
		break;
	case Z_MEM_ERROR:
	case Z_VERSION_ERROR:
	case Z_STREAM_ERROR:
		return -1;
	}

	if (MASS_stat(c->mass, NULL, NULL, &msize) < 0)
		return -1;

	int ret;
	uint32_t zindex_sector = 0;
	do {
		/* get data from disk */
		rsize = MIN(msize - (zindex_sector * 1024), 1024);
		if (MASS_read(c->mass, sec_buf, 0, zindex_sector * 1024, rsize)
		    < 0) {
			(void)inflateEnd(&stream);
			return -1;
		}
		++zindex_sector;

		stream.avail_in = rsize;
		stream.next_in = sec_buf;
		do {
			/* inflate data */
			stream.avail_out = end - MAX(start, zoutdex);
			stream.next_out = buf;
			ret = inflate(&stream, Z_NO_FLUSH);
			switch (ret) {
			case Z_STREAM_ERROR:
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&stream);
				return -1;
			}

			uint32_t count =
			    (end - MAX(start, zoutdex)) - stream.avail_out;
			uint32_t new_zoutdex = zoutdex + count;
			if (zoutdex > start) {
				buf += count;	/* keep output from being overwritten */
			} else if (new_zoutdex > start) {	/* first useful output into the buffer */
				uint32_t inc = new_zoutdex - start;
				memmove(buf, buf + (start - zoutdex), inc);
				buf += inc;
			}
			zoutdex = new_zoutdex;
		} while ((stream.avail_out == 0) && (zoutdex < end));	/* while there is */
		/* data in the sector buffer and it is not the end of the write section */
	}
	while ((zoutdex < end) && (ret != Z_STREAM_END));
	(void)inflateEnd(&stream);
	return 0;
}
#endif

static int32_t CPIO_get_header(CPIO_T * const c, CPIO_HEADER_T * const header,
			   const uint32_t header_start)
{
	/* read the size of the smaller header from the disk */
	uint32_t header_size = MIN(CPIO_HEADER_SIZE_BIN,
				   CPIO_HEADER_SIZE_ASCII);
#ifdef CONFIG_ZLIB
	/* alter header size to take into account CPIO_HEADER_SIZE_GZ */
	header_size = MIN(header_size, CPIO_HEADER_SIZE_GZ);
#endif
	header->end = header_start + header_size;
	/* read binary header from disk */
	if (c->gmr(c, header->buf, header_start, header->end) != 0) {
		return -1;
	}
	/* check binary/ascii */
	switch (CPIO_headerCheckMagic(header->buf)) {
	case 0:		/* binary LE */
		header->mod = &CPIO_headerUnalignedReadModLE;
		header->get_field = &CPIO_headerGetFieldBin;
		if (header_size != CPIO_HEADER_SIZE_BIN) {
			header->end = header_start + CPIO_HEADER_SIZE_BIN;
			if (c->gmr(c, header->buf + header_size,
				   header_start + header_size,
				   header->end) != 0) {
				return -1;
			}
			header_size = CPIO_HEADER_SIZE_BIN;
		}
		break;
	case 1:		/* binary BE */
		header->mod = &CPIO_headerUnalignedReadModBE;
		header->get_field = &CPIO_headerGetFieldBin;
		if (header_size != CPIO_HEADER_SIZE_BIN) {
			header->end = header_start + CPIO_HEADER_SIZE_BIN;
			if (c->gmr(c, header->buf + header_size,
				   header_start + header_size,
				   header->end) != 0) {
				return -1;
			}
			header_size = CPIO_HEADER_SIZE_BIN;
		}
		break;
	case 2:		/* ascii */
		header->mod = &CPIO_headerAsciiToUint32Mod;
		header->get_field = &CPIO_headerGetFieldAscii;
		if (header_size != CPIO_HEADER_SIZE_BIN) {
			header->end = header_start + CPIO_HEADER_SIZE_ASCII;
			if (c->gmr(c, header->buf + header_size,
				   header_start + header_size,
				   header->end) != 0) {
				return -1;
			}
			header_size = CPIO_HEADER_SIZE_ASCII;
		}
		break;
#ifdef CONFIG_ZLIB
	case 3:		/* gzip */
		/* change the get mem range function to the gz one, */
		if (c->gmr == CPIO_getMemRangeGZ) {
			return -1;	/* corrupt data */
		}
		c->gmr = &CPIO_getMemRangeGZ;
		return CPIO_get_header(c, header, header_start);
		break;
#endif
	default:
		return -1;
	}
	return 0;
}

static int32_t CPIO_findFile(CPIO_FILE_T
			 * const file, CPIO_HEADER_T
			 * const header,
			 CPIO_T * const c, const uint8_t * const path)
{
	/* check path length */
	uint32_t i = 0;
	while (path[i] != 0) {
		if (i > CPIO_MAX_NAMESIZE) {
			return -ENAMETOOLONG;
		}
		++i;
	}
	/* loop through the files on disk */
	uint32_t header_start = 0;
	while (1) {
		/* get header */
		if (CPIO_get_header(c, header, header_start) != 0) {
			return -1;
		}
		/* get name size from header */
		const uint32_t namesize = header->get_field(header,
							    CPIO_FIELD_NAMESIZE);
		if (namesize > CPIO_MAX_NAMESIZE) {
			return -EFAULT;	/* return corrupt data */
		}
		/* read name from disk */
		uint8_t name_buf[namesize];	/* VLA > C99 only */
		if (c->gmr(c, name_buf, header->end,
			   header->end + namesize) != 0) {
			return -1;
		}
		/* validate string */
		if (name_buf[namesize - 1] != 0) {
			return -EFAULT;	/* return corrupt data */
		}
		/* get file size from header */
		const uint32_t filesize =
		    header->get_field(header, CPIO_FIELD_FILESIZE);
		uint32_t data = header->end + namesize;
		if (data & 1) {	/* bring to the nearest 2 byte boundary */
			++data;
		}
		const uint32_t end = data + filesize;
		const uint32_t next = (end & 1) ? end + 1 : end;	/* bring to the nearest 2 byte boundary */
		/* test filename */
		if (strcmp((char *)name_buf, (char *)path) == 0) {
			file->header = header_start;
			file->data = data;
			file->pointer = data;
			file->end = end;
			file->next = next;
			return 0;	/* found */
		} else if (strcmp((char *)name_buf, "TRAILER!!") == 0) {
			return -ENOENT;	/* END not found */
		}
		header_start = next;
	}
	return -EFAULT;		/* unreachable */
}

static inline int32_t CPIO_checkFile(const CPIO_T * const c,
				 const CPIO_FILE_T * const file)
{
	if (file->data == 0) {
		return -EBADF;
	}
	return 0;
}

int32_t CPIO_init(CPIO_T * const c, MASS_T * const mass)
{
	DBG_assert((MASS_stat(mass, NULL, NULL, NULL) &
		    (MSF_REMOVED | MSF_ERROR)) == 0,
		   "Can't initialise cpio on %p!\n", mass);
	c->mass = mass;
	memset(c->files, 0, sizeof(c->files));
	c->gmr = CPIO_getMemRange;
	return 0;
}

int32_t CPIO_open(CPIO_T * const c, const uint8_t * const path)
{
	int ret = 0;
	while (ret < CPIO_MAX_FILES) {
		CPIO_FILE_T *file = &c->files[ret];
		if (file->data == 0) {
			CPIO_HEADER_T header;
			if (CPIO_findFile(file, &header, c, path) != 0) {
				return -ENOEXIST;
			}
			return ret;
		}
		++ret;
	}
	return -ENFILE;		/* max files reached */
}

void CPIO_close(CPIO_T * const c, const int fd)
{
	c->files[fd].data = 0;
}

ssize_t CPIO_read(CPIO_T * const c,
		  const int fd, uint8_t * const buf, const size_t count)
{
	CPIO_FILE_T *file = &c->files[fd];
	if (CPIO_checkFile(c, file) != 0) {
		return -EBADF;
	}
	size_t read_size = MIN(count, file->end - file->pointer);
	if (read_size > 0) {
		if (c->gmr(c, buf, file->pointer,
			   file->pointer + read_size) != 0) {
			return -EINVAL;
		}
		file->pointer += read_size;
	}
	return read_size;
}

off_t CPIO_lseek(CPIO_T * const c,
		 const int fd, const off_t offset, const int whence)
{
	CPIO_FILE_T *file = &c->files[fd];
	if (CPIO_checkFile(c, file) != 0) {
		return -EBADF;
	}
	switch (whence) {
	case SEEK_SET:
		file->pointer = file->data + offset;
		break;
	case SEEK_CUR:
		file->pointer += offset;
		break;
	case SEEK_END:
		file->pointer = file->end + offset;
		break;
	}
	return file->pointer - file->data;
}

off_t CPIO_tell(CPIO_T * const c, const int fd)
{
	CPIO_FILE_T *file = &c->files[fd];
	if (CPIO_checkFile(c, file) != 0) {
		return -EBADF;
	}
	return file->pointer - file->data;
}

int32_t CPIO_stat(CPIO_T * const c,
	      const uint8_t * const path, struct CPIO_stat *const buf)
{
	CPIO_FILE_T file;
	CPIO_HEADER_T header;
	if (CPIO_findFile(&file, &header, c, path) != 0) {
		return -EBADF;
	}
	buf->dev = header.get_field(&header, CPIO_FIELD_DEV);
	buf->ino = header.get_field(&header, CPIO_FIELD_INO);
	buf->mode = header.get_field(&header, CPIO_FIELD_MODE);
	buf->uid = header.get_field(&header, CPIO_FIELD_UID);
	buf->gid = header.get_field(&header, CPIO_FIELD_GID);
	buf->nlink = header.get_field(&header, CPIO_FIELD_NLINK);
	buf->rdev = header.get_field(&header, CPIO_FIELD_RDEV);
	buf->mtime = header.get_field(&header, CPIO_FIELD_MTIME);
	buf->namesize = header.get_field(&header, CPIO_FIELD_NAMESIZE);
	buf->filesize = header.get_field(&header, CPIO_FIELD_FILESIZE);
	return 0;
}
