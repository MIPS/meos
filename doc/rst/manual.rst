Introduction
------------

The MIPS Embedded Operating System
==================================

The MEOS (MIPS Embedded Operating System) provides a framework that allows systems programmers to design systems as loosely coupled collections of tasks and device driver modules, with the details of scheduling, synchronisation and communication being handled by a standardised and well tested environment.

A three stage process is used to define a system:

1) The hardware environment is defined using an industry standard devicetree file. This specifies how to use memories and other devices, and is used to automatically configure enabled drivers.
2) The software environment is defined using a menu-driven configuration tool. This activates the features of the kernel and packaged middleware suites, seamlessly integrating with the hardware environment.
3) You write an application, with the kernel providing a pre-emptive, prioritised, multi-tasking software environment, and the middleware suites simplifying services.

Defining Your Hardware Environment
==================================

The devicetree standard provides a means of describing the hardware available to a system. The specification is available from `https://www.devicetree.org <https://www.devicetree.org>`_. MEOS takes a devicetree source (.dts) file at build time, and uses it to produce a board support package and configure the build system.

MEOS comes with a number of .dts files built in for a selection of evaluation platforms. If you are using one of these, you can select it when configuring MEOS.

For other systems, if your platform is supported by Linux using a .dts file, you may be able to specify it when later configuring MEOS. It is important that the CPU clock is specified correctly within this file, since it will be used to calibrate MEOS timers.

Otherwise, you may have to write your own .dts file. How to do this is beyond the scope of this document.

To use the MEOS import/export mechanism, you may have to customise your devicetree - please see `Devicetrees and MEOS`_.

Defining Your Software Environment
==================================

The MEOS build system is responsible for configuration, compilation, and installation of MEOS.

Using the MEOS Build System
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Prerequisites
+++++++++++++

MEOS can only be built on a Linux host. It is suggested that users of other desktop operating systems should build MEOS in a virtual machine.

The CentOS, Debian, Fedora, and Red Hat distributions are fully supported. On these platforms, the MEOS build system will automatically try to install any necessary missing prerequisites.

The Ubuntu distribution is also supported, but due to it's consumer nature, requires additional steps to allow it to work. ``make`` must be installed with the following command before the rest of these instructions may be followed:

.. code:: sh

	sudo apt-get install make

On unsupported Linux distributions, you will need working host GNU tools, including a C/C++ compiler, a Python installation, and curl or wget.

To run the test suite on real hardware, you will need a suitable debug probe. If Codescape Console is not available, MEOS will automatically install it. For simulated testing, you will need an existing installation of Codescape Console, featuring the simulation Hardware Support Packages and suitable licenses.

Invoking make
+++++++++++++

The MEOS build system is controlled using ``make``. You must specify a destination directory for build products using the ``BUILD_DIR`` environment variable:

.. code:: sh

	make BUILD_DIR=~/meos.build

**``BUILD_DIR`` may not be a subdirectory of the MEOS source directory**. The same value must be specified for all make invocations, or exported into the shell environment.

Configuring MEOS
~~~~~~~~~~~~~~~~

MEOS requires configuring for its target environment:

.. code:: sh

	make BUILD_DIR=~/meos.build config

This will present a menu of options, and then generate a ``meos.cfg`` file in ``BUILD_DIR``.

Alternatively, an existing configuration file may be supplied:

.. code:: sh

	make BUILD_DIR=~/meos.build CONFIG=~/meos.cfg config

The same value of ``CONFIG`` should again be specified for all invocations of make, or exported into the shell environment.

Complete descriptions available options are provided in `Configuration options`_, but some of the important ones include:

Board Support Package
	Allows the target board to be selected. If support is not available for your board, the ``Custom`` option allows a .dts file to be specified, as described in `Defining your hardware environment`_. Alternatively, selecting ``None`` allows MEOS to be built without tailored board support - note that you must call ``TMR_setClockSpeed`` for timers to work correctly.

Supported packages
	This menu lets you select which middleware suites you want to be built for MEOS.

Target | Target architecture
	This defaults to Linux for testing. Changing this to MIPS will reveal a number of options for targeting your specific core.

MIPS configuration | MIPS toolchain
	This default to the Codescape mips-mti-elf toolchain.

Toolchain configuration | Optimisation level
	The optimisation level defaults to 0 to ease debugging during development. For production, you will want to increase it.

Toolchain configuration | Debug level
	The debug level defaults to 3, again to ease development, and may be reduced for production.

Debugging facilities | Diagnostics
	`Debug features and instrumentation`_ describes these options in more detail. The default settings are strongly advised for development, but should be disabled for production builds.

Building the Binaries
~~~~~~~~~~~~~~~~~~~~~

To build MEOS, invoke the default target:

.. code:: sh

	make BUILD_DIR=~/meos.build

To rebuild MEOS from clean:

.. code:: sh

	make BUILD_DIR=~/meos.build clean all

Testing Your Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~

On a Linux Target
+++++++++++++++++

To build and execute the test suite, execute:

.. code:: sh

	make BUILD_DIR=~/meos.build test

On a Baremetal MIPS Target
++++++++++++++++++++++++++

To build and execute the test suite on an attached probe via Codescape, run:

.. code:: sh

	make BUILD_DIR=~/meos.build test PROBE=sp09

The value of ``PROBE`` selects the device to use. Alternatively, if ``PROBE`` is exported into the shell environment, it may be left out.

If you have the Codescape simulation Hardware Support Packages installed, you can run the tests under simulation. In the configuration tool enable *Tests | Simulated testing* and set *MIPS configuration | MIPS model* to the name of the simulation model you would like to use. Then execute:

.. code:: sh

	make BUILD_DIR=~/meos.build test

Building the Demos
~~~~~~~~~~~~~~~~~~

To build any configured demos, execute:

.. code:: sh

	make BUILD_DIR=~/meos.build demos

This will create a number of subdirectories within your build directory, containing the selected demos.

Defining Your Application
~~~~~~~~~~~~~~~~~~~~~~~~~

It is recommended that user projects utilise the provided MEOS project build system. It will greatly simplify compilation and linkage of your application with your customised build of MEOS and the suite of supported packages, as well correctly managing multiprocessor ensembles. This document will assume you follow the recommended path. If you wish to use another build system, please see `Manual Integration`_.

Creating a New Project
++++++++++++++++++++++

The project build system is contained within the ``project`` sub-directory of your MEOS source. To start a new project, copy it and all subdirectories to a new directory:

.. code:: sh

	cp -R project ~/mynewproject

Customising Your Project
++++++++++++++++++++++++

Within your new project, the specification file ``spec.mk`` describes where to find the source code, and how to turn it into target binaries.

A Simple, Single Processor Application
++++++++++++++++++++++++++++++++++++++

As a simple example, let's analyse the default specification:

.. code:: make

	PY := bin/main.py

	LOG := logs/out.log

	ELF0 := bin/main-0.elf

	SRC0 := src/main.c

``PY`` is used when building for baremetal MIPS targets. A Codescape script will be written to this file. When used as a load script in Codescape, it will reset your board, and load your application in preparation for execution.

``LOG`` specifies a log file to use when the ``test`` target is invoked. Output from running your application will be written to this file.

``ELF0`` names the output ELF file to be built for the first and only processor. This will be your fully linked executable file.

``SRC0`` lists the source files that will be compiled and then linked with MEOS and your selected suite of middleware to produce ``ELF0``

Building your first project
+++++++++++++++++++++++++++

From the base directory of your project, run:

.. code:: sh

	make BUILD_DIR=~/mynewproject.build MEOS_DIR=~/meos.build

``BUILD_DIR`` should contain the directory where you want the build products for your project to go. ``MEOS_DIR`` should contain the directory containing the MEOS build products, i.e, the ``BUILD_DIR`` you used when building MEOS. Alternatively, you may export these variables into the shell environment.

To clean and rebuild, execute:

.. code:: sh

	make BUILD_DIR=~/mynewproject.build MEOS_DIR=~/meos.build clean all

Running your project
++++++++++++++++++++

To run your project, execute:

.. code:: sh

	make BUILD_DIR=~/mynewproject.build MEOS_DIR=~/meos.build run

This operates in the same way as the MEOS test system described in `Testing Your Configuration`_. If *Tests | Simulated testing* is selected in the MEOS configuration, the app will be run under simulation. Otherwise, if you are running on baremetal MIPS, you may use ``PROBE`` to select a debug probe.


Making your project configurable
++++++++++++++++++++++++++++++++

The project build system supports configurable projects:

.. code:: sh

	make BUILD_DIR=~/mynewproject.build MEOS_DIR=~/meos.build run

By default, there are no configurable options. By editing the ``Kconfig`` file in your project directory, you can add options. The language used is defined in `https://www.kernel.org/doc/Documentation/kbuild/kconfig-language.txt <https://www.kernel.org/doc/Documentation/kbuild/kconfig-language.txt>`_. To use these option in your application, include ``config.h`` in your source.

