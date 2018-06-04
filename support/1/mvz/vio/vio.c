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
*          File:    $File: //meta/fw/meos2/DEV/LISA.PARRATT/targets/mips/common/target/m32c0.h $
* Revision date:    $Date: 2015/06/09 $
*   Description:    Virtio common
*
*************************************************************************/

#include "meos/mvz/mvz.h"
#include "meos/vio/vio.h"
#include "meos/mem/mem.h"
#include "meos/inttypes.h"

int VIO_read(void *address, void *buffer, int size, int n, void *priv)
{
	const uint32_t magic = 0x74726976, version = 2;
	MVZ_VREGS_T *vreg = (MVZ_VREGS_T *) priv;
	VIO_T *vio = vreg->priv;
	uint32_t *value = (uint32_t *) buffer;
	uintptr_t offset = MVZ_gv2gp(vreg->guest, (uintptr_t) address, NULL,
				     NULL) - (vreg->start & ~1);
	if (offset >= VIRTIO_REG_CONFIG) {
		int r = vio->read((void *)offset, buffer, size, n, vio);
		return r;
	}
	if (n != 1) {
		DBG_assert(n != 1, "Guest '%s attempted Virtio multi-read !\n",
			   KRN_taskName(NULL));
		MVZ_restart(vreg->guest);
	}
	if (size != 4) {
		DBG_assert(n != 1,
			   "Guest '%s attempted Virtio common non-word read!\n",
			   KRN_taskName(NULL));
		MVZ_restart(vreg->guest);
	}
	switch (offset) {
	case VIRTIO_REG_MAGICVALUE:	// R
		*value = MEM_lerw(&magic);
		break;
	case VIRTIO_REG_VERSION:	// R
		*value = MEM_lerw(&version);
		break;
	case VIRTIO_REG_DEVICEID:	// R
		*value = MEM_lerw(&vio->deviceID);
		break;
	case VIRTIO_REG_VENDORID:	// R
		*value = MEM_lerw(&vio->vendorID);
		break;
	case VIRTIO_REG_DEVICEFEATURES:	// R
		*value = MEM_lerw(&vio->deviceFeatures[vio->deviceFeaturesSel]);
		break;
	case VIRTIO_REG_QUEUENUMMAX:	// R
		*value = MEM_lerw(&vio->queues[vio->queueSel].numMax);
		break;
	case VIRTIO_REG_QUEUEREADY:	// RW
		*value = MEM_lerw(&vio->queues[vio->queueSel].ready);
		break;
	case VIRTIO_REG_INTERRUPTSTATUS:	// R
		*value = MEM_lerw(&vio->interrupts);
		break;
	case VIRTIO_REG_STATUS:	// RW
		*value = MEM_lerw(&vio->status);
		break;
	case VIRTIO_REG_CONFIGGENERATION:	// R
		*value = MEM_lerw(&vio->configGeneration);
		break;
	default:
		DBG_assert(0,
			   "Guest '%s' attempted read from Virtio write-only register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart(vreg->guest);
	}
	return 4;
}

int VIO_write(void *address, void *buffer, int size, int n, void *priv)
{
	uint64_t lv;
	uint32_t sv;
	MVZ_VREGS_T *vreg = (MVZ_VREGS_T *) priv;
	VIO_T *vio = vreg->priv;
	uint32_t *value = (uint32_t *) buffer;
	uintptr_t offset = MVZ_gv2gp(vreg->guest, (uintptr_t) address, NULL,
				     NULL) - (vreg->start & ~1);

	if (offset >= VIRTIO_REG_CONFIG) {
		return vio->write((void *)offset - 0x100, buffer, size, 1, vio);
	}
	if (n != 1) {
		DBG_assert(n != 1, "Guest '%s attempted Virtio multi-write !\n",
			   KRN_taskName(NULL));
		MVZ_restart(vreg->guest);
	}
	if (size != 4) {
		DBG_assert(n != 1,
			   "Guest '%s' attempted Virtio common non-word write!\n",
			   KRN_taskName(NULL));
		MVZ_restart(vreg->guest);
	}
	switch (offset) {
	case VIRTIO_REG_DEVICEFEATURESSEL:	// W
		DBG_assert(VIRTIO_STATUS_NEGOTIATE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->deviceFeaturesSel, *value);
		break;
	case VIRTIO_REG_DRIVERFEATURES:	// W
		DBG_assert(VIRTIO_STATUS_NEGOTIATE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->driverFeatures[vio->driverFeaturesSel], *value);
		break;
	case VIRTIO_REG_DRIVERFEATURESSEL:	// W
		DBG_assert(VIRTIO_STATUS_NEGOTIATE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		vio->driverFeaturesSel = *value;
		break;

	case VIRTIO_REG_QUEUESEL:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->queueSel, *value);
		if (vio->queueSel > vio->nQueues) {
			DBG_assert(0,
				   "Guest '%s' op %03" PRIx32
				   " selects non-existent queues!\n",
				   KRN_taskName(NULL), offset);
			vio->queueSel = 0;
		}
		break;
	case VIRTIO_REG_QUEUENUM:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->queues[vio->queueSel].num, *value);
		break;
	case VIRTIO_REG_QUEUEDESCLOW:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].desc;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].desc =
		    (lv & 0xffffffff00000000ULL) | sv;
		break;
	case VIRTIO_REG_QUEUEDESCHIGH:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].desc;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].desc =
		    (lv & 0x00000000ffffffffULL) | ((uint64_t) sv << 32);
		break;
	case VIRTIO_REG_QUEUEAVAILLOW:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].avail;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].avail =
		    (lv & 0xffffffff00000000ULL) | sv;
		break;
	case VIRTIO_REG_QUEUEAVAILHIGH:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].avail;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].avail =
		    (lv & 0x00000000ffffffffULL) | ((uint64_t) sv << 32);
		break;
	case VIRTIO_REG_QUEUEUSEDLOW:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].used;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].used =
		    (lv & 0xffffffff00000000ULL) | sv;
		break;
	case VIRTIO_REG_QUEUEUSEDHIGH:	// W
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		lv = vio->queues[vio->queueSel].used;
		MEM_leww(&sv, *value);
		vio->queues[vio->queueSel].used =
		    (lv & 0x00000000ffffffffULL) | ((uint64_t) sv << 32);
		break;
	case VIRTIO_REG_QUEUEREADY:	// RW
		DBG_assert(VIRTIO_STATUS_INITIALISE(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->queues[vio->queueSel].ready, *value);
		break;

	case VIRTIO_REG_QUEUENOTIFY:	// W
		DBG_assert(VIRTIO_STATUS_RUNNING(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		MEM_leww(&vio->queueNotify, *value);
		MQDRIVER_notify(&vio->queues[vio->queueNotify].mq);
		break;
	case VIRTIO_REG_INTERRUPTACK:	// W
		DBG_assert(VIRTIO_STATUS_RUNNING(vio),
			   "Guest '%s' op %03" PRIx32 " in wrong state!\n",
			   KRN_taskName(NULL), offset);
		sv = vio->interrupts;
		MEM_leww(&vio->interrupts, vio->interrupts & ~*value);
		if (sv && !vio->interrupts)
			MVZ_downInt(vreg->guest, vio->gInt);
		break;
	case VIRTIO_REG_STATUS:	// RW
		sv = vio->status;
		MEM_leww(&vio->status, *value);
		if (vio->status == 0) {
			reg_t i;
			for (i = 0; i < vio->nQueues; i++)
				vio->queues[i].ready = 0;
		}
		if (VIRTIO_STATUS_RUNNING(vio)
		    && ((sv ^ vio->status) & VIRTIO_STATUS_DRIVER_OK))
			vio->bringUpDown(vio, 0);
		break;
	default:
		DBG_assert(0,
			   "Guest '%s' attempted write to Virtio read-only register %03"
			   PRIx32 "\n", KRN_taskName(NULL), offset);
		MVZ_restart(vreg->guest);
	}
	return 4;
}

void VIO_synthInt(VIO_T * vio, uint32_t i)
{
	uint32_t sv;
	i = 1 << i;
	sv = vio->interrupts & i;
	vio->interrupts |= i;
	(void)sv;
	//if (!sv && vio->interrupts)
	MVZ_upInt(vio->regs.guest, vio->gInt);
}

void VIO_reconfigure(VIO_T * vio)
{
	vio->configGeneration++;
	VIO_synthInt(vio, VIRTIO_INT_CONFIGURATION_CHANGE);
}

void VIO_init(VIO_T * device, uint32_t id, uint32_t vendor)
{
	memset(device, 0, sizeof(VIO_T));
	device->deviceID = id;
	device->vendorID = vendor;
	device->deviceFeatures[1] = VIRTIO_FLAG_VERSION_1;
}

void VIO_kill(VIO_T * device)
{
	device->bringUpDown(device, 1);
}

int VIO_writeGQ(void *address, void *buffer, int size, int n, void *priv)
{
	VIO_Q_T *vq = (VIO_Q_T *) priv;
	MVZ_GUEST_T *gt = vq->guest;
	uint16_t start, token;
	size_t total = 0, blksize;
	uintptr_t blkaddr;

	size *= n;

	start = token = MQDRIVER_getAvailable(&vq->mq, &blkaddr, &blksize);
	if (token == 0xffff)
		return 0;

	for (;;) {
		blksize = size < blksize ? size : blksize;
		MVZ_writeGPC((void *)blkaddr, buffer, blksize, 1, gt);
		buffer += blksize;
		size -= blksize;
		total += blksize;

		if (!size)
			break;

		token =
		    MQDRIVER_chainAvailable(&vq->mq, token, &blkaddr, &blksize);
		if (token == 0xffff)
			break;
	}

	MQDRIVER_addUsed(&vq->mq, start, total);
	return total;
}

size_t VIO_readGQ(VIO_Q_T * vq, MVZ_XFERFUNC_T * w, void *wp, uintptr_t wo)
{
	MVZ_GUEST_T *gt = vq->guest;
	uint16_t start, token, flags;

	size_t size, total = 0;
	uintptr_t addr;

	for (;;) {
		start = token =
		    MQDRIVER_getFirstAvailable(&vq->mq, &addr, &size, &flags);
		if (token == 0xffff)
			return total;

		for (;;) {
			MVZ_xfer(MVZ_readGPC, gt, addr, size, w, wp, wo);
			wo += size;
			total += size;

			token =
			    MQDRIVER_getMoreAvailable(&vq->mq, token, &addr,
						      &size, &flags);
			if (token == 0xffff)
				break;
		}
		MQDRIVER_getNextAvailable(&vq->mq);
		MQDRIVER_addUsed(&vq->mq, start, total);
		total = 0;
	}
}

void VIO_attach(VIO_T * device, MVZ_GUEST_T * guest)
{
	device->regs.read = &VIO_read;
	device->regs.write = &VIO_write;
	device->regs.priv = device;
	MVZ_addRegs(guest, &device->regs);
}
