Virtualisation allows a system to be split into independent parts, called guests. Each of these guests contains a virtual machine, running independent system software. A hypervisor running at root level, or directly on top of the hardware, manages these guests, controlling their run time and access to hardware.

MVZ and VIO provide a number of components, allowing MEOS to play the role of a type 1 hypervisor on VZ ASE compliant MIPS cores. This modular nature means that a number of different system models can be supported:

Separation kernel
	Lean guests are defined with strictly specified environments and predetermined roles, communicating to form a whole. The separation of concerns into individual guests provides a highly secure system. This is most suited to static systems that do not change.

Virtual systems
	Hardware support is provided by the root, and shared with heavy guests through a generalised environment with virtual devices. This is suited to dynamic systems, where the owner may want to deploy functionality to a heterogeneous network of nodes, e.g. Internet of Things hubs with apps.

Hybrid
	A combination of lean and heavy guests cooperate. This is suited to rich devices that require regulatory compliance, e.g. a router where radios must be strictly controlled, but with the potential for apps to be deployed.

The MVZ framework integrates guests into the core of MEOS as first class entities, equal to tasks. It provides the necessary CP0 emulation to provide a MIPS 24K-like environment. Memory can then be mapped in to provide the simplest form of guest. By sharing some memory mappings between guests, a separation kernel can be realised. See `mvz_comms`_ for a demonstration.

At this point, the guests do not have any external I/O. For a light weight system, registers and interrupts can be mapped 1:1 onto guests. Instead of sharing hardware between guests, inter-guest communications is used to forward requests to the owner. The root can also provide services to the guests through hyperlibraries. These are libraries of user provided root functions that can be called from guests. Please see `Hyperlibraries`_ for details.

Alternatively, VIO can be used to virtualise hardware access, creating a virtual system. It provides Virtio services for constructing virtual systems with MVZ. The root takes direct ownership of the hardware, and arbitrates access from multiple guests through the use of Virtio devices. This allows multiple guests to share hardware, and guests do not require intimate knowledge of the overall system design. By defining and targeting a virtual system specification, a portable guest binary may be produced. This will be runnable on any root system implementing a virtual system compatible with the same specification.

For technical details about the Virtio standard, please see `http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html <http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html>`_.

MVZ Configuration Options
~~~~~~~~~~~~~~~~~~~~~~~~~

The MVZ configuration options can be found in the configuration menu under ``Supported Packages->Hypervision``. These settings apply to the root and all guests.

Dynamic TLB
	This allows the hosting of dynamic systems (see `Static vs Dynamic`_) through the use of a TLB refill handler. Disables ``MVZ_fixMapping`` and enables ``MVZ_addMapping``. Guest execution will potentially be slower.

Filter network packets by MAC
	By default, incoming network packets will be forwarded to all guests sharing an interface. This setting provides privacy by only forwarding incoming packets that are addressed to the MAC declared by the guest. It should be noted that this is only as secure as your network fabric, which in most cases will easily be snooped with a computer running Wireshark.

Emulate guest timers
	Some implementations of VZ do not connect the guest CP0 count/compare timer signal to the guest interrupt system, meaning there will be no virtual timer interrupts. This emulates expected guest timer operation through interrupt synthesis.

Guest interrupt acknowledgement via cause
	Allows a guest to set and clear the IPn bits. This provides an easy way for a guest to clear an interrupt, without emulating hardware. The lightweight IRQ acknowledgement mechanism is described in `Virtual Interrupts`_.

Static vs Dynamic
~~~~~~~~~~~~~~~~~

When you create your system, you need to consider how your system will utilise guests.

In a static system, you will define the guests and their resources at design time. Guest resources must be defined in advance, e.g. fixed ranges of physical memory dedicated to specific guests, fixed register/interrupt mappings, and predefined virtual devices and communication buffers. Guests will be created and configured during system start up.

In a dynamic system, the exact configuration is not known at design time. For example, a remote control centre may request that the system deploys a guest, or the user may download "apps" containing guest binaries. In such a system, guests and resources may be allocated and configured at runtime, and recycled for reuse upon termination.