.. code:: c

	#include "config.h"

The symbols defined in the Kconfig file will have ``CONFIG_`` prepended.

Building a multiprocessor application
+++++++++++++++++++++++++++++++++++++

As long as you have the appropriate extension nodes in your devicetree (see `Devicetrees and MEOS`_), building a multiprocessor ensemble is almost as easy as building a single processor application:

.. code:: make

	PY := bin/main.py

	LOG := logs/out.log

	ELF0 := bin/main-0.elf
	ELF1 := bin/main-1.elf

	SRC0 := src/main.c src/t0.c
	SRC1 := src/main.c src/t1.c

For each ``SRCn``, then corresponding ``ELFn`` will be built. The load script ``PY`` will then ensure every ELF file is loaded. So long as you are using a board support package, you can then use the features described in `Hardware Multi-processing`_.

The Core
========

The MEOS kernel provides a pre-emptive, prioritised, multi-tasking software environment. This simplifies the task of systems programmers, by allowing them to design systems as loosely coupled collections of tasks and device driver modules, with the details of scheduling, synchronisation and communication being handled by a standardised and well tested environment.

MEOS is designed to be compact, and simple. It offers:

*    A multi-tasking environment.
*    Abstractions of low level processor hardware.
*    An application "style" suitable for real-time data-intensive systems, focussing on passing pointers to data buffers, and avoids copying data wherever possible.
*    Simple support for communication between tasks running on different cores.

Beyond satisfying these aims, the kernel avoids any features which would add size or complexity. It is intended for use in simple, relatively unsophisticated embedded systems. If you find yourself needing features like virtual memory, inter-task protection or demanding hard real-time performance guarantees, then this kernel is not for you. This is not to say that the kernel doesn't perform well enough for many real-time systems, just that we cannot guarantee it, because actual performance will vary depending upon configuration and what each designer chooses to do with the system. Other more sophisticated operating systems are also available, this is the one for simple embedded systems.

Contents of MEOS
================

MEOS is a collection of packages that can be combined to provide the supporting stack for your application. Although the different packages are obvious through the naming of the functions, you need include only one header file and link with only one library.

The packages are broadly classified as either foundations, abstractions, utilities, or supplemental:

Foundations
~~~~~~~~~~~

	Lists
		A general purpose module for managing simple single linked lists. Objects defined by this module have names starting with ``LST_``.

	Dqueues
		A general purpose module for managing double linked lists (generally referred to as queues). Objects defined by this module have names starting with ``DQ_``.

	Trees
		A general purpose module for managing tree structures. Objects defined by this module have names starting with ``TRE_``.

	Rings
		A general purpose module for managing fixed sized FIFO byte buffers. Objects defined by this module have names starting with ``RING_``.


Abstractions
~~~~~~~~~~~~

	IRQ
		An abstraction of the target platform's interrupt system. Objects defined by this module have names starting with ``IRQ_``.

	Memory
		An abstraction of the target platform's memory system. Objects defined by this module have names starting with ``MEM_``.

	Timer
		An abstraction of the target platform's timer system. Objects defined by this module have names starting with ``TMR_``.

Utilities
~~~~~~~~~

	Kernel
		The multi-tasking scheduler system. Objects defined by this module have names starting with ``KRN_``.

	IPM
		The inter-processor messaging system. Objects defined by this module have names starting with ``IPM_``.

	MQ
		Virtio vring compatible message queues. Used to implement inter-processor messaging and interaction with outside systems. Objects defined by this module have names starting with ``MQ_``

	QIO
		Support for interrupt driven input/output. Objects defined by this module have names starting with ``QIO_``.

MEOS Version Identification
===========================

The ``MEOS.h`` header file defines the following macros, to allow you to identify which version of MEOS you are using.

``MEOS_MAJOR_ID``
	The major version number. The major version number is incremented when the API is *changed* or the behaviour of the package is altered in ways that may be incompatible with existing software.

``MEOS_MINOR_ID``
	The minor version number. The minor version number is an integer, which is incremented when the API is *extended* or when internal changes, compatible with existing software, are made.

``MEOS_ID_STRING``
	A quoted string derived from the major and minor ID values. In addition the string may contain indications of the development or bug-fix status of a MEOS version.

For example, in MEOS version 1.0, ``MEOS.h`` contains the following:

.. code:: c

	#define MEOS_MAJOR_ID 1
	#define MEOS_MINOR_ID 0
	#define MEOS_ID_STRING "1.0"

Programs which need to adjust to the particular MEOS version in use should do so by testing ``MEOS_MAJOR_ID`` and ``MEOS_MINOR_ID`` only.

``MEOS_ID_STRING`` is intended only for display to human readers, e.g. in banner text messages shown at system startup. Although its definition is derived from the ID values, its precise syntax may vary. For example a series of  "beta" release of MEOS 1.1 would all have the same ``MEOS_MAJOR_ID`` and ``MEOS_MINOR_ID`` values, but ``MEOS_ID_STRING`` might have values like ``"1.1B0"``, ``"1.1B1"`` etc.

In-line Directives
==================

The functions in the lists (``LST_``), dqueues (``DQ_``) and rings (``RINGS_``) modules are generally very simple. It can sometimes be advantageous to compile these "in-line". This reduces function call overheads and also provides the C compiler with more scope for optimisation.

You can select in-lining by defining the following macros, either using the ``-D`` argument on the compiler command line, or by using ``#define`` prior to including ``MEOS.h``:

``LST_INLINE``
	Causes in-lining of the lists module

``DQ_INLINE``
	Causes in-lining of the dqueues module

``RING_INLINE``
	Causes in-lining of the rings module

It isn't necessary to apply these directives universally. It is permissible to use in-lining on just a selection of your source code modules.

The Kernel and Task Scheduling
------------------------------

Terminology
===========

We need to clarify some terminology before proceeding.

Software engineers familiar with multi-tasking concepts will recognise that what this kernel provides is a software multi-threading environment in which co-operating threads share the same memory space. That is to say:

*    Different threads may execute the same program code.
*    Different threads may access the same data memory (and must adopt suitable protection conventions to avoid interference).

In conventional software engineering parlance the phrase "multi-tasking" is sometimes taken to imply rather more than this. For example tasks may execute under the supervision and control of an "operating system". They may have independent memory spaces and there may be mechanisms to prevent one task disrupting the execution of another. None of this applies to MEOS.

In this document we use the word "task" to refer to one of our software threads.

This document will use the word "processor" to refer to a hardware execution context, e.g. core, VPE, or VP.

Features and Facilities
=======================

MEOS provides support for the following features:

*    Task creation, prioritisation and removal.
*    Pre-emptive task scheduling with yield.
*    Idle mode.
*    Critical region protection.
*    Memory resource management via object pools.
*    Communication and synchronisation via semaphores, event flags, mailboxes, resource locks, and synchronisation barriers.
*    Tickless timer management, providing:
     *    Operation timeouts.
     *    Timer functions for repeating and one-shot events.
     *    Optional time slicing.
*    Interrupt configuration and routing.
*    QIO system for device driver and I/O management.
*    Synchronisation with software on other processors.

Task scheduling is pre-emptive. The choice of active task is re-evaluated each time a significant event (device interrupt, timer tick, or voluntary yield) occurs.

Managing memory
~~~~~~~~~~~~~~~

Apart from a small amount of static data which the kernel uses for managing the task scheduler, interrupt enable state, and some abstraction specific data, the kernel does not manage any memory directly. The kernel and its API are designed around the principle that buffers, control blocks, stacks, etc. are allocated by the system designer and passed as arguments to kernel functions. Memory management is part of the application.

This approach leaves the maximum flexibility to size and locate memory items as required, albeit by requiring the system designer to do a little more work. However, the kernel and the other modules from which it is built provide a number of facilities to ease this burden. `Memory Management`_ discusses these issues in more detail.

Start-up
========

Start-up normally involves a standard sequence of activities:

1.  A single call to ``KRN_reset``. This will also initialise the interrupt system.
2.  A single call to ``IPM_init``, to identify the IPM conduit for communication, followed by a call to ``KRN_installImpExp`` to install import/export tables and message processing code. This step should be omitted if you do not use the import/export feature, to reduce the code and data memory requirements of your system.
3.  A single call to  ``KRN_startOS``. This creates a task context and starts the scheduler. All code following the ``KRN_startOS`` call executes in the context of this first task which uses the system stack and initially executes at the lowest possible priority. At this point, scheduler interrupts are enabled.
4.  Optionally, I/O system initialisation, consisting of calls to ``QIO_init``, one per device. It is not essential to initialise devices at this stage. They may be initialised any time after ``KRN_startOS`` but before they are used. However, run-time behaviour will be more predictable if all device initialisation is done during the start-up sequence.
5.  Optionally, a single call to ``KRN_startTimerTask``. You provide a stack for this task and define the timer tick period in microseconds. The tick period defines the granularity of the timing system and should be as long as possible consistent with your requirements. Shorter tick periods incur more CPU overhead, while longer tick periods make timing more prone to quantisation effects. If you don't use any of the timer or finite time-out features of the kernel, you can omit this call, and immediate and infinite time-outs will still operate correctly. After this call, both timer and scheduler interrupts are enabled.
6. Optionally, a single call to ``BSP_init``. This initialises all selected supported packages.

