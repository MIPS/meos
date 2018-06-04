/***(C)2001***************************************************************
*
* Copyright (C) 2001 MIPS Tech, LLC
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
****(C)2001**************************************************************/

/*************************************************************************
*
*   Description:	UART abstraction
*
*************************************************************************/

#include <stdio.h>
#include "MEOS.h"

int UART_read(void *dummy, void *buffer, int size, int n, void *priv)
{
	UART_T *uart = (UART_T *) priv;
	size_t i, j, r = 0;
	uint8_t *p = (uint8_t *) buffer;

	for (i = 0; i < n; i++) {
		for (j = 0; j < size; j++) {
			if (RING_read(&uart->rx, p++, 0) == 0)
				goto escape;
			r++;
		}
	}
      escape:
	return r;
}

int UART_inject(void *dummy, void *buffer, int size, int n, void *priv)
{
	UART_T *uart = (UART_T *) priv;
	uint8_t *p = (uint8_t *) buffer;
	size_t i = size * n;
	int32_t timeout = 0;

	do {
		while (i && RING_write(&uart->rx, *p, timeout)) {
			p++;
			i--;
		}
		if (uart->rFunc)
			uart->rFunc(uart->rPar);
		if (IRQ_bg())
			timeout = KRN_INFWAIT;
	} while (i);

	return size * n;
}

int UART_write(void *dummy, void *buffer, int size, int n, void *priv)
{
	UART_T *uart = (UART_T *) priv;
	uint8_t *p = (uint8_t *) buffer;
	size_t i = size * n;
	int32_t timeout = 0;

	do {
		while (i && RING_write(&uart->tx, *p, timeout)) {
			p++;
			i--;
		}
		uart->enableTXEmptyInt(uart);
		if (IRQ_bg())
			timeout = KRN_INFWAIT;
	} while (i);

	return size * n;
}

int UART_extract(void *dummy, void *buffer, int size, int n, void *priv)
{
	UART_T *uart = (UART_T *) priv;
	size_t i, j, r = 0;
	uint8_t *p = (uint8_t *) buffer;

	for (i = 0; i < n; i++) {
		for (j = 0; j < size; j++) {
			if (RING_read(&uart->tx, p++, 0) == 0)
				goto escape;
			r++;
		}
	}
      escape:
	return r;
}

static size_t UART_fread(UART_T * uart, uint8_t * data, size_t length)
{
	size_t i, r = 0;
	uint32_t timeout = KRN_INFWAIT;
	for (i = 0; i < length; i++) {
		if (RING_read(&uart->rx, data++, timeout) == 0)
			goto escape;
		timeout = 0;
		r++;
	}
      escape:
	return r;
}

static size_t UART_fwrite(UART_T * uart, uint8_t * data, size_t length)
{
	return UART_write(NULL, data, 1, length, (void *)uart);
}

static int UART_fseek(void *dummy, off_t * offset, int whence)
{
	/* This allows us to fseek, which allows us to clear EOF */
	*offset = 0;
	return 0;
}

static const cookie_io_functions_t UART_cookie = {
	.read = (cookie_read_function_t *) UART_fread,
	.write = (cookie_write_function_t *) UART_fwrite,
	.seek = (cookie_seek_function_t *) UART_fseek
};

FILE *UART_fopen(UART_T * uart, const char *mode)
{
	return fopencookie(uart, mode, UART_cookie);
}

void UART_readyFunc(UART_T * uart, UART_READYFUNC_T * rFunc, void *rPar)
{
	uart->rFunc = rFunc;
	uart->rPar = rPar;
}