These two types of system have differing memory mapping requirements. MEOS does not typically use the MIPS TLB mechanism, but MVZ must. The root TLB is used by VZ to virtualise the guest's physical memory.

On a small, static system with only a few guests, all of the TLB entries required for memory mapping can fit into the root TLB. This has the advantage of being fast, reducing code size, and simplifying root access to guest memory. You should use this model if the requirements of your system will allow.

Otherwise, you must use dynamic mapping. This uses the normal style of TLB memory mapping to create TLB entries as required.

If you have a system that is only slightly dynamic, you may consider a hybrid approach - statically reserving known guest configurations, and setting them aside for later use. These will be configured during system start up, but loaded and started during run time.

Demos
~~~~~

mvz_static
++++++++++

The mvz_static demo is only available when the ``Dynamic TLB`` configuration variable is clear.

It demonstrates a static root system which hosts 3 trivial guests. It uses static memory allocation, defined at build time through the use of a Devicetree. The first guest runs the MEOS hello demo, and the second and third guest run simple Mynewt and RIOT demos respectively.

This demo offers a good starting point for building your own statically configured, statically allocated system.

mvz_dynamic
+++++++++++

This demo is only available when the ``Dynamic TLB`` variable is set.

It provides a command line interface that allows guests to be manipulated dynamically at run time, communicating on the default terminal. It uses both dynamic allocation and memory mapping. It also demonstrates the use of Virtio. If the root system supports the necessary hardware, Virtio console and net devices will be attached to the guest. This demo can host virtual machines running Linux. Due to licensing concerns, a virtual machine image is not distributed at this time.

Commands comprise a keyword, and subsequent parameters. The main interesting commands offered by this demo are:

``help``
	Displays a full list of available commands.

``list``
	Lists running and halted guests.

``halt gid``
	Halts execution of the guest with guest ID *gid*. This does not destroy the guest, execution may be resumed with the ``continue`` command.

``continue gid``
 	Resumes running the guest with guest ID *gid*.

``open gid file``
 	Loads ELF *file* via semi-hosting, into the guest with guest ID *gid*.

``new bytes``
	Allocates *bytes* of memory, and uses them to create a new guest, attaching appropriate virtual hardware.

``kill gid``
	Stop and destroy the guest with guest ID *gid*. This releases the resources allocated to the guest.

mvz_comms
+++++++++

The mvz_comms demo is only available when the ``Dynamic TLB`` configuration variable is clear.

This demo shows two guests communicating via a shared buffer, using an inter-guest interrupt to 'kick' transfers. It uses static memory allocation and mapping, and shares a buffer between both guests. Resources are defined and assigned manually, in C code.

If the ``Guest interrupt acknowledgement via cause`` configuration variable is set, then interrupts are acknowledged using a lightweight mechanism. Otherwise, a trivial register interface will be created, and used to acknowledge interrupts. Both of these mechanisms are detailed in `Virtual Interrupts`_.

Starting the Hypervisor
~~~~~~~~~~~~~~~~~~~~~~~

When creating your own system, you must first fully initialise MEOS, and then initialise MVZ.

Before you start the hypervisor, it is strongly advised that you ensure the MEOS timer is started and time-slicing is engaged using ``KRN_setTimeSlice``. If you do not do this, the scheduler will only trigger when a guest executes a ``wait`` instruction, or an ISR causes a reschedule. This will result in guest execution being erratic and unintuitive.

You must provide an instance of ``MVZ_T`` to store system state. You should pass this to the ``MVZ_hypervise`` function, along with one of the following hypercall handler functions:

``MVZ_HR``
	No support for hypercalls. Guests executing ``hypcall`` will be restarted.

``MVZ_HLT``
	Provides support for hyperlibraries.

``MVZ_UHI``
	Allows UHI operations to be dispatched through guest ``hypcalls``. This opens a potential security hole, and should not be used on production systems.

``MVZ_UHIHLT``
	Combines UHI support and hyperlibrary support. This opens a potential security hole, and should not be used on production systems.

