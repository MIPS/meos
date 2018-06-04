All
~~~

FPU
+++

FPU context is lazily saved/restored. If no other task is using the FPU, there will be no overhead, otherwise there will be a short delay as context is saved & loaded at the first point each task uses the FPU after a context switch. FPU context is stored in the task data structure.

Performance counters
++++++++++++++++++++

Counter 0 defaults to number of cycles, counter 1 is configured by CP0. Attempts to change counter 0 will be reset by calls to ``TMR_resetCycleCount``. If performance counter interrupts are enabled, all performance counters will become invalid and deconfigured during interrupt initialisation.

microMIPS
~~~~~~~~~

There is no support for unwinding microMIPS stack frames. You will have to use MIPS32 if you wish to use the debugging facilities that use this.

Baseline
~~~~~~~~

Baseline interrupts provides the traditional non-EIC MIPS interrupt model.

IRQ specification
+++++++++++++++++

The baseline IRQ implementation provides support for 32 exceptions, and the 8 interrupt lines. 32 software interrupts are multiplexed onto interrupt 0. Interrupt 1 is reserved for inter-processor communication.

IRQ specialisation
++++++++++++++++++

There is no specialisation for baseline interrupts. SoC routing must be performed manually.


Baseline + XPS interrupt controller
+++++++++++++++++++++++++++++++++++

The MIPS FPGA platform uses a secondary interrupt controller to extend the baseline interrupt model.

The additional interrupts are added to the intNum numbering space, numbered sequentially after the core interrupts. The constant ``XPSINTC_FIRSTINT`` may be used to simplify access.

GIC
~~~

GIC interrupts supports a GIC attached to a MIPS core. It supports both non-EIC and EIC interrupt models.

IRQ specification
+++++++++++++++++

The GIC IRQ implementation provides support for 32 exceptions, up to 64 hardware interrupts (the number available may be lower due to hardware limits), and 33 soft interrupts. The 32 MEOS style software interrupts are mapped on interrupt 0. Interrupt 1 is available for user usage, though external interrupt signals may not be mapped onto it. *Do not use internal interrupt 2.* It is reserved for MEOS.

Interprocessor interrupts use the same numbering space as external interrupts, and are wired routed through to the second internal interrupt. You should ensure your devicetree allocates them appropriately.

You may install multiple descriptors with the same ``intNum``, but a different ``extNum``, simultaneously. Only the ISR for the most recently installed handler will be invoked, and it is recommended that you use the same handler. ``IRQ_cause`` will return the appropriate descriptor, allowing you to demultiplex the interrupts manually.

IRQ specialisation
++++++++++++++++++

The following public members are available within the ``impspec`` member of ``IRQ_DESC_T``.

core
	The core to map the interrupt to. 0 means self, absolute core indexes are from 1 upwards. This uses the same numbering scheme as ``KRN_proc``.
extNum
	The external interrupt number, as attached to the GIC.
trigger
	The trigger mode for the interrupt: May be one of:

	``IRQ_LEVEL_SENSITIVE``: Trigger based on level.

	``IRQ_EDGE_TRIGGERED``: Trigger based on edges.

	``IRQ_EDGE_DOUBLE_TRIGGERED``: Trigger on two edges.

polarity
	The polarity required to trigger the interrupt. May be one of:

	``IRQ_ACTIVE_HIGH``: For level sensitive interrupts, trigger when high.

	``IRQ_ACTIVE_LOW``: For level sensitive interrupts, trigger when low.

	``IRQ_RISING_EDGE``: For edge sensitive interrupts, trigger on rising edge.

	``IRQ_FALLING_EDGE``: For edge sensitive interrupts, trigger on falling edge.

PIC32
~~~~~

PIC32 interrupts supports a PIC32 interrupt controller attached to a MIPS core. It only supports the EIC interrupt model.

IRQ specification
+++++++++++++++++

The PIC32 IRQ implementation provides support for 32 exceptions, up to 256 hardware interrupts (the number available may be lower due to hardware limits), and 33 soft interrupts. The 32 MEOS style software interrupts are mapped on interrupt 1. *Do not use interrupt 1.* Interrupt 2 is available for user usage.

IRQ specialisation
++++++++++++++++++

The following public members are available within the ``impspec`` member of ``IRQ_DESC_T``.

priority
	The priority to give the interrupt - from 1 (lowest) to 7 (highest).
polarity
	For the 5 external interrupts, the edge to trigger on may be IRQ_RISING_EDGE or IRQ_FALLING_EDGE.

IRQ acknowledgement
+++++++++++++++++++

For interrupts tagged as "persistent" in the PIC32 datasheets, the cause of the interrupt must be dealt with before invoking ``IRQ_ack``, otherwise the interrupt will immediately retrigger.

HWSTAT
~~~~~~

HWSTAT interrupts supports an HWSTAT attached to a MIPS core. It supports only the non-EIC interrupt model.

IRQ specification
+++++++++++++++++

The HWSTAT IRQ implementation provides support for 32 exceptions, 128 hardware interrupts mapped onto 7 vectors, and 33 soft interrupts. The 32 MEOS style software interrupts are mapped on interrupt 0. Interrupt 1 is available for user usage, though external interrupt signals may not be mapped onto it. You may install multiple descriptors with the same ``intNum``, but a different ``extNum``, simultaneously. Only the ISR for the most recently installed handler will be invoked, and it is recommended that you use the same handler. ``IRQ_cause`` will return the appropriate descriptor, allowing you to demultiplex the interrupts manually.

IRQ specialisation
++++++++++++++++++

The following public members are available within the ``impspec`` member of ``IRQ_DESC_T``.

extNum
	The external interrupt number, as attached to the HWSTAT. If ``IRQ_RAW`` is specified, the descriptor will not cause HWSTAT to be reconfigured, and ``IRQ_cause`` will not match it, allowing you to install a catch-all handler. You must ensure that the interrupt is acknowledged manually, i.e. by writing the appropriate value to HWCLEAR.