Once this is complete, the start-up code can then continue to activate the rest of the system. Normally this involves:

*    Initialising shared data objects such as mailboxes, semaphores, etc.
*    Starting other tasks.

Designing the start-up sequence requires some care. You must ensure that there is no possibility of a shared data object being used by any task before the object has been initialised.

Tasks and Stacks
================

Each task is defined by:

A task control object
	This has type  ``KRN_TASK_T``. You don't need to know about the structure of this object. You simply need to declare one for each task in your system. You provide the object to ``KRN_startTask`` when you create the task and you must ensure that it is not re-used for as long as the task is active. When you need to identify a task to a kernel function call, you do so by providing a pointer to the task object. These task pointers can be treated as anonymous handles. The structure of the objects to which they point should be treated as private.

A stack
	Each task has its own stack. You declare a data area, which should be 64-bit aligned, and is typically an array of ``uint32_t``, and pass it to ``KRN_startTask``. You must ensure that the stack remains in existence for as long as the task is active and you must not modify its contents directly in any way.

A priority
	Each task has a priority. Task priorities are integers in the range ``KRN_LOWEST_PRIORITY`` (0) to the value you specified when calling ``KRN_reset``.

A name
	Task names are ``NUL``-terminated character strings. Each task is given a name by passing a pointer to a (constant) string as an argument to ``KRN_startTask``. You can provide a ``NULL`` name argument if you don't want to name a task. Task names are not required, but they are highly recommended since they assist debugging.

A function
	The code executed by a task is contained in a function with the signature ``void func(void)``.

A call to ``KRN_removeTask(NULL)`` will never return - the current task is removed from the system's schedule first.

Tasks are started by passing the information listed above to ``KRN_startTask``. Tasks can be terminated by calling ``KRN_removeTask``. When you terminate a task, you must be very sure that it is not holding any resources (locks, semaphores, etc.) which are needed by the surviving tasks. The system has no facilities to release resources when a task is killed. This is not a problem in practice. The vast majority of the systems for which MEOS is intended simply create a fixed set of tasks at start-up.

Two tasks in the system are special:

*    The initial start-up task has a predefined stack and control object. You don't need to provide these to ``KRN_startOS``. It defaults to the lowest priority so you don't need to provide that either.
*    Like the start-up task, the timer processing task has a predefined control object and priority, but you need to provide a stack.

Stack Sizes
~~~~~~~~~~~

The system will fail if a task's stack overflows. MEOS will only assert if the stack has overflowed on a context switch if support is enabled in the configuration. Task stacks are used for C call frames and automatic variables. The size needed will depend on the maximum function nesting level and the automatic data that the functions use.

Interrupt stacks
~~~~~~~~~~~~~~~~

An interrupt stack must be provided for use by ISRs, including the scheduler. This allows individual task stacks to be made smaller.

For example, the following code declares an interrupt stack, which is passed to MEOS via ``KRN_reset``:

.. code:: c

	/* Global definition */
	uint32_t istack[STACKSIZE];

	...

	/* In initialisation code */
	KRN_reset(&sched, schedQueues, MAX_PRIORITY, 0, istack, STACKSIZE, NULL,
		  0);
	KRN_TASK_T *bgtask = KRN_startOS("Background Task");

The KRN_stackInfo function
~~~~~~~~~~~~~~~~~~~~~~~~~~

``KRN_stackInfo`` can be used during development to measure how much space has been used on a task's stack or on the interrupt stack. However, to use ``KRN_stackInfo``:

*    You must provide a non-zero ``stackInit`` value to ``KRN_reset``.
*    The stacks must be large enough for proper operation. The normal procedure is to size stacks generously, use ``KRN_stackInfo``, and then reduce stack sizes depending on the results.

Be aware that activating ``KRN_stackInfo`` increases the time taken to start a task, so it may affect the operation of your system slightly.

Unfortunately, ``KRN_stackInfo`` cannot provide any information about the stack used by the initial start-up task. You will need to investigate this using other development tools and use the linkage tools to set the stack size accordingly.

Priorities
~~~~~~~~~~

Task priorities are set initially by ``KRN_startTask``, but may be altered later by ``KRN_priority``.

The scheduler runs the highest priority task that is not waiting for some event. If more than one task has the same priority, then these are scheduled in round-robin fashion at each scheduler event. The default scheduling algorithm suits "data driven" designs in which most tasks are blocked waiting for data. If you have tasks which perform significant computation, you may wish to consider enabling time slicing (See `Time Slicing`_).

You control the number of available priority levels with the ``maxPriority`` argument to ``KRN_reset``. You can choose any number between 2 and ``(CONFIG_FEATURE_MAX_PRIORITIES - 1)``. The default value of ``CONFIG_FEATURE_MAX_PRIORITIES`` is 256. However, the more priority levels you use, the greater will be the scheduling overhead, so we recommend you use only as many as your application requires. You can expect the time taken to perform a task switch to increase for each additional 32 priority levels used by your system. While you don't need to trim the number of priority levels aggressively, you should not configure too many more than you actually need.

The timer task, which manages finite time-outs and timers, must be the single highest priority task in the system. The ``maxPriority`` level is reserved for the timer task.

``KRN_startTask`` and ``KRN_priority`` both prevent you from setting a task priority any higher than ``(maxPriority - 1)``.

Idling
~~~~~~

In many systems, it is common to have a low priority background or idle task which will run when no other task can. Ideally, this model should *not* be used in a MEOS system, since it consumes resources that could be used elsewhere. MEOS deals with the situation, where no task can run, by "stalling" the processor. This saves power and releases processing cycles for other virtual processors.

If for some reason you still need an idle task, you can do this is by having a task execute the following loop:

.. code:: c

	for (;;)
		KRN_release();

This repeatedly calls the scheduler and will defer to any other task of equal or higher priority.

Synchronisation and Communication
=================================

MEOS offers a number of mechanisms for synchronisation and communication between tasks:

*    Semaphores
*    Resource Locks
*    Mailboxes
*    Event Flags
*    Synchronisation Barriers
*    Pools
*    Work Queues

Semaphores
~~~~~~~~~~

A semaphore is a synchronisation object that has a positive integer *value*. The initial value is set when the semaphore is initialised using ``KRN_initSemaphore``. This must be done before the semaphore is used.

You can *set* a semaphore by calling ``KRN_setSemaphore`` which adds a positive integer to the semaphore's value.

You can *test* a semaphore by calling ``KRN_testSemaphore`` which tests whether the semaphore's value is greater than or equal to the test value. If the test succeeds, then the semaphore's value is reduced by the test value. If the test fails, then the semaphore's value is unaffected. You can arrange for ``KRN_testSempahore`` to block (which implies scheduling another task) until the test succeeds:

*    The wait period may be infinite, in which case the test will only return when it succeeds, but could wait forever.
*    The wait period may be zero in which case ``KRN_testSemaphore`` always returns immediately. The test may or may not succeed.
*    An intermediate wait period activates the time-out feature. ``KRN_testSemaphore`` will wait until the test succeeds or the time-out expires, whichever happens first.

Suitable combinations of initial value and test semantics allow semaphores to used for many purposes.

Resource Locks
~~~~~~~~~~~~~~

Resource locks are used by tasks to mediate voluntary exclusive access to shared objects. A resource lock object ``KRN_LOCK_T`` is associated with the resource requiring protection and initialised using ``KRN_initLock``.

When a task requires access to the resource it calls ``KRN_lock``. If no other task has locked the resource, the function returns success. Otherwise it may block until the resource is available. When the task has finished using the resource, it releases it for other tasks by calling ``KRN_unlock``.

As with semaphores, you may specify a zero, intermediate or infinite time-out period on a ``KRN_lock`` call.

You must take care to match ``KRN_lock``/``KRN_unlock`` calls in pairs. Attempts to "nest" locks on the same resource will fail.

Resource locks are implemented as semaphores with a particular combination of initial value, test value and interpretation.

Mailboxes
~~~~~~~~~

Mailboxes combine inter-task synchronisation with message passing. Mailboxes must be initialised before use with ``KRN_initMbox``.

Any list-able object (which includes queue-able, tree-able, and pool-able objects) may be placed in a mailbox by calling ``KRN_putMbox``.

