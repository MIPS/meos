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
*   Description:	virtio console driver
*
*************************************************************************/

#include "MEOS.h"

int32_t VIRTIOCONSOLE_config(UART_T * uuart, const char *config)
{
	DBG_assert(0, "Meaningless to configure a VIRTIO console!\n");
	return 0;
}

void VIRTIOCONSOLE_read()
{
	VIRTIOCONSOLE_T *console = (VIRTIOCONSOLE_T *) KRN_taskParameter(NULL);
	MQ_MSG_T *msg;
	while ((msg = MQDEVICE_recv(&console->rq, 0))) {
		RING_writeBuffer(&console->uart.rx, MQ_MSG_data(msg),
				 MQ_MSG_length(msg), KRN_INFWAIT);
		MQDEVICE_return(&console->rq, msg);
	}
}

void VIRTIOCONSOLE_write()
{
	VIRTIOCONSOLE_T *console = (VIRTIOCONSOLE_T *) KRN_taskParameter(NULL);
	MQ_MSG_T *msg;
	while ((!RING_empty(&console->uart.tx))
	       && (msg = MQDEVICE_take(&console->wq, 0))) {
		msg->length =
		    RING_readBuffer(&console->uart.tx, MQ_MSG_data(msg),
				    MQ_MSG_length(msg), 0);
		MQDEVICE_send(&console->wq, msg);
	}
}

void VIRTIOCONSOLE_isr(int32_t sig)
{
	IRQ_DESC_T *desc = IRQ_cause(sig);
	VIRTIOCONSOLE_T *console = (VIRTIOCONSOLE_T *) desc->priv;
	KRN_queueWQ(LOPRI, (KRN_TASKFUNC_T *) VIRTIOCONSOLE_read, console,
		    "VIRTIO console read", 0);
	MQDEVICE_notify(&console->wq);
	IRQ_ack(desc);
	MEM_leww(console->ack,
		 VIRTIO_INT_USED_RING_UPDATE | VIRTIO_INT_CONFIGURATION_CHANGE);
}

void VIRTIOCONSOLE_enableTXEmptyInt(UART_T * uuart)
{
	KRN_queueWQ(LOPRI, (KRN_TASKFUNC_T *) VIRTIOCONSOLE_write, uuart,
		    "VIRTIO console write", 0);
}

void VIRTIOCONSOLE_init(VIRTIOCONSOLE_T * console, uint8_t * txBuf,
			size_t txLen, uint8_t * rxBuf, size_t rxLen,
			IRQ_DESC_T * irq)
{

	void *vAddr = MEM_p2v(console->pAddr, MEM_P2V_UNCACHED);
	console->ack = vAddr + VIRTIO_REG_INTERRUPTACK;
	console->uart.config = VIRTIOCONSOLE_config;
	console->uart.enableTXEmptyInt = VIRTIOCONSOLE_enableTXEmptyInt;
	/* Verify */
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_MAGICVALUE) == 0x74726976UL,
		   "Virtio magic mismatch! %08" PRIx32 "@%p != %08"
		   PRIx32 "\n", MEM_lerw(vAddr + VIRTIO_REG_MAGICVALUE),
		   vAddr + VIRTIO_REG_MAGICVALUE, 0x74726976UL);
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_VERSION) == 2,
		   "Virtio version mismatch!\n");
	DBG_assert(MEM_lerw(vAddr + VIRTIO_REG_DEVICEID) == VIRTIO_DEV_CONSOLE,
		   "Not a Virtio console!\n");
	/* Reset */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, 0);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Set ACKNOWLEDGE (1) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Set DRIVER (2) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Deal with feature bits */
	MEM_leww(vAddr + VIRTIO_REG_DRIVERFEATURESSEL, 0);
	MEM_leww(vAddr + VIRTIO_REG_DRIVERFEATURES, 0);	/* We don't use any special features */
	/* Set FEATURES_OK (8) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS,
		 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
	/* Check FEATURES_OK (8) still set */
	DBG_assert((MEM_lerw(vAddr + VIRTIO_REG_STATUS) & VIRTIO_STATUS_FEATURES_OK) ==
		   VIRTIO_STATUS_FEATURES_OK,
		   "Failed to negotiated Virtio console device! STATUS=%08"
		   PRIx32 ".\n", MEM_lerw(vAddr + VIRTIO_REG_STATUS));
	/* Init MQs */
	KRN_initPool(&console->rhpool, &console->rheaders, 16,
		     sizeof(MQ_MSG_T));
	KRN_initPool(&console->rmpool, &console->rmsgs, 16,
		     sizeof(VIRTIOCONSOLEMSG_T));
	KRN_initPool(&console->whpool, &console->wheaders, 16,
		     sizeof(MQ_MSG_T));
	KRN_initPool(&console->wmpool, &console->wmsgs, 16,
		     sizeof(VIRTIOCONSOLEMSG_T));
	MQDEVICE_init(&console->rq, &console->rhpool, &console->rring, 4096,
		      NULL, (uint32_t *) vAddr, 0, &console->rmpool,
		      sizeof(VIRTIOCONSOLEMSG_T));
	MQDEVICE_init(&console->wq, &console->whpool, &console->wring, 4096,
		      NULL, (uint32_t *) vAddr, 1, &console->wmpool,
		      sizeof(VIRTIOCONSOLEMSG_T));
	/* Init driver */
	RING_init(&console->uart.tx, txBuf, txLen);
	RING_init(&console->uart.rx, rxBuf, rxLen);
	irq->isrFunc = VIRTIOCONSOLE_isr;
	irq->priv = (void *)console;
	IRQ_route(irq);
	/* Set MQs */
	MEM_leww(vAddr + VIRTIO_REG_QUEUESEL, 0);
	MEM_leww(vAddr + VIRTIO_REG_QUEUEREADY, 1);
	MEM_leww(vAddr + VIRTIO_REG_QUEUESEL, 1);
	MEM_leww(vAddr + VIRTIO_REG_QUEUEREADY, 1);
	/* Set DRIVER_OK (4) */
	MEM_leww(vAddr + VIRTIO_REG_STATUS,
		 VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK |
		 VIRTIO_STATUS_DRIVER_OK);
	KRN_barrier(KRN_BARRIER_FLAG_WRITE);
}
