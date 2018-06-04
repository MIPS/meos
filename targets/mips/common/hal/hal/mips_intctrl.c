#include <mips/cpu.h>
#include <mips/m32c0.h>
#include <mips/hal.h>

/*
 * Interrupt masking and acknowledging functions - these are weak so they can
 * be replaced with versions that understand more complex interrupt models.
 */

reg_t _mips_intmask(const reg_t index, const reg_t enable)
    __attribute__ ((weak));
reg_t _mips_intmask(const reg_t index, const reg_t enable)
{
	register reg_t enbefore, valbefore = 0, indexedbit = 0;

	/*
	 * Calculate which bit upfront to minimise critical section.
	 * Note that this function supports the MUCON ASE, unlike the .h files.
	 */
	if ((index >= 0) && (index <= 8))
		/* Traditional/1st MUCON ASE interrupt */
		indexedbit = SR_IM0 << index;
	else if (index == 9)
		/* 2nd MUCON ASE interrupt. */
		indexedbit = SR_IM7 << 2;

	/* Make sure we can safely adjust the mask */
	enbefore = _mips_intdisable();

	/* Make the change */
	valbefore = mips32_bcssr(indexedbit, enable ? indexedbit : 0);

	/* Go live again */
	_mips_intrestore(enbefore);

	/* Return true if it was enabled, again outside critical section */
	return (valbefore & indexedbit) != 0;
}

reg_t _mips_intack(const reg_t index) __attribute__ ((weak));
reg_t _mips_intack(const reg_t index)
{
	register reg_t enbefore, valbefore = 0, indexedbit;

	/* We only handle software interrupts - bail out otherwise */
	if ((index < 0) && (index > 1))
		return 0;

	/* Calculate which bit upfront to minimise critical section */
	indexedbit = CR_IP0 << index;

	/* Make sure we can safely adjust the state */
	enbefore = _mips_intdisable();

	/* Make the change */
	valbefore = mips32_bicsr(indexedbit);

	/* Go live again */
	_mips_intrestore(enbefore);

	/* Return true if it was enabled, again outside critical section */
	return (valbefore & indexedbit) != 0;
}
