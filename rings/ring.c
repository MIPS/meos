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
*   Description:        Ring buffer
*
*************************************************************************/

/* controls for inline compilation */
#define RING_COMPILE
#ifndef RING_BUILDC
#ifdef RING_INLINE
#ifndef RING_CINCLUDE
#undef RING_COMPILE		/* only compile through the H file when inlining */
#endif
#endif
#else
#undef RING_INLINE
#undef RING_FQUALS
#define RING_FQUALS
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef RING_COMPILE

#ifndef RING_INLINE
#include "meos/rings/ring.h"	/* don't re-include H file if inlining */
#endif

RING_FQUALS void RING_init(RING_T * ring, uint8_t * buffer, size_t size)
{
	ring->buffer = buffer;
	ring->size = size;
	ring->head = 0;
	ring->tail = 0;
	DQ_init(&ring->rq);
	DQ_init(&ring->wq);
}

RING_FQUALS int32_t RING_empty(RING_T * ring)
{
	return (ring->head == ring->tail);
}

RING_FQUALS int32_t RING_full(RING_T * ring)
{
	size_t next = (size_t) (ring->head + 1) % ring->size;
	return next == ring->tail;
}

RING_FQUALS int32_t RING_space(RING_T * ring)
{
	return (ring->tail - ring->head) +
	    (-((int32_t) (ring->tail <= ring->head)) & ring->size);
}

RING_FQUALS int32_t RING_used(RING_T * ring)
{
	return (ring->size + ring->head - ring->tail) % ring->size;
}

RING_FQUALS int32_t RING_write(RING_T * ring, uint8_t data, int32_t timeout)
{
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	size_t next;
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (RING_full(ring)) {
		if (timeout == 0) {
			IRQ_restoreIPL(ipl);
			return 0;
		}
		_KRN_hibernateTask(_KRN_current, &ring->wq);
		KRN_scheduleUnprotect(ipl);
		ipl = IRQ_raiseIPL();
		if (_KRN_current->timedOut != KRN_RUNNING) {
			IRQ_restoreIPL(ipl);
			return 0;
		}
	}
	next = (size_t) (ring->head + 1) % ring->size;
	ring->buffer[ring->head] = data;
	ring->head = next;
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	KRN_wake(&ring->rq);
	return 1;
}