Objects are retrieved from mailboxes, in the order in which they were inserted, by calling ``KRN_getMbox``. Like the other synchronisation objects, ``KRN_getMbox`` takes a time-out argument which may cause it to block until an object is available.

Mailboxes have no capacity limits since the objects written to them are provided by the user.

Take care to ensure that objects you write to mailboxes really are list-able. The system doesn't verify this and failures will occur if you don't follow this rule.

Event Flags
~~~~~~~~~~~

Event flags are single bit semaphores: their values are restricted to 0 or 1.

They are managed in clusters of 32 flags which are initialised by calling ``KRN_initFlags``.

Set and test operations can operate on sets of flags within the cluster, with the set being defined by a *flag mask* argument. Several flags may be set, cleared or inverted atomically, using ``KRN_setFlags``, ``KRN_clearFlags``, and ``KRN_toggleFlags``.

``KRN_testFlags`` allows a task to wait for any or all of the flags in a specified set and provides an option to clear the flags or not after a successful test.

As with semaphores, you may specify a zero, intermediate or infinite time-out period on a ``KRN_testFlags`` call.

Synchronisation Barriers
~~~~~~~~~~~~~~~~~~~~~~~~

Synchronisation barriers provide a mechanism to ensure multiple threads have completed their work before proceeding. This helps to reduce the risk of race conditions.

Synchronisation barriers must be initialised with ``KRN_initSync`` before use by any thread. The number of threads cooperating must be specified at the time of initialisation.

Each thread that calls ``KRN_sync`` on the same synchronisation barrier will block until the number of threads specified at initialisation have called. All threads blocked on the same object will then unblock.

Pools
~~~~~

Pools are used for dynamic memory management. In addition, they provide synchronisation by letting a task block until a memory buffer is released by another task. Pools are implemented using semaphores and are described further in `Memory Pools`_.

Work Queues
~~~~~~~~~~~

A work queue is a queue of job descriptions, optionally coupled to a number of tasks that service it. This may be specified during initialisation with ``KRN_initWQ``. They are useful for implementing the top/bottom half ISR model, interacting with middleware that is only accessible from a single task, or for scheduling low priority jobs (such as checking for firmware updates) with minimal impact on real time processes.

A job is similar to a task: it has a name, a function pointer, and an item of private data. However, it describes a single unit of work, rather than an ongoing process. Instead of being executed in a new context, it is stored on a work queue with ``KRN_queueWQ``, for later execution.

When a work queue is dispatched, a job description is taken from the work queue, and invoked synchronously in the current task. Control will not return to the current task until the job has completed.

When the work queue is created, a number of tasks may be associated with it, with their own stacks. When there is no work available, the tasks will block. When jobs are available, each associated task will try to run one job from the queue. This means multiple jobs may be running simultaneously, so care must be taking to avoid the usual multitasking hazards. Alternatively, the user may manually dispatch a number of jobs from a work queue from an existing task using ``KRN_dispatchWQ``.

If multiple tasks are dispatching jobs from the same work queue, then there is no guarantee that jobs will execute to completion in sequence. If this behaviour is desired, care should be taken to ensure a work queue is only ever dispatched by one task.

Deadlock avoidance
~~~~~~~~~~~~~~~~~~

The system contains no facilities for deadlock avoidance, detection, or resolution.

Deadlocks must be avoided by design.

Memory Management
=================

The system does not do any memory management directly, and makes no use of any heap available.However, MEOS makes extensive use of the "double linked queues" (``DQ_``) and the "lists" (``LST_``) modules, which it extends by providing functions to manage buffer "pools". Pools also support the "trees" module, although trees are not used internally by MEOS.

Lists
~~~~~

The lists module provides functions for managing simple linked lists of objects. To be included in a list, an object must be "list-able". That is, it must be a ``struct`` with the special item ``LST_LINK`` as its first field. A list-able object looks like:

.. code:: c

	struct {
		LST_LINK;
		...
	} object;

List-able objects can be manipulated by the ``LST_`` module functions.

Double linked queues
~~~~~~~~~~~~~~~~~~~~

The double linked queues module provides functions for managing doubly linked queues of objects. Double linked queues allow a more comprehensive range of operations at the expense of a little more storage in each object. To be included in a queue, an object must be "queue-able". That is, it must be a ``struct`` with the special item ``DQ_LINK`` as its first field. A queue-able object looks like:

.. code:: c

	struct {
		DQ_LINK;
		...
	} object;

Queue-able objects can be manipulated by the ``DQ_`` module functions.

Queue-able objects are also list-able, so they may also be placed in lists managed by the ``LST_`` module functions.

Trees
~~~~~

The trees module provides functions for managing trees of objects. Trees allow more complex organisations than the simple linear lists provided by the lists and queues modules. To be included in a tree, an object must be "tree-able". That is, it must be a ``struct`` with the special item ``TRE_LINK`` as its first field. A tree-able object looks like:

.. code:: c

	struct {
		TRE_LINK;
		...
	} object;

Tree-able objects can be manipulated by the ``TRE_`` module functions.

Tree-able objects are also queue-able and list-able, so they may also be placed in queues or lists managed by the ``DQ_`` and ``LST_`` module functions.

Rings
~~~~~

The rings module provides functions for managing fixed sized FIFO byte buffers. This simplifies adapting data between differing buffer sizes.

The RING_T type manages access to a single provided buffer, which may be manipulated using the ``RING_`` module functions. Data may be added and removed either byte-wise or buffer wise, with optional blocking and timeouts.

Memory Pools
~~~~~~~~~~~~

Although the kernel does not provide dynamic memory allocation, it provides "pool" functions to allow you to construct your own.

A pool is collection of *identical* data objects. Objects may be allocated from a pool and returned to the pool. To be in included in a pool, an object must be "pool-able". That is, it must be a ``struct`` with the special item ``KRN_POOLLINK`` as its first field. A pool-able object looks like:

.. code:: c

	struct {
		KRN_POOLLINK;
		...
	} object;

You create a pool with ``KRN_initPool``. You may allocate an object from the pool by using ``KRN_takePool``. If the pool is empty, depending on its time-out/wait argument, ``KRN_takePool`` may block until another task returns an object to the pool by calling ``KRN_returnPool``.

An interesting feature of pool-able items is that they "know" their owner pool. ``KRN_returnPool`` will always return an item to the same pool from which it was allocated. This supports systems in which buffers are allocated and passed from task to task or processor to processor before being returned. Tasks can dispose of data objects without needing to know where those objects came from.

Pool-able objects are tree-able, queue-able and list-able, you do not need to include ``LST_LINK``, ``DQ_LINK``, or ``TRE_LINK``. Once taken from the pool they can be managed with ``LST_,`` ``DQ_`` and ``TRE_`` functions before being returned. Most significantly, any object taken from a pool may be passed from one task to another via a mailbox.

Interrupts
==========

Descriptors
~~~~~~~~~~~

Interrupt descriptors are used to route external interrupts to internal core IRQs. They are made of two parts, a generic part, specifying the core interrupt and the ISR, and a system specific part, providing information on how the external interrupt should be configured. A descriptor may not be reused or deallocated while in use by MEOS.

Each descriptor in use  must specify a unique internal interrupt number/external interrupt number pair. On systems without external interrupts, each internal interrupt number must be unique. MEOS does not provide hardware interrupt multiplexing.

All interrupt descriptors support the following public members:

intNum
	The internal interrupt number to route the interrupt to.
isrFunc
	The interrupt service routine to invoke when this interrupt is received.
priv
	Private user data that can be retrieved in the ISR. Not available for interrupts involved with QIO.

Depending on your platform, the following common public members may be supported:

extNum
	The external interrupt number, as seen by the interrupt controller.
trigger
	The trigger mode for the interrupt.
polarity
	The polarity required to trigger the interrupt.

Please see `Platform specific notes`_ for further details of the system specific parts for your platform.

Routing
~~~~~~~

To route an interrupt, fill in a descriptor, and pass it to `IRQ_route`_:

.. code:: c

	/* Global definition */
	IRQ_DESC_T intDesc = {
		.intNum = 2,
		.isrFunc = intFunc
	};
	...
	IRQ_route(&intDesc);

To unroute an interrupt, set the isrFunc member to NULL, and pass it to ``IRQ_route``:

.. code:: c

	intDesc.isrFunc = NULL;
	IRQ_route(&intDesc);

You may now recycle ``intDesc``.


Handling and acknowledging
~~~~~~~~~~~~~~~~~~~~~~~~~~

An interrupt service routine is passed the internal interrupt number that caused it. This may be passed to ``IRQ_cause`` to return the descriptor that caused the interrupt. This may then be passed to ``IRQ_ack`` to acknowledge it, which returns the descriptor for later use. This prevents the interrupt from retriggering:

