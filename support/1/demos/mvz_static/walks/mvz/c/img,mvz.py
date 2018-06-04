print("""
#include "MEOS.h"
#include <string.h>

MVZ_T MVZ_hyper;

static uint32_t tlbIndex = 0;

void MVZ_guest0Config(void);
void MVZ_guest0Start(void);

static int _loadELF(void *paddr, void *buf, int size, int n, void *priv)
{
	memcpy((void*)buf, (void *)((uintptr_t) priv + (uintptr_t)paddr), size * n);
	return size * n;
}

void MVZ_start(void)
{
	MVZ_hypervise(&MVZ_hyper, (KRN_ISRFUNC_T*)%(hypcall)s);
	MVZ_guest0Config();
	MVZ_splitTLBs(tlbIndex);
	MVZ_guest0Start();
}
""" % {"hypcall" : node["mti,hypcall"]})
