1.0P1
~~~~~

*	MicroMIPS support.
*	PIC32 support.
*	More readable code.
*	Changes to reduce reliance on MIPS toolchain.
*	Reenabled targeting mips-gnu-linux.
*	Use system ncurses if available, avoiding a segmentation fault for some users.
*	Report last error on build failure, to reduce the need for V=1
*	Compiler compatibility fixes for CentOS/MIPS Linux toolchain.
*	Fix bad URL in default configuration.

1.0P2
~~~~~

*	Multitoolchain support: SDE, MTI, XC32.
*	R6 support.
*	Cache fixes.
*	Build cleanly with a C++ compiler.
*	Paranoia fixes.
*	Better timer tick recovery.
*	GIC interrupt fixes.
*	Improved timing test accuracy.

1.0P3
~~~~~

*	HWSTAT interrupt support.
*	MCU ASE support.
*	Interprocessor messaging protocol changed to be rproc-serial compatible.
*	Framework for defining resource tables.
*	Prevent internal headers from conflicting with system headers.
*	XC32 build fixes.
*	DSP context fixes.
*	Cache operation fixes.
*	Expanded test suite: timerirq, intsynth, softint
*	Test case fixes.
*	Further improvements to timing test accuracy.

1.0P4
~~~~~

*	Initial support for IPM between Linux/bare metal.
*	MQs tested against Linux VRings, and made compatible.
*	UHI compatibility and chaining protocol support.
*	Use MTI context format.
*	MCU ASE fixes.
*	Paranoia/IRQ fixes.
*	MTI toolchain fixes.
*	meos-config flag output fixes.
*	Remove HWSTAT test due to hardware changes.
*	Support -minterlink-compressed, -ffunction-sections, -fdata-sections on MIPS.
*	Multi-processor test under simulation.
*	Test and documentation fixes.

1.0P5
~~~~~

*	MEOS now undergoes automated testing.
*	MIPS port:

	*	2015.01-7 MTI toolchain support.
	*	UHI support.
	*	FPxx support.
	*	MIPS context format synchronised with MTI toolchain definitions.
	*	No longer reserves k1, freeing it up for toolchain use.
	*	Makes minimal use of noreorder, for better MIPS32R6 compatibility.
	*	Option to reconfigure Malta for better baseline compatibility.
	*	Ensure 64-bit stack alignment.
	*	GIC interrupt routing improved.
	*	Baseline IPI guaranteed when a debug probe is in use.
	*	Timer rewritten: default mode does not try to compensate for missed ticks. A new adaptive mode permanently reduces granularity to avoid missed ticks. A warning message indicates when this occurs, allowing timer resolution to be suitably adjusted by the user.
	*	Test harness bug fixes.

*	New primitive, a synchronisation barrier: KRN_SYNC_T, KRN_initSync(), KRN_sync().
*	interThread test rewritten to contain fewer race conditions.
*	KRN_import() waits for the other side, making initialisation safer.
*	Paranoia now C++ compatible.
*	MQ_T compatibility with Linux Vrings improved.
*	More detailed trace logs, including DBG_logT() and DBG_putT().
*	Trace data quality improved.
*	Test and documentation fixes.
*	Demo system introduced. Currently contains one demo, Eliza. This currently operates via stdio.

2.0
~~~

*	New data structure, a byte oriented ring buffer: RING_T.
*	Support for communicating between processors running Linux and bare metal MEOS. This is awaiting MIPS remoteproc support in the Linux kernel.

*	MIPS port:

	*	For MTI, the 2015.06-05 toolchain ( http://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/available-releases/version-1-3/ ) or newer is required.
	*	Move to using di/ei instructions, requiring MIPS R2 or greater.
	*	Memory manager configuration removed in favour of unified support.
	*	Performance counter interrupt support, extending precision to 63 bits.
	*	Support for manual accumulation of performance counter data.
	*	Special handling of FDC and timer interrupts for baseline interrupts allowing limited sharing of interrupts.
	*	Support for debug logging via FDC.
	*	Eliza demo now supports PIC32 and Malta 16550 UARTs.
	*	GIC interrupt overheads reduced.
	*	MIPS R6 interrupt entry point fixed.
	*	PIC32 single entry point mode fixed.
	*	Support for 4 performance counters, where available.
	*	IRQ IPL protected against aggressive optimisation.
	*	Timer adaptation made more conservative so that delays do not spiral out of control.

*	Race condition in KRN_sync() fixed.
*	QIO test rewritten to use KRN_sync().
*	MEM_P2V_VIEW_T hint added to MEM_p2v, specifying visibility requirements of generate virtual address.
*	Consistent import/export protocol, regardless of paranoia configuration.
*	Improved dependency handling.
*	Test and documentation fixes.
*	New demos:

	*	ckw_gpio: chipKIT WiFire GPIO demo: LD1-4 show a binary number. Decrement/increment with BTN1/2.
	*	timed_paranoia: FPU paranoia instrumented with performance counters.

*	KNOWN BUG: Fedora 22 unsupported. Awaiting upstream fix in GNU indent.

3.0
~~~

*	Supporting packages added: these are auxiliary libraries that complement MEOS.
	*	LwIP 2.0.1.
	*	FatFS 12c.
	*	Apache MyNewt Newtron flash filing system.
	*	zlib 1.2.8.
	*	.cpio/.cpio.gz ROM filing system.
	*	Mass storage abstraction.
	*	Software real time clock, optionally seeded by SNTP.
	*	TFTP client.
	*	Shared low priority work queue.
	*	QIO and timer ISRs converted to supported packages.
	*	Drivers:

		*	UART: 16550A, PIC32, FDC, virtio console.
		*	Networking: LAN9211, MRF24G, XPSETHLITE, virtio net.
		*	Mass storage: Memory backed drives, fixed partitions.

*	DeviceTree support added, providing:

	* Auto-generated board support packages, automatically initialising MEOS and supporting packages.
	* Auto-configured linker scripting.
	* Simplfied multiprocessor system specification.

*	Tickless timers for better power consumption: timeouts are now in microseconds.
*	New data structure, a work queue: WQ_T.
*	Hibernating tasks may now be woken with an optional value.
*	MEOS project build system added. This provides a turnkey build system for new MEOS projects, simplifying configuration and deployment.
*	Auto install: MEOS will try to install missing build dependencies on supported host platforms.
*	Support for generating remoteproc compatible firmware images.
*	Preparatory work for VZ support.
*	MIPS port:

	*	Remove SDE and XC32 support.
	*	Add R6 IMG toolchain support.
	*	Remove dependency on IASim - simulators provided by Codescape
	*	Enhanced version of MIPS HAL.
	*	Exception chaining to boot PROM support removed.

*	Linux port:

	*	MEOS_join()/MEOS_leave() simplify writing Linux code that communicates with MEOS.

*	Time slicing fixed.
*	Improved virtio queue support, may now be used for other purposes, e.g. virtual devices.
*	Improved import/export protocol.
*	Improved dependency handling.
*	Test and documentation fixes.
*	New demos:

	*	web_page: a trivial web server.
	*	srtc_demo: demonstrating the software realtime clock, seeded by SNTP.
	*	tftp_demo: showing data fetched via tftp.
	*	mipsfpga_demo: a combination of other demos curated for the mipsFPGA platform.

*	All supported distributions should now build clean.