.. code:: c

	void intFunc(int32_t intNum)
	{
		IRQ_DESC_T *desc = IRQ_ack(IRQ_cause(intNum));
	}

If there is only one descriptor that may cause an interrupt, this can be shortened to:

.. code:: c

	void intFunc(int32_t intNum)
	{
		(void)intNum;
		IRQ_ack(&intDesc);
	}

Communication between ISRs and the rest of the system
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In some simple cases, it may be that an ISR, once installed, can run independently of the rest of the system, autonomously handling some activity. More usually, an ISR will need to activate other system tasks in response to the events it handles. This will normally be done using mechanisms such as semaphores and mailboxes, perhaps in conjunction with global data buffers.

It is important to realise that most kernel (``KRN_``) and QIO (``QIO_``) functions must not be used inside ISRs. These functions are designed to be run in the context of an active task with interrupts enabled.

The exceptions are the main inter-task communication operations, which are permitted inside ISRs but are subject to some restrictions:

*    An ISR may activate a task by setting a semaphore, setting event flags, sending a message via a mailbox or returning an item to a pool.
*    An ISR may test a synchronisation primitive (semaphore, event flag, mailbox or pool), provided it does so in such a way that the test cannot block. These functions will ignore any ``timeout`` argument, treating it as zero. The code must be written so as to handle the possibility of failure. For example, an ISR may allocate a data buffer from a pool, but must handle the case where no buffer is immediately available.
*    Any synchronisation object accessed in an ISR must be locally defined (not imported or exported). It is not possible for an ISR to directly activate a task on another processor using MEOS.
*    If you use list (``LST_``), queue (``DQ_``) or tree (``TRE_``) manipulation primitives in an ISR, remember that these operations are not protected for re-entrant operation. In non-interrupt code, you will have to explicitly protect any data objects shared with the ISR.

As a rule, you should assume that none of the ``KRN_`` or ``QIO_`` functions may be used inside an ISR unless explicitly stated in the API documentation. Please see the `API Reference`_.

Use of global data buffers must be protected with the appropriate kind of critical region.

Software interrupts
~~~~~~~~~~~~~~~~~~~

``IRQ_soft`` will fill a descriptor so as to register and synthesize a software interrupt:

.. code:: c

	/* Global  definition */
	IRQ_DESC_T softDesc;
	...
	IRQ_soft(1, &softDesc);
	softDesc.isrFunc = intFunc;
	...
	IRQ_route(&softDesc);

Up to ``IRQ_SOFTINTS`` software interrupts are available, please see `Platform specific notes`_ for the exact number for your platform. note that as with hardware interrupts, software interrupts must be acknowledged.

To generate a software interrupt, call `IRQ_synthesize`_ with the descriptor:

.. code:: c

	IRQ_synthesize(&softDesc);

Inter-processor interrupts
~~~~~~~~~~~~~~~~~~~~~~~~~~

``IRQ_ipi`` will fill a descriptor to allow you to register and synthesize an interprocessor interrupt. There is support for 1 interprocessor interrupt per processor. The exact mapping of this interrupt will be platform specific. Please see `Platform specific notes`_ for details.

To register the IPI interrupt for your self:

.. code:: c

	/* Global  definition */
	IRQ_DESC_T inDesc;
	...
	IRQ_ipi(IRQ_SELF, &inDesc);
	...
	IRQ_route(&inDesc);

Note that you may not register IPI interrupts for other threads.

To synthesize an IPI interrupt for another processor:

.. code:: c

	IRQ_DESC_T outDesc;
	...
	IRQ_ipi(2, &outDesc);
	IRQ_synthesize(&outDesc);

``KRN_proc`` may be used to find out the index of the current processor, and ``KRN_procs`` provides the number of processors visible to MEOS.

Protecting critical regions
===========================

A critical region is a section of code which operates on some resource (data, CPU register, device register etc) and which is vulnerable to disruption by another task operating on the same resource. Critical regions come in a variety of forms but you should particularly look out for read/modify/write operations. In a pre-emptive environment, something as simple as...

.. code:: c

	variable += 1;

...is a critical region if ``variable`` is used by more than one task.

All the kernel's internal critical regions include their own protection so you don't need to worry about protection around most ``KRN_`` calls. Exceptions are KRN_testSemaphoreProtected and KRN_testFlagsProtected which are described in `Protected Tests`_. Critical regions in your own code must be protected by ensuring that only one task accesses the resource at a time.

The type of critical region used depends on how data is shared. For buffers shared with other tasks, semaphore and resource locks should be used. Buffers shared with ISRs should employ interrupt protection. When buffers are shared between processors, the volatile keyword should be used in their declaration, and inter-processor protection should be used if necessary.

Semaphores and Resource Locks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The standard way to protect a critical region is to associate a resource lock with the region. At the start of the region you "seize" the lock (``KRN_lock``) and at the end of the region you release the lock (``KRN_unlock``). Resource locks and semaphores are closely related. If you prefer you can protect critical regions by using semaphores directly.

If you need to protect critical regions across multiple processors, you can use the import/export system (See `Hardware Multi-processing`_) to set up shared semaphores or resource locks which operate across processors.

There are two things to remember when using resource locks or semaphores to protect critical regions:

*    When you release the lock, you will usually cause a scheduler event.
*    Efficiency - the lock/unlock sequence may involve far more code than the critical region you are trying to protect. You may prefer simply to disable interrupts around the critical region (See `Interrupt protection`_).

Interrupt protection
~~~~~~~~~~~~~~~~~~~~

You can protect critical regions by disabling normal priority interrupts. In some ways this is a "sledgehammer" approach, because *all* other normal activity is locked out, not just the potentially conflicting tasks. However, since interrupt disable/enable operations are implemented fairly efficiently, this is the preferred approach for *short* critical regions. Note that high priority interrupts will continue to occur.

You can protect critical regions by using ``IRQ_raiseIPL`` and ``IRQ_restoreIPL``. IPL stands for Interrupt Priority Level, which suggests a multi-level interrupt system, but the kernel only provides a simple "on/off" switch.

A typical protected region looks like:

.. code:: c

	IRQ_IPL_T oldIPL;
	...
	oldIPL = IRQ_raiseIPL(); /* protect */
	...
 	<critical code>
	...
	IRQ_restoreIPL(oldIPL);  /* unprotect */

Note that we always restore the previous interrupt state. We do not assume that we started with interrupts enabled. This means that everything still works if critical regions are nested.

Protected Memory
~~~~~~~~~~~~~~~~

The system offers no facilities for protected memory spaces. All memory is accessible by all tasks. Design with care!

Timing
======

The key elements of the kernel's time handling system are a timer processing task and a timer interrupt service routine. When you start the timer task with ``KRN_startTimerTask`` (See `Start-up`_) you specify a scheduler tick period in microseconds. The system programs the platform specific interrupt timer to generate a repeating interrupt at your chosen tick period.

The timer ISR is invoked in response to the tick interrupt and, if necessary, schedules the timer task to deal with any time related events that need attention. This same mechanism is used both for the kernel's own time-out processing and for any of your own special timed events.

Timers
~~~~~~

The simplest use of the timing system is to queue a timer using ``KRN_setTimer``.

``KRN_setTimer`` binds a function, a parameter for that function and a delay time (in scheduler ticks) to a timer object. It then arranges that the function will be called (with its parameter) after the specified period has expired. The function is executed once only by, and in the context of, the timer processing task. ``KRN_setSoftTimer`` is similar to ``KRN_setTimer``, but has a tolerance parameter. It will attempt to schedule the timer so that it occurs at the same time as any pre-existing timer, to reduce scheduling overhead.

A timer can be cancelled by passing a pointer to the timer object to ``KRN_cancelTimer``. It doesn't hurt to cancel a timer after it has expired, although the timer's action will already have been executed.

You must not reuse the timer object until the timer has "fired" or been cancelled.

Time-outs
~~~~~~~~~

We have already discussed time-outs in `Synchronisation and Communication`_.

Time-outs are also available in the QIO system (See `QIO`_).

Finite time-outs are implemented using the timer mechanism discussed in `Timers`_. Immediate time-outs and infinite time-outs do not require the timer mechanism. The timer functions for time-outs are special functions, built into the kernel, which know how to undo the operation which is in progress when the timer "fires".

Hibernation
~~~~~~~~~~~

A task may hibernate using ``KRN_hibernate``. A hibernating task will not be scheduled until either it is woken by another task (``KRN_wake``, or ``KRN_wakeAll``) or its time-out expires, whichever occurs first. You can specify a zero (pointless), timed or infinite wait, similar to the time-outs on the synchronisation operations discussed in `Synchronisation and Communication`_.

Time Slicing
~~~~~~~~~~~~