``MVZ_hypervise`` will initialise its data structures, configure the VZ ASE, and set up the appropriate exception handlers to service guest exceptions. Once this is done, you may start creating guests.


Creating a Guest
~~~~~~~~~~~~~~~~

There are five steps to creating a new guest:

Initialising a Guest with MVZ_initGuest
+++++++++++++++++++++++++++++++++++++++

``MVZ_GUEST_T`` is an extension of ``KRN_TASK_T`` that adds the additional state required to manage a guest context. ``MVZ_initGuest`` initialises the guest data, but does not start it. This allows the user to configure and assign resources before beginning execution. During initialisation, a guest is assigned a ``gid`` or Guest ID, and a ``start`` function, as described in `The Start Function`_.

Setting Guest Flags for TLBs
++++++++++++++++++++++++++++

There are three ways a guest may use TLBs:

Not at all!
	A small, highly embedded system may run entirely out of ``KSEG0`` or ``KSEG1``.

Dynamically
	A rich guest, such as Linux, will use the TLBs for paging, and will use a TLB refill handler to fix TLB misses.

Statically
	A larger, leaner guest, such as an RTOS, will manually configure TLBs for its needs. Accesses will never miss.

The VZ architecture provides mechanisms for sharing the single set of guest TLBs between multiple guests without the risk of leakage. However, this means one guest may cause the TLB entries for another guest to be evicted. This works fine in the first two cases, since the first does not care, and the second has the capability to refill the TLBs. However, in the third case, there is the significant danger that the TLB entries will be overwritten, which will cause the guest to crash.

Use the ``MVZ_setFlags`` function to set the guest's ``MVZ_GUEST_FLAG_SAVETLBS`` flag to solve this problem. This flag makes MVZ save and restore the guest TLBs on a context switch. This slows down context switch, but ensures the guest operates as expected. This should be done immediately after the guest has been initialised.

Assigning Resources
+++++++++++++++++++

A freshly initialised guest has no memory or I/O devices. Resources must be assigned to it before it may be started. This should only be done once, after initialisation. Please see `Guest Resources`_ for more details.

Starting a guest with MVZ_startGuest
++++++++++++++++++++++++++++++++++++

``MVZ_startGuest`` is equivalent to ``KRN_startTask``. It adds a guest to the MEOS scheduler at a specified priority, and starts executing it. The ``start`` function that was assigned to the guest at initialisation is called to assist in booting the guest.

Once a guest has been started, it can be restarted using ``MVZ_restart``.

The Start Function
++++++++++++++++++

The ``start`` function is called when a guest is initially started, explicitly rebooted, or restarted due to a fatal guest error. A ``start`` function should:

* Zero RAM using ``MVZ_zeroGP`` or ``MVZ_zeroGV``. This improves security by reducing the risk of data leaking.
* Load a binary. This can be done manually using ``MVZ_writeGV`` and ``MVZ_writeGP``, or from ELF format data using ``MVZ_loadELF``.
* Write any extra data to RAM using ``MVZ_writeGV`` and ``MVZ_writeGP``.
* Set initial guest register state. This is done by fetching and modifying guest context structures using ``MVZ_gpCtx`` and ``MVZ_cp0Ctx``. This is useful if loading a binary does not set the initial PC, or registers require specific values for the UHI boot protocol.

You may also want to use this function to reset any custom virtual devices or hyperlibraries associated with the guest.

Guest Resources
~~~~~~~~~~~~~~~

There are a number of different resources you may wish to attach to a guest:

Memory
++++++

On a static system, the ``MVZ_fixMapping`` function is available. This function allows you to create TLB entries at system start up that map guest physical memory to root physical memory. You must pass in a pointer to a variable containing the index to start creating entries from. The variable will be incremented depending on the number of entries created.

On a dynamic system, you must instead use the ``MVZ_addMapping`` function. This fills in a supplied data structure with a description of the mapping desired, and attaches it to the specified guest. This mapping will be used at run time in the TLB refill handler to create appropriate TLB entries mapping guest physical memory to root physical memory.

