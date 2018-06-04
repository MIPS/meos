#include <_ansi.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mips/cpu.h>
#include <mips/m32c0.h>

extern char _end[];

void _get_ram_range(void **ram_base, void **ram_extent);

/* FIXME: This is not ideal, since we do a get_ram_range() call for
   every sbrk() call. */
char *sbrk(int nbytes)
{
	static volatile char *heap_ptr = NULL;
	static char *heap_start = NULL;
	static unsigned long heap_size = 0;
	char *base = NULL;
	unsigned int avail = 0;
	void *ram_base;
	void *ram_extent;

	if (heap_start == NULL) {
		_get_ram_range(&ram_base, &ram_extent);

		/* If the _end symbol is within the RAM then use _end.  */
		if ((void *)_end > ram_base
		    && (size_t) _end <= ((size_t) ram_extent - 1))
			ram_base = _end;
		__sync_bool_compare_and_swap(&heap_start, 0, ram_base)
		    && __sync_bool_compare_and_swap(&heap_ptr, 0, ram_base)
		    && __sync_bool_compare_and_swap(&heap_size, 0,
						    (size_t) ram_extent -
						    (size_t) ram_base);
		while (heap_size == 0)
			__sync_synchronize();
	}

	do {
		if ((heap_ptr >= heap_start)
		    && (heap_ptr <= (heap_start + heap_size - 1))) {
			avail = (heap_start + heap_size) - heap_ptr;
			base = (char *)heap_ptr;
		}
		/* else will fail since "nbytes" will be greater than zeroed "avail" value */
		if (((unsigned int)nbytes > avail)
		    || (heap_ptr + (unsigned int)nbytes < heap_start)) {
			base = (char *)-1;
			break;
		}
	} while (!__sync_bool_compare_and_swap
		 (&heap_ptr, base, heap_ptr + nbytes));

	return base;
}