Normally, scheduling events occur in response to explicit scheduler interrupts, typically resulting from semaphore or event flag operations, device interrupts, or timer expiry. If the system is fairly quiet, a compute bound task could run on the CPU for an extended period, at the expense of other tasks with the same priority. There is nothing wrong with this, but you may wish to introduce time sharing among tasks with equal priority.

You can set the time slice period (or disable time slicing) with ``KRN_setTimeSlice``. The system will arrange that the maximum period without a scheduler event is that which you specify. If other scheduler events occur before the time slice period expires, then the period is restarted. Consequently, in an active system, the time slicing may not introduce any additional scheduling events.

Time slicing is initially disabled when the kernel is started.

FPU contexts
============

MEOS will ensure that FPU contexts are appropriately saved and restored for tasks that use the FPU unit. The exact policy is dependent on the specific platform, please see `Platform specific notes`_.

Advanced Features
-----------------

Protected Tests
===============

Two functions in the kernel API are rarely used, but are provided to deal with special situations.

KRN_testSemaphoreProtected
~~~~~~~~~~~~~~~~~~~~~~~~~~

Occasionally you may need to combine a semaphore with your own code to create an atomic operation. For example, you might combine a semaphore with a buffer to make a FIFO. As a first attempt, to implement a blocking read from the FIFO, you may write code like:

.. code:: c

	char FIFO_get()
	{
    		char c;
    		IRQ_IPL_T oldipl;

	    	KRN_testSemaphore(&sem, 1, KRN_INFWAIT); /* wait for data       */
    		oldipl = IRQ_raiseIPL();                 /* protect data buffer */
    		c = getData();                           /* take data           */
    		IRQ_restoreIPL(oldipl);

    		return c;
	}

The problem with this is that the semaphore and the buffer are temporarily inconsistent. If the executing task were killed between the semaphore grant and the data removal, the FIFO would be left permanently inconsistent.

As an improvement, you might try making the transaction atomic by disabling interrupts:

.. code:: c

	char FIFO_get()
	{
		char c;
		IRQ_IPL_T oldipl;

		oldipl = IRQ_raiseIPL();                 /* protect transaction */
		KRN_testSemaphore(&sem, 1, KRN_INFWAIT); /* wait for data       */
		c = getData();                           /* take data           */
		IRQ_restoreIPL(oldipl);
		return c;
	}

This doesn't work either. If the buffer is empty the caller will block on the semaphore. Unfortunately, because interrupts are disabled, no other activity will ever be scheduled so the system will just stop.

The solution is to disable interrupts and then use ``KRN_testSemaphoreProtected``:

.. code:: c

	char FIFO_get()
	{
		char c;
		IRQ_IPL_T oldipl;

		oldipl = IRQ_raiseIPL();
		/* wait for data, temporarily enabling interrupts if necessary */
		KRN_testSemaphoreProtected(&sem, 1, &oldipl);
		/* take data */
		c = getData();
		IRQ_restoreIPL(oldipl);
		return c;
	}

The key feature of ``KRN_testSemaphoreProtected`` is that, if blocking is necessary, interrupts are re-enabled until the test succeeds. Interrupts are then disabled once more. Furthermore, the function guarantees that test success and the subsequent interrupt disable are atomic. You can be sure that no other task will be scheduled once you have been "granted" the semaphore until you explicitly re-enable interrupts.

``KRN_testSemaphoreProtected`` may alter your saved IPL value (``oldipl`` above). This can happen if another task initialises a device while you are waiting for the semaphore, so it is important to follow the formalism of the example.

When invoked from a protected region (interrupts disabled) ``KRN_testSemaphoreProtected`` will succeed immediately or fail. If invoked upon an imported semaphore with interrupts disabled then it will always fail.

Finally, note that ``KRN_testSemaphoreProtected`` will always block until the test succeeds. It has no polling or time-out options, excluding the above restrictions.

KRN_testFlagsProtected
~~~~~~~~~~~~~~~~~~~~~~

``KRN_testFlagsProtected`` addresses the same problem as ``KRN_testSemaphoreProtected`` for event flag cluster instead of semaphores. It is used in a similar way.

Low level timers
================

``TMR_resetCycleCount`` will reset the system cycle counter if available. This resets the physical counter and computes the overhead involved in fetching a cycle count, in an attempt to minimise jitter. ``TMR_startCycleCount`` will start a count immediately, and ``TMR_stopCycleCount`` will stop the count, compute the duration, attempting to compensate for overflow and overheads, and return the number of cycles. To minimise error, this should be done soon after a call to ``TMR_resetCycleCount``.

``TMR_getPerfCount`` can be used to fetch the a platform specific performance counter, useful for testing. Please see `Platform specific notes`_

Memory barriers
===============

``KRN_barrier`` can be used to perform a simple CPU level memory barrier. Flags allow the type of barrier (read/write/completion) to be specified. MEOS will ensure the barrier is at least as strong as the type specified.

Address translation
===================

If MEOS is configured with memory management support, ``MEM_p2v`` and ``MEM_v2p`` can be used to translate between memory mappings. ``MEM_map`` can be used to configure a mapping so that a particular piece of physical memory appears at a particular address.

With no memory management support, these functions are non-ops, and will always succeed.

Hardware Multi-processing
-------------------------

You can run MEOS on as many or as few of a systems processors as you wish.

You can set different timer tick periods on different processors.

MEOS synchronisation structures (semaphores, mailboxes etc) must be treated as local to a single processor. They are not designed to synchronise tasks executing on different processor. If you need tasks on different processors to interact with each other, then you must use the import and export mechanisms described in `Communicating with other MEOS processors (Import/Export)`_.

The two additional considerations in a multi-processor system are:

*    Each interrupting I/O device must be handled exclusively by a driver on a single processor. Different devices may be handled on different processors. This follows from the requirement, which is implicit some hardware architectures, that any given core interrupt be serviced on one processor only.
*    Some extra care is necessary to ensure that the processors start up in a proper sequence. No inter-processor operations should occur before the participating processors have all called ``KRN_reset`` and ``KRN_startOS`` and have initialised any inter-processor synchronisation objects.

Querying other processors
=========================

The ``KRN_procs`` function will return the number of processors visible to MEOS. These processors may be running other operating systems, so care should be taken prior to interacting with them. ``KRN_proc`` will return the index of the current processor. This may be used with ``IRQ_ipi``, or the import/export system, described below.

Communicating with other MEOS processors (Import/Export)
========================================================

A processor may *export* a synchronisation object (such as a semaphore, event flag cluster, mailbox, lock or pool) by calling ``KRN_export``. Part of the export process is to define a public *Export Id* for the object which allows it to be referenced by other processors.

Other processors may then import the object using ``KRN_import``. Once the import/export relationships have been set up, each processor accesses a local object, but the set of objects *behave* collectively as if they were a single shared object. This behaviour is achieved transparently through a message exchange protocol between the processors.

Prior to calling ``KRN_import`` or ``KRN_export``, you must initialise the import/export system. This is a two step process: first you must call ``IPM_init`` to initialise the inter-processor conduit, then you must call ``KRN_installImpExp`` to manages the objects. Normally, you would do this immediately following the call to ``KRN_reset``. If you don't need the import/export feature, then don't use ``IPM_init`` or call ``KRN_installImpExp`` - your program will be smaller since the extra code to support the message exchange protocol won't be included.

The import/export mechanism is built on top of a low level messaging protocol, which in turn passes messages between processors using Linux-compatible Vrings. In principle, any software may interact with a MEOS processor by using the MEOS inter-processor messaging protocol. However, in many cases this will be inappropriate. The semantics of the MEOS messaging system may not be well matched to other environments.

Using the IMPORT/EXPORT Mechanism (MEOS - MEOS)
===============================================

The Import/Export mechanism is the preferred choice when MEOS processors must communicate with each other.

Interprocessor messaging system
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The interprocessor messaging system provides a low level conduit for passing messages between processors. You must declare an area of memory to be shared between all intercommunicating processors. This can either be done by convention, or reserved in the linker script. In the code below, this has been assigned the symbol ``shared``.

You must then tell MEOS where this buffer is by calling ``IPM_init``, after ``KRN_reset``. ``IPM_init`` also requires some local storage for storing control data.

.. code:: c

	/* Global definitions */
	extern uint8_t *shared;
	IPM_T ipm;

	...

	/* Initialisation code */
	IPM_init(&ipm, shared, page_size);

This will ensure the platform specific structure is correctly generated at ``shared``.

Reset the Import/Export System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before you attempt to import or export an object, you must *install* the import/export system on top of the interprocessor messaging system by calling ``KRN_installImpExp``. This function installs data tables, which you provide, for managing the import/export lists and the extra code which MEOS uses internally for message handling.

``KRN_installImpExp`` is normally called immediately after ``IPM_init``.

The requirement for an additional start-up function for the import/export system is a minor burden for the system programmer, but this approach allows us to avoid linking the extra code and data for import/export processing into systems which do not require it.

