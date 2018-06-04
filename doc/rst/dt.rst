Devices
~~~~~~~

MEOS attempts where possible to use widely standard devicetree bindings. Specifications for many bindings may be found at `https://www.kernel.org/doc/Documentation/devicetree/bindings/ <https://www.kernel.org/doc/Documentation/devicetree/bindings/>`_ . For other devices, you should ask your hardware supplier to provide you with the necessary binding.

Processors
~~~~~~~~~~

If a devicetree processor node is compatible with "mti,mips", MEOS will try to
use the first associated clock to calibrate timers.

MEOS also adds a custom extension for laying out multithreaded applications.
This allows the supported methods of building MEOS applications to adjust the
parameters of the UHI linker script.

If the node is compatible with "mti,embedded-cpu", the 2 field "mti,vmem"
attribute will be used to set the start and size of memory used by the
application built for that processor. The first field specifies the starting
virtual address, the second the number of bytes to use.

.. code:: c

	cpu@0 {
		compatible = "mti,embedded-cpu", "mti,mips";
		mti,vmem = <0x80000000 0x800000>;
		clocks = <&CPU_CLK>;
	};

MIPS embedded CPU
+++++++++++++++++

Required properties:
 - compatible = "mti,embedded-cpu";
 - mti,vmem : virtual address and size of memory

Optional properties:
 - mti,vstart : virtual address of application entry point

Interprocessor messaging buffers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If MEOS processors are interacting with each other, you must specify the interface in the devicetree.

A node compatible with "mti,meos-shared" is used to specify the discovery space used by all cooperating processors to discover each other and synchronise. The first field of the 2 field "reg" attribute specifies the starting virtual address, the second the number of bytes to use. This memory must be visible to all cooperating processors before starting the import/export subsystem.

Each processor must have a corresponding "mti,meos-ring" node. This is used specify the interprocessor interrupts that will be used for signalling by the vrings underlying the import/export subsystem.

.. code:: c

	rings {
		compatible = "mti,meos-shared";
		mti,vmem = <0x817ff000 0x1000>; /* Chunk from the end of RAM, otherwise unused */

		processor@0 {
			compatible = "mti,meos-ring";
			interrupt-parent = <&gic>;
			interrupts = <0 38 0>; /* Interrupt unused by hardware devices */
		};

		processor@1 {
			compatible = "mti,meos-ring";
			interrupt-parent = <&gic>;
			interrupts = <0 39 0>; /* Interrupt unused by hardware devices */
		};
	};

MEOS shared memory
++++++++++++++++++

Required properties:
 - compatible = "mti,meos-shared";
 - mti,vmem : virtual address and size of memory

MEOS ring kick
++++++++++++++

Required properties:
 - compatible = "mti,meos-ring";
 - interrupts : interrupt connection

remoteproc
~~~~~~~~~~

If your primary system is Linux based, and all MEOS processors are booted via remoteproc, you do not need to use the devicetree to specify the interprocessor interface.

Instead, build your application with librproc. To make the library available, enable ``Kernel features->Virtio Vrings`` in the MEOS configuration. You can then specify ``RPROC=y`` in your project ``specs.mk`` file, and the build system will pull in libproc.

librproc handles discovery and synchronisation with a Linux primary system, and ensures the resultant ELF contains the appropriate resource table entries required by the remoteproc loader.

A hybrid system is one that combines a Linux controlled system and independent processors running MEOS. To achieve this, you must define devicetree nodes If you wish to use a hybrid system, with some processors not under remoteproc control, you will need to define devicetree nodes for all MEOS processors.