RING_FQUALS int32_t RING_writeBuffer(RING_T * ring, uint8_t * data,
				     size_t length, int32_t timeout)
{
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	size_t i, next;
	size_t space;
	const int32_t partial = 1;
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (timeout == 0) {
		if (((partial == 1) && RING_full(ring)) ||
		    ((partial == 0) && (RING_space(ring) < length))) {
			IRQ_restoreIPL(ipl);
			return 0;
		} else {
			/* Partial */
			space = RING_space(ring);
			length = space < length ? space : length;
		}
	} else {
		while (RING_space(ring) < length) {
			_KRN_hibernateTask(_KRN_current, &ring->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			if (_KRN_current->timedOut != KRN_RUNNING) {
				if (((partial == 1) && RING_full(ring)) ||
				    ((partial == 0)
				     && (RING_space(ring) < length))) {
					IRQ_restoreIPL(ipl);
					return 0;
				} else {
					/* Partial */
					space = RING_space(ring);
					length =
					    space < length ? space : length;
					break;
				}
			}
		}
	}
	/* Write */
	for (i = 0; i < length; i++) {
		next = (size_t) (ring->head + 1) % ring->size;
		ring->buffer[ring->head] = *data;
		data++;
		ring->head = next;
	}
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	KRN_wake(&ring->rq);
	return length;
}

RING_FQUALS int32_t RING_read(RING_T * ring, uint8_t * data, int32_t timeout)
{
	if (RING_empty(ring)) {
		if (timeout == 0)
			return 0;
		KRN_hibernate(&ring->rq, timeout);
		if ((timeout > 0) && (_KRN_current->timedOut != KRN_RUNNING))
			return 0;
	}

	*data = ring->buffer[ring->tail];
	ring->tail = (size_t) (ring->tail + 1) % ring->size;
	KRN_wake(&ring->wq);
	return 1;
}

RING_FQUALS int32_t RING_readBuffer(RING_T * ring, uint8_t * data,
				    size_t length, int32_t timeout)
{
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	size_t i;
	size_t used;
	const int32_t partial = 1;
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (timeout == 0) {
		if (((partial == 1) && RING_empty(ring)) ||
		    ((partial == 0) && (RING_used(ring) < length))) {
			IRQ_restoreIPL(ipl);
			return 0;
		} else {
			/* Partial */
			used = RING_used(ring);
			length = used < length ? used : length;
		}
	} else {
		while (RING_used(ring) < length) {
			_KRN_hibernateTask(_KRN_current, &ring->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			if (_KRN_current->timedOut != KRN_RUNNING) {
				if (((partial == 1) && RING_empty(ring)) ||
				    ((partial == 0)
				     && (RING_used(ring) < length))) {
					IRQ_restoreIPL(ipl);
					return 0;
				} else {
					/* Partial */
					used = RING_used(ring);
					length = used < length ? used : length;
					break;
				}
			}
		}
	}
	/* Write */
	for (i = 0; i < length; i++) {
		*data = ring->buffer[ring->tail];
		data++;
		ring->tail = (size_t) (ring->tail + 1) % ring->size;
	}
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	KRN_wake(&ring->wq);
	return length;
}

RING_FQUALS int32_t RING_peekContiguous(RING_T * ring, uint8_t ** pointer,
					size_t length, int32_t timeout)
{
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	const int32_t partial = 1;
	size_t used;
	size_t maxlen;
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (timeout == 0) {
		if (((partial == 1) && RING_empty(ring)) ||
		    ((partial == 0) && (RING_used(ring) < length))) {
			IRQ_restoreIPL(ipl);
			return 0;
		} else {
			/* Partial */
			used = RING_used(ring);
			length = used < length ? used : length;
		}
	} else {
		while (RING_used(ring) < length) {
			_KRN_hibernateTask(_KRN_current, &ring->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			if (_KRN_current->timedOut != KRN_RUNNING) {
				if (((partial == 1) && RING_empty(ring)) ||
				    ((partial == 0)
				     && (RING_used(ring) < length))) {
					IRQ_restoreIPL(ipl);
					return 0;
				} else {
					/* Partial */
					used = RING_used(ring);
					length = used < length ? used : length;
					break;
				}
			}
		}
	}
	*pointer = &ring->buffer[ring->tail];
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	maxlen = ring->size - ring->tail;
	if (maxlen < length)
		length = maxlen;
	return length;
}

RING_FQUALS int32_t RING_skipContiguous(RING_T * ring, size_t length,
					int32_t timeout)
{
	IRQ_IPL_T ipl;
	KRN_TIMER_T timer;
	const int32_t partial = 1;
	size_t maxlen;
	size_t used;
	ipl = _KRN_raiseIPLWithTimeout(&timer, timeout);
	if (timeout == 0) {
		if (((partial == 1) && RING_empty(ring)) ||
		    ((partial == 0) && (RING_used(ring) < length))) {
			IRQ_restoreIPL(ipl);
			return 0;
		} else {
			/* Partial */
			used = RING_used(ring);
			length = used < length ? used : length;
		}
	} else {
		while (RING_used(ring) < length) {
			_KRN_hibernateTask(_KRN_current, &ring->wq);
			KRN_scheduleUnprotect(ipl);
			ipl = IRQ_raiseIPL();
			if (_KRN_current->timedOut != KRN_RUNNING) {
				if (((partial == 1) && RING_empty(ring)) ||
				    ((partial == 0)
				     && (RING_used(ring) < length))) {
					IRQ_restoreIPL(ipl);
					return 0;
				} else {
					/* Partial */
					used = RING_used(ring);
					length = used < length ? used : length;
					break;
				}
			}
		}
	}
	maxlen = ring->size - ring->tail;
	if (maxlen < length)
		length = maxlen;
	ring->tail = (size_t) (ring->tail + length) % ring->size;
	_KRN_restoreIPLWithTimeout(ipl, &timer, timeout);
	KRN_wake(&ring->wq);
	return length;
}

#endif