Establish Import/Export Relationships
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The correct sequence of events for all exportable objects is

1.  Server initialises the item
2.  Server exports the item with ``KRN_export`` using the published item Id (the server export Id).
3.  Each importing client initialises an item of the same type.
4.  Each importing client imports the item with ``KRN_import``, using the published server export Id.

Messages are exchanged during the import/export process to ensure that both sides of the link are synchronised prior to usage.

Once import/export relationships have been established they are intended to be permanent. You should not attempt to re-initialise the objects, re-use Id values or break the import/export relationship. The intention is that import/export correspondences are established at system start-up and then treated as a fixed aspect of the application.

The system does not check that a client has used an object of the correct type when establishing an import/export link. However, if the types are not the same (for example if you import a mailbox when the server exported a semaphore with the same Id, all client requests will fail (They will be answered by a ``NACK`` response).

Ids and Storage
~~~~~~~~~~~~~~~

A certain amount of working storage is required to manage each imported item. To allow you to keep this to a minimum, this storage is kept separate from the objects themselves in an *import table* and an *export table*. Import and Export Ids are used to index into the corresponding table.

The tables themselves must be declared explicitly and passed to the system as arguments to ``KRN_installImpExp``. You can keep the tables as small as possible by allocating Id's sequentially from 1 upwards (zero not allowed, maximum allowed Id is 255). The number of elements required in each table is described in the reference definition of ``KRN_installImpExp``.

Limitations and requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There several important requirements in the use of the import/export system which you must be careful to observe. In the interests of a compact memory footprint, MEOS does not provide much useful diagnostic information when things go wrong. If you fail to observe the following points carefully, your system will probably fail in unpredictable and mysterious ways.

Importing Pools
+++++++++++++++

When you initialise a pool which is to be imported, you don't need to provided any storage area for pool data, since this is located on the server, or exporting, processor. Just initialise the imported pool as in the following example:

.. code:: c

	KRN_initPool(&pool, NULL, 0, 0);

KRN_returnPool delivers an item back to its owner pool. If you return an item which belongs to a pool on another processor, it is essential that you import the owner pool to the processor where you do the return even if, subsequently, you don't ever access the pool directly.

To some extent this negates the benefits of having ``KRN_returnPool`` decide for itself where to deliver the returned item. This is a compromise we accept in the interests of simplicity and code size.

Address Spaces
++++++++++++++

Mailbox and pool functions provide pointers to data buffers. This is only meaningful across processors if both share a common address space. You must ensure that the same pointer value points to the same memory location on each processor.

If you don't have common address spaces, then you cannot use the MEOS mailbox and pool functions across processors.

The semaphore, lock and event flag functions do not rely on common address spaces and will work across processors, regardless of memory configuration.

Cache refresh after mailbox and pool operations
+++++++++++++++++++++++++++++++++++++++++++++++

MEOS supports both coherent and non-coherent systems. While coherent systems should just work, extra care must be taken when communicating between non-coherent processors.

When you deliver a buffer from one processor to another via a mailbox, it is possible that the receiving processor will read buffer data incorrectly since it may have invalid cached data for the buffer memory.

In such circumstances, you must ensure that the cache is valid before trusting the data. One way to do this is by using ``KRN_refreshCache`` or ``KRN_flushCache``. These functions are not necessarily the most efficient way of dealing with cache coherency issues, but they should at least ensure that the data you see is correct.

In general, MEOS cannot look after cache coherency for you, since it has no idea of the sizes of objects being passed via mailboxes.

MEOS does, however, look after the coherency of pool-able objects' linkage fields. This is the minimum required to ensure that the pool "owner" fields are correct and that ``KRN_returnPool`` will work properly (always assuming the pool was imported if necessary).

You can avoid these problems by locating shared data objects in un-cached memory, although this may result in either performance degradation or in using relatively scarce core memory.

Use in ISRs
+++++++++++

Access to synchronisation objects (semaphore, lock, event flag cluster, mailbox, pool) in ISRs *must* be limited to local or exported objects only. You *must not* access an imported object in an ISR.

If you really do need to synchronise activity on one processor with an ISR on another, then make sure the object is owned and exported by the processor where the ISR executes.

Performance
+++++++++++

Inter-processor synchronisation operations are considerably slower than their local equivalents since the protocol messages must be formed, transmitted and validated. A task waiting on an imported semaphore or event flag will be reactivated every time the semaphore or flag "value" is increased.

You should design your system to avoid high inter-processor event rates.

If possible, you should avoid testing for imported semaphore values greater than one, or for ALL of imported event flag cluster flag sets.

Debug features and instrumentation
----------------------------------

Debugging a multi-tasking system is, inevitably, more complex than debugging a simple linear-execution system. MEOS provides many features to assist with this difficult problem:

* Codescape task level debug
* Diagnostics message and checks
* Post-mortem diagnostics
* Wrapper based diagnostics
* Software and hardware tracing
* Paranoia
* Kernel tracing
* OS profiling
* Per-task performance counters

Codescape task level debug
==========================

The Codescape interactive debugger is aware of MEOS task structures. You must explicitly enable the facility in Codescape.

Codescape will let you set up separate stack trace and local watch windows for each MEOS task. At present break points and program stepping are application (not task) oriented, but Codescape is under continuous development so more features may become available in future. Refer to the Codescape documentation for the latest information.

Diagnostic messages and checks
==============================

The following functions are provided for diagnostic messages and checks:

DBG_logF
	This is a portable version of printf. Note that it may be slow.
DBG_logT
	This is a variant of DBG_logF that uses the trace log as storage. See `Trace messages`_.
DBG_info
	This provides an informational message, which may be intercepted.
DBG_insist
	This checks a condition, reporting a interceptable warning message if the check fails.
DBG_assert
	This checks a condition, and displays an error message and stops if the check fails. This behaviour may be intercepted.

MEOS uses some of these facilities to provide internal consistency checks. You should not see them trigger under normal circumstances. Many of these messages are useful for both kernel and application developers. Ignoring an assert will likely lead to instability and a crash. A warning may be ignored without causing a crash, but the system will still not behave as desired. If you do see an assert "fire", check for obvious mistakes, such as inconsistent array sizes or un-initialised variables, before seeking help. Warnings typically indicate potential sources of bugs or poor performance.

Post-mortem diagnostics
=======================

Backtrace
~~~~~~~~~

Optionally, MEOS can provide a register dump and backtrace when it handles an exception. This greatly increases code size, but is useful for finding tricky bugs.

Trace dump
~~~~~~~~~~

MEOS can also be configured to dump its trace buffer in a user readable format on an exception. Again, this greatly increases code size.

Wrapper based diagnostics
=========================

MEOS can be configured to wrap its API to provide advanced diagnostics and tracing facilities. This comes at the cost of increased code size and reduced performance.

Pre/post conditions
~~~~~~~~~~~~~~~~~~~

A subset of MEOS functions can validate their arguments on entry and their return value on exit. This utilises DBG_assert calls in the wrapper mechanism.

Excluding symbols
~~~~~~~~~~~~~~~~~

A set of API functions may be specified that are excluded from wrapping. If the wrapper causes performance to be too slow, this allows critical functions to be exempted.

Only specific objects
~~~~~~~~~~~~~~~~~~~~~

If you are getting too much trace data, you can specify that only API calls specifying particular instances are traced.

A global per-object trace buffer must be declared first using ``DBG_PER_OBJECT_POOL(nObjects);``, where ``nObjects`` is the maximum number of traceable objects. Objects may then be traced by calling ``DBG_tronObject``, and tracing stopped using ``DBG_troffObject``.

Only specific modules
~~~~~~~~~~~~~~~~~~~~~

Specific modules may be excluded from wrapping by setting the appropriate configuration options.

Software and hardware tracing
=============================

The tracing system allows events to be recorded either to a platform specific hardware trace mechanism, or to a generic software trace buffer. The software trace buffer is initialised using ``KRN_reset``.

Trace messages
~~~~~~~~~~~~~~

Debug messages may be stored to the trace log using ``DBG_logT``. Note this must be explicitly enabled in the MEOS configuration. This mechanism will rapidly use space in the trace buffer, but is faster than ``DBG_logF`` and has less effect on the timing properties of the system.

Tracing API entry/exit with wrappers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The wrapper may be configured to emit trace data. This includes the function entry point rounded to the nearest word, the arguments, and the return value.

ISR/scheduler entry/exit
~~~~~~~~~~~~~~~~~~~~~~~~

ISR and scheduler entries and exits may be traced. This will have a heavy penalty on interrupt latency.

IPL manipulation
~~~~~~~~~~~~~~~~

IPL manipulation may also be traced. The levels that are being transitioned between are stored in the trace log.

Context switches
~~~~~~~~~~~~~~~~

Context switches may be traced - the tasks being switched are written to the trace log. Additionally, a limited backtrace may be written to the trace log to help identify exactly what was happening at the switching point.

Extra
~~~~~

Extra detail may be provided in the trace data. This typically includes the address within the calling function that caused the event, and a platform specific datum.

Implementation
~~~~~~~~~~~~~~

For software trace, a trace buffer is an array of ``KRN_TRACE_T`` structures. Once provided to ``KRN_reset``, the kernel treats this as a circular buffer, using it to record system events. If you stop your system, you can access the trace buffer via three pointers:

KRN_tracePtr
	Points to the most recent complete entry in the trace buffer.

KRN_traceMin
	Points to the bottom (lowest address) item in the trace buffer.

KRN_traceMax
	Points to the top (highest address) item in the trace buffer.

KRN_traceSize
	Number of elements in the trace buffer.

The same format is used for hardware trace. The first word of an event will be emitted using the first user trace register, subsequent words will be emitted using the second user trace register. This eases synchronising with the event stream.

Each KRN_TRACE_T structure is either an *initial* event, or a *supplemental* event. An initial event contains:

*	An event type.
*	An absolute time stamp, as provided by ``TMR_getMonotonic``.
*	2 event specific data.

If an event needs to dump more data, it may be followed by a supplemental event:

* 	The event type. This encodes the number of data used.
* 	3 additional event specific data.

Events include:

Function entry
	A function is about to be entered. Event data can optionally encode function parameters. DBG_TRACE_CONTINUATION events may be emitted until all parameters have been encoded.
Function exit
	A function is about to be exited. Event data can optionally encode return value.
DBG_TRACE_ENTER_ISR
	An ISR is about to be entered. Datum 1 contains the interrupt number, datum 2 contains the PC interrupted.
DBG_TRACE_EXIT_ISR
	An ISR has returned. Datum 1 contains the interrupt number, datum 2 contains the PC interrupted.
DBG_TRACE_SCHED_ISR
	The scheduler is about to be entered, datum 1 contains the PC interrupted.
DBG_TRACE_CHAIN_UHI
	The UHI mechanism is about to be activated, datum 1 contains the PC interrupted.
DBG_TRACE_HOTWIRE_ISR
	A context is about to be activated. Datum 1 contains a pointer to KRN_TASK_T->savedContext for the new task, datum 2 contains the PC where execution will recommence.
DBG_TRACE_CTX_SW
	A context switch is about to occur. This event will always be followed by supplemental events. Datum 1 contains a KRN_REASON_T specifying the cause of the switch, Datum 2 contains the TCB being switched out, datum 3 contains the TCB being switched in. Datum 4 contains the PC that is being suspended, datum 5 contains the PC that is about to be resumed.
DBG_TRACE_CTX_BT
	A backtrace for the context about to be switched out in the preceding DBG_TRACE_CTX_SW event. Datum 1 contains the number of PCs traced, following data contains those PCs.
DBG_TRACE_RAISE
	The IPL is being raised. Datum 1 encodes the IPL prior to the raise. This may be followed by a DBG_TRACE_EXTRA2 supplemental event, where datum 1 is the PC where the raise occurs, and datum 2 is the platform specific datum.
DBG_TRACE_LOWER
	The IPL is being lowered. Datum 1 encodes the IPL prior to the lower, datum 2 the IPL afterwards.  This may be followed by a DBG_TRACE_EXTRA2 supplemental event, where datum 1 is the PC where the lower occurs, and datum 2 is the platform specific datum.
DBG_TRACE_LOG
	A string has been output to the trace log. Data for this event contains up to the first 8 characters. If the string is shorter than 8 characters, it will be NUL terminated. If followed by a DBG_TRACE_CONTINUATIONn event, the event will contain up to 12 more characters. If the string is shorter than 12 characters, it will again be NUL terminated.
DBG_TRACE_CONTINUATION1, DBG_TRACE_CONTINUATION2, DBG_TRACE_CONTINUATION3, DBG_TRACE_EXTRA1, DBG_TRACE_EXTRA2, DBG_TRACE_EXTRA3
	These events are meaningless without the context of the preceding event.

The table below shows how events will be encoded into words:

=== ===== ===================== ===================
MSB MSB-1 MSB-2..0              Meaning
=== ===== ===================== ===================
  0     0 0                     ISR entry
  0     0 Function pointer >> 2 Function entry
  0     1 Function pointer >> 2 Function exit
  1     X Enumeration value     DBG_TRACE event
=== ===== ===================== ===================

Function entry/exit events provide all but the bottom two bits of the entry point for the function. The debugger is expected to snap this to the nearest function. Given function call over heads, it should not be possible for different functions to alias. ISR entry (``DBG_TRACE_ENTER_ISR``) has the special value of zero in an attempt to minimise ISR entry latency.

Paranoia
========

The paranoia system is a heavy weight consistency checking system designed to detect memory corruption. By increasing the size of all kernel data structures, and very aggressively auditing their integrity on almost every kernel API entry/exit, memory corruption can be caught almost immediately after it has occurred. If MEOS detects corruption, it will halt. Combined with the trace facility above, this allows the code that caused the corruption to be easily identified.

This facility also uses GCC function instrumentation facilities to track it's management data, contributing to further performance loss. As such, it should only be activated for the duration of the bug being diagnosed.

Paranoia may not work as expected with systems that use dynamic memory allocation.

OS Profiling
============

MEOS implements an OS profiling interface to allow external profiling tools to collect performance statistics such as instruction issues, cache misses, etc. on a task by task basis.

To enable this, your application must declare a buffer to collect performance statistics and "install" this buffer with the kernel by calling ``KRN_installPerfStats()``. This must be done after calling ``KRN_reset()`` and before ``KRN_startOS()``.

.. code:: c

	#define NUM_STATS_SLOTS 24
	KRN_STATS_T perfStats[NUM_STATS_SLOTS];

	...

	int main(int argc, char **argv)
	{
	    KRN_reset(...);
	    ...
	    KRN_installPerfStats(perfStats, NUM_STATS_SLOTS);
	    ...
	    KRN_startOS(...);
	    ...

Since collecting performance statistics introduces additional overheads in the scheduler and in interrupt handlers, you should not normally install a performance statistics buffer in production builds.

MEOS will allocate a slot in the performance statistics array to each task, as it is created. When tasks are destroyed, the statistics slot is released for use by a new task.

The performance statistics array must be large enough to accommodate the maximum number of tasks in your system, plus the special categories listed in `Performance statistics categories`_. If you install a statistics array which is too small, your application will fail.

Profiling interface
~~~~~~~~~~~~~~~~~~~

Each slot in the performance statistics buffer is defined as follows

.. code:: c

	struct {
		/* Pointer to entity name                   */
		const char *name;
		/* Number of time entity has been scheduled */
		int32_t runCount;
		/* Elapsed time in monotonic ticks 			*/
		uint32_t monotonics;
		/* Platform specific performance counters */
		uint32_t perfCount[TMR_PERFCOUNTERS];
	}

MEOS programmers use the data type ``KRN_STATS_T`` to define these structures.

The ``name`` field points to a ``NUL``-terminated character string. Unused elements are identified by a ``NULL`` value in the ``name`` field.

Un-named (but active) elements have ``name`` pointing to an empty string, i.e., ``name`` points to a location containing ``NUL``.

The following global variables provide analysis tools with access to the data:

\_\_ThreadStatsArray
	Points to the first element in the user-provided statistics buffer array.

\_\_ThreadStatsArraySize
	The number of elements in the user-provided statistics buffer array.

\_\_ThreadStatsCtrl
	Controls/displays the current state of statistics collection.

Analysis tools control data collection by writing to ``__ThreadStatsCtrl`` as follows:

\_\_ThreadStatsCtrl = 0
	Stop Collection

\_\_ThreadStatsCtrl = 2
	Reset statistics and start data collection. The system acknowledges this request by changing the value to 1, which indicates that collection is active.

Performance statistics categories
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

MEOS collects statistics for each task in the system. The ``name`` field points to the task name provided by the user or to ``NULL``, if the task is unnamed.

In addition, MEOS reports data for the following special categories of activity:

Idle
	Time spent with no run-able task.

Timer Interrupts
	Activity attributable to the MEOS timer interrupt handler.

QIO Device Interrupts
	Activity attributable to the QIO system device interrupt handler (and the device driver functions it calls).

Scheduler interrupts
	Activity attributable to scheduler processing.

The interrupt processing statistics are collected only over the portable MEOS code involved. The low level platform specific entry/exit sequences are not explicitly attributed. This activity will be included in the counts for the task (or idle) that was interrupted, which is why the counts for the idle activity are non-zero.

Supported Packages
------------------

.. include:: packages.rst

Appendices
----------

.. include:: appendices.rst