Some implementations of the VZ ASE share one TLB between the root and guests. For such systems, the ``MVZ_splitTLB`` function allows the split to be defined.

Shared Memory
+++++++++++++

The same piece of physical memory can be assigned to more than one guest. This allows it to be used for communication between guests, though communication can be enhanced through the use of inter-guest kick interrupts. Please see `Virtual Interrupts`_ for details on creating these interrupts.

Registers and Interrupts
++++++++++++++++++++++++

If you wish to exclusively give a block of registers to a single guest, you can map them to a guest in the same way that you would map memory. Please see `Memory`_ for details.

``MVZ_intMap`` can be used to get the hypervisor to forward a specified interrupt to a guest. Note that the registers and interrupt must be exclusively given to the guest: they may not be given to another guest, nor may they be used by the root. This makes this method suitable only for devices that do not use shared interrupts or interlaced registers.

If you need to share a single piece of hardware, or arbitrate access, you can either virtualise it (see `Virtual Registers`_ and `Virtio`_), or you can create a hyperlibrary of functions to abstract access (see `Hyperlibraries`_).

Virtual Registers
+++++++++++++++++

The ``MVZ_REGS_T`` data type is used to define a virtual register bank, which is then attached to a guest using ``MVZ_addRegs``.

The ``.start`` and ``.stop`` members define the extent of the registers in the guest physical address map.

The ``.read`` and ``.write`` functions are callbacks that will be used when virtual registers are accessed from the guest. These can be used to mediate or emulate access to resources. Both must be defined for register access to be enabled.

``.prepare`` is a callback that will be called prior to a guest context being reactivated. This can be used to perform any necessary context switching actions for the device.

Your device may additionally require one or more virtual interrupts.

Virtual Interrupts
++++++++++++++++++

The ``MVZ_upInt`` and ``MVZ_downInt`` functions can be used by the root to synthesize and clear guest interrupts for virtual devices. Depending on the interrupt hardware attached to your core, these may be edge based (no ``MVZ_downInt`` required), level based (``MVZ_downInt`` will always clear), or counting (number of calls to ``MVZ_downInt`` must match number of calls to ``MVZ_upInt``).

If the ``Guest interrupt acknowledgement via cause`` configuration variable is clear, you will have to implement a virtual device that provides a means of acknowledging the interrupt. That device should use ``MVZ_downInt`` to clear the interrupt condition, otherwise the guest will repeatedly retake the interrupt.

If the ``Guest interrupt acknowledgement via cause`` configuration variable is set, MVZ will expose a lightweight mechanism to the guest for acknowledging virtual interrupts that can be used instead. The IPn bits in CP0.Cause will become read/write instead of read only. The guest may then acknowledge interrupts by clearing the relevant bit, clearing the interrupt. Such an arrangement is not normally compatible with systems using interrupt controllers, e.g. ICU, GIC, etc.

By defining virtual registers with ``.read`` and ``.write`` set to ``NULL``, a virtual device may be defined with no presence within the memory map. This may not sound useful, but the ``.prepare`` function will still be invoked upon guest activation. This is useful for inter-guest communication: it allows requests to be queued and automatic inter-guest kick generation to be deferred until context switch (e.g. on time-slice), rather than manually invoking a switch for each transaction. The ``.prepare`` function can check to see if there is data waiting, and if there is, call ``MVZ_upInt``.

Virtio
++++++

VIO provides an implementation of Virtio; this simplifies the instantiation of virtual devices. These devices are supported by appropriately configured Linux and MEOS kernels out-of-the-box with a suitable Devicetree. The following devices are provided by VIO:

VIO_console
 	This provides a Virtio console compliant device to a guest, creating a virtual console/serial port. ``VIO_initConsole`` will create an instance that is attached to a ``UART_T`` compatible driver. Alternatively, ``VIO_initFileConsole`` can be used to create an instance that uses a stdio ``FILE`` handle as a back end.

VIO_net
	This provides Virtio net services to a guest, connecting it to the network via an LwIP packet driver. ``VIO_setMAC`` can be used to give each guest a unique MAC address. By default, all incoming packets are delivered to all guests sharing the same interface. The ``Filter network packets by MAC`` configuration variable ensures packets are only delivered to devices with matching MAC addresses.

VIO_dummy
	This acts a virtual "blanking plate". Guest reads and writes will succeed, but it does not provide nor advertise any services. This is useful for filling gaps in the guest that are not implemented on a specific root platform, enabling portable guests.

VIO itself provides the common Virtio infrastructure: new Virtio services can be built using it.

Creating New Virtio Devices
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``VIO_TEMPLATE_T`` (support/1/mvz/vio/vio_template.c and support/1/mvz/vio/vio_template.h) can be used as a starting point for defining your own Virtio devices. It provides the infrastructure for providing a Virtio compatible device to the guest. It is suggested that you duplicate the source, rename the files and the type, add the required additional registers, and wire the queues up to data sources/sinks. Comments and directives in the source code will tell you how to do this.

Hyperlibraries
~~~~~~~~~~~~~~

MVZ supports providing hyperlibraries to guests. For the purposes of MVZ, a hyperlibrary is defined as a collection of functions provided by the root, but callable by guests.

Since the guest stack is not intimately shared with the root, the functions in hyperlibraries must be specifically created for the purpose - you cannot use a normal user library! Only 4 parameters are passed from the guest to the root, and pointers must be treated carefully. You may not directly derefence them; instead you must use MVZ to transfer data from the root to the guest and back.

hlt
+++

MVZ provides the ``hlt`` tool to generate hyperlibrary thunks which deal with the hypercalls:

.. code :: sh

	$MEOS_DIR/bin/hlt -r root.S -g guests.S library.a...

Where ``$MEOS_DIR`` is the directory containing MEOS build products, ``root.S`` is the name of the file to create that contains generated root code, ``guests.S`` is the name of the file to create that contains generated guest code, and ``library.a`` is one or more library files that should be analysed.

``hlt`` takes one or more .a libraries, and generates thunking assembler that is used to facilitate hypercalls. You should assemble ``root.S`` and link it to the root along with your hyperlibrary ``.a`` files. ``guests.S`` should be assembled and linked with your guests. Guests may then call hyperlibrary functions as though they were normal functions.

Implementing hyperlibrary functions
+++++++++++++++++++++++++++++++++++

All hyperlibrary functions are available to all guests. To restrict access to specific guests, use the ``KRN_me`` function to get a pointer to the ``MVZ_GUEST_T`` representing the guest that made the call. If the current guest is not allowed to call your function, you may return early with an error code, use ``DBG_assert``, or use ``MVZ_restart`` to reboot the guest.

It is strongly recommended that you encode all required data for the hyperlibrary call directly in the 4 available arguments. If this is not possible, you may use the ``MVZ_readGV`` and ``MVZ_writeGV`` functions to access data in guest memory. Note that performing the address translations required for such an operation is a substantial amount of work, so you should attempt to minimise the number of individual calls to these functions if possible. Take care when using these functions to avoid creating buffer overruns, otherwise you will compromise the security of the system.

Inter-guest Communication
~~~~~~~~~~~~~~~~~~~~~~~~~

The best way for guests to communicate under MVZ is for them to share memory: the same piece of root virtual memory is mapped into more than one guest. A virtual register interface or a hyperlibrary call may then be used to issue a 'kick' interrupt to indicate to the other side that the buffer requires servicing. An example is shown in mvz_comms demo - please see `mvz_comms`_.

Shutting down a running guest
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before the resources assigned to a guest can be reassigned, the guest using them must first be shut down. ``KRN_removeTask`` will remove a guest from the scheduler, halting execution. You may then perform any operations required to shut down any associated devices, e.g. call ``VIO_kill`` to detach Virtio devices. Once everything has been detached, any memory associated with a guest can be recycled as needed. If you are using a dynamic system, it is recommended that you call ``mips_tlbinvalall`` to ensure TLB entries do not leak between instances. This is demonstrated in the ``MC_kill`` function in the mvz_dynamic demo - please see `mvz_dynamic`_.
