The QIO system provides a standardised framework for writing and using software drivers for interrupt-driven I/O devices. It allows systems to be constructed efficiently by handing data buffers from tasks to devices and back to (often different) tasks again, without significant copying overheads.

Drivers for new devices can be written very quickly, since such drivers need contain only the bare minimum of device specific code.

Finally, device drivers can be implemented with software tasks modelling hardware behaviour, allowing systems to be developed in the absence of the real hardware.

Using the QIO System
~~~~~~~~~~~~~~~~~~~~
The IO model supported by the QIO system is that of a queue of operations which are performed sequentially by a device. Operations are held on a pending queue until the device indicates that it is ready to accept a new operation, by raising its interrupt. In response to the interrupt, the QIO system:
*    Completes the active operation by delivering the results to their destination.
*    Takes the next operation from the pending queue and loads the device registers as required to initiate the operation.

Some devices are multi-rank. That is, one or more subsequent operations can be loaded into the device registers before the current operation completes. Such devices can automatically start their next operation immediately the active operation completes. Since this does not involve software, multi-rank devices provide a way for system designers to avoid demanding interrupt response/latency requirements. The QIO system manages multi-rank devices by maintaining an active queue of those operations which are loaded on the device. The length of the active queue will always be less than or equal to the device rank. The QIO system can support devices of arbitrary rank, although in practice, only device ranks of 1 or 2 are typical.

Devices
~~~~~~~

Devices are identified by device objects (``QIO_DEVICE_T``). Device objects must be initialised using ``QIO_init`` (once only) before use. Initialisation associates the appropriate software driver with the device object. Devices are usually initialised at system start-up - after calling ``KRN_startOS``, but before calling ``KRN_startTimerTask``. It's not essential to initialise devices here, you can delay device initialisation if you wish. However, the run-time behaviour of your system will be more predictable if all devices are initialised during the start-up sequence.

After initialisation, devices must be enabled using ``QIO_enable`` before use. Devices may then be disabled and re=enabled (``QIO_disable``, ``QIO_enable``) as often as required.

Remember to enable a device before queuing an operation on it. If you queue an operation on a disabled device, the operation will be not be performed, but delivered to its recipient as if it had been cancelled.

Power Management
++++++++++++++++

Some devices have the capability for a low=power idle mode of operation. We cater for four kinds of power control:

None
	The device has no facility for power management.

Automatic
	The device manages its power automatically. (As far as the QIO system is concerned, this is the same as no power management!)

Coarse
	The device is powered up when it is enabled by ``QIO_enable`` and powered down by ``QIO_disable``.

Fine
	The device is powered up whenever an active operation is loaded and powered down otherwise.

Whatever the device's power class, the user should call ``QIO_enable`` and ``QIO_disable`` as appropriate (often just an initial ``QIO_enable``). The appropriate power management will then be handled automatically by the QIO system, regardless of the device's characteristics.

I/O Control Blocks
++++++++++++++++++

In order to mange the operation queues, every I/O operation is defined by an *I/O Control Block* (IOCB) object, represented by the ``QIO_IOCB_T`` data type. To leave the user with maximum freedom to design his memory management system, IOCBs are obtained by the user and passed to the QIO system. IOCBs are *pool=able*, which means they can be managed in pools, lists, and double linked queues. The QIO system makes no demands on IOCB management except that it needs exclusive access to the IOCB from the moment an operation is first queued until the operation is complete and the results collected.

I/O parameters and results
++++++++++++++++++++++++++

I/O operations are normally specified and initiated by using calling ``QIO_qio``. The I/O operation continues in parallel with normal processing, with any results being delivered to a mailbox from where they can be retrieved using ``QIO_result``.

QIO_qio
^^^^^^^

This function takes the following arguments:

dev
	Pointer to the device object

iocb
	Pointer to an IOCB. The user must obtain this IOCB from somewhere. The QIO system will then use the object to manage the I/O operation. The ``iocb`` object must not be re=used until after the operation is complete.

iopars
	Pointer to a ``QIO_IOPARS_T`` parameter block. This object contains a set of device specific parameters to define the operation. By convention, the four parameters are: an operation code, a buffer pointer, a counter, and a spare pointer, However, the actual interpretation is defined by the particular device driver. The ``iopars`` object need not be preserved after the ``QIO_qio`` call.

outMbox
	Pointer to a mailbox. On completion of the operation, the IOCB will be delivered to this mailbox (for retrieval by ``QIO_result``).

compFunc
	Completion function pointer. ``NULL`` if not required. See discussion below.

timeout
	Time-out value.

As we can see, the processing model is "start an operation and arrange for the result to be delivered to the specified mailbox". In many cases the result is just the IOCB ready to be released for re=use. The real result data may be in memory buffers or, for an output operation, there may be no result data.

QIO completion functions
^^^^^^^^^^^^^^^^^^^^^^^^

The ``compFunc`` argument to ``QIO_qio`` lets you specify a so=called "completion function". QIO completion functions are provided with device, IOCB, I/O parameters and I/O completion status details. A completion function may either augment or replace the normal processing of the QIO system when an I/O operation completes.

QIO completion functions return a Boolean value (as a C ``int32_t``). If a completion function returns non-zero (``TRUE``), then it completely replaces the normal QIO completion activities. Most significantly, the IOCB will not be delivered to the output mailbox. If a completion function returns ``FALSE`` (0), then the normal QIO completion activities will be performed as well.

A common use for a completion function is in dealing with simple "fire and forget" output devices. By having the completion function release the IOCB (and possibly other buffers) for re=use, we don't need another task to collect and release them from the output mailbox. Indeed if you provide a completion function which always returns non=zero (``TRUE``), you don't need the output mailbox at all (and you can specify ``NULL`` for ``outMbox``). Conversely, if you provide a completion function which disposes of the IOCB, then it *must* return non=zero (``TRUE``).

Another use for completion functions is to provide additional signalling of I/O completion. For example a completion function might set an event flag or semaphore, before returning ``FALSE`` and letting the QIO system deliver the IOCB to the output mailbox as usual.

QIO completion functions are executed in interrupt context and therefore must be kept short, and non-blocking

QIO_result
^^^^^^^^^^

Completed operations are "collected" by calling ``QIO_result``, which takes the IOCB from the target mailbox and decodes it into useful information. Its arguments are:

mbox
	Pointer to mailbox where result is expected.

dev
	Pointer to a location to receive the device pointer. This allows the callee to identify the device which performed the operation, making it practical to direct results from several devices to a single mailbox.

status
	Pointer to a ``QIO_STATUS_T`` item. This receives the completion status of the operation (one of "normal", "cancelled", or "timed out"). Always check the completion status!

iopars
	Pointer to a ``QIO_IOPARS_T`` item. Normally this will receive a copy of the device specific parameters provided to the original ``QIO_qio`` call. Some drivers, however, may provide modified values. For example a requested byte count might be modified to return the number of bytes actually provided.

timeout
	Time-out value.

The return value is a pointer to the IOCB associated with the operation, or ``NULL`` if the call times out.

QIO_qioWait
^^^^^^^^^^^

The ``QIO_qio``/``QIO_result`` combination provides a very flexible I/O framework at the expense of some complexity for the user in managing IOCBs and mailboxes.

For simple applications, where the I/O initiator simply waits for the result, ``QIO_qioWait`` may be used. This function hides all details of IOCBs and mailboxes, requiring just the device, the device specific parameters and a time-out value.

QIO Time-outs
~~~~~~~~~~~~~

Both ``QIO_qio`` and ``QIO_result`` accept time-out arguments. A positive value specifies the maximum number of scheduler clock ticks to wait. A negative value means no time-out (infinite wait). Zero is meaningless and should not be used.

QIO_qio Time-outs
+++++++++++++++++

When a ``QIO_qio`` time-out expires, the system will attempt to cancel the operation and deliver it to the output mailbox with a "timed out" completion status.

Pending operations will always be cancelled successfully. However, you should be aware that it may not be possible to cancel an operation once it has reached the front of the queue and has been made active - this depends on the hardware design of the device. If the device and its driver do not support cancellation of active operations, then an operation which is activated before the time-out expires will then run to completion regardless of the time-out expiry.

QIO_result Time-outs
++++++++++++++++++++

A ``QIO_result`` time-out applies to the act of waiting on the output mailbox for a result, not to the I/O operation itself. If ``QIO_result`` times out, it simply means that no operation has completed *yet*. Pending or active operations may still be in the pipeline - a ``QIO_result`` time-out has no effect whatsoever on these or on the hardware device.

QIO_qioWait Time-outs
+++++++++++++++++++++

The time-out value provided to ``QIO_qioWait`` is treated as if it is applied to a ``QIO_qio`` call with an infinite wait on the corresponding ``QIO_result`` call. If the underlying driver does not support cancellation of active operations, then a ``QIO_qioWait`` will never time out, once the operation has become active. In typical usage patterns, ``QIO_qioWait`` doesn't build a queue of pending operations, so the underlying ``QIO_qio`` is usually activated immediately. If the driver doesn't support cancellation, the time-out feature is not very useful.

Unloading Device Drivers
~~~~~~~~~~~~~~~~~~~~~~~~

A device driver may be unloaded by calling ``QIO_unload``. Before unloading a device driver you must first ensure that there are no active operations on the device and then disable it by calling ``QIO_disable``.

When a device is unloaded:

*    Interrupts for the device are unrouted.
*    If the driver contains a shutdown function it is executed. Shutdown functions typically place the device hardware in a "safe" or "inactive" state.
*    The ``QIO_DEVICE_T`` device object is rendered invalid and should not be used unless the driver is re=installed with ``QIO_init``.

You need to take care when unloading a device which shares a hardware interrupt with other devices, since the act of unloading one device could disable the other devices sharing the interrupt. Unloading such devices should be a co=ordinated activity, with all devices sharing the interrupt being unloaded at the same time.

In practice, unloading device drivers is rarely necessary and best avoided. The only real reason to do it is when a system needs to transfer control of a device from one processor to another. Clearly a high degree of coordination is required to do this safely. You should probably treat a requirement to do this as a failure in the system or hardware design.

``QIO_unload`` is an escape route for a system design problem, not a recommended procedure.

Device Drivers
~~~~~~~~~~~~~~

A device driver is a software module which contains the specific code to operate a particular device. It is usually a fairly small software module providing up to seven standard functions which are called by the QIO system at various stages of the I/O process. Of these seven functions, five are optional. Many device drivers can be implemented by writing just two short functions.

Using Device Drivers
++++++++++++++++++++

The QIO system "binds" a device driver to a software device object when the device is initialised (using ``QIO_init``). Once this is done, the user need have no further interest in the device driver. All accesses to the device are performed through ``QIO_qio`` (or ``QIO_qioWait``).

A device driver is presented to its users as a ``QIO_DRIVER_T`` object. A typical device driver exports a global (usually ``const``) ``QIO_DRIVER_T`` item whose address is passed to ``QIO_init``. The ``QIO_DRIVER_T`` object is, in fact, a ``struct`` containing pointers to the driver's component functions, but QIO system users can (and should) treat the driver as an anonymous object.

Writing Device Drivers
++++++++++++++++++++++

To write a device driver, you need to:

*    Write up to seven standard device driver functions.
*    Define an array of ``IRQ_DESC_T`` descriptors for the related interrupts.
*    Declare a ``QIO_DRIVER_T`` object and initialise it with the interrupt descriptors, the number of interrupt descriptors, and the addresses of the driver functions. ``NULL`` pointers are used where optional functions are not provided. Initialisation of functions is usually performed at compile time, IRQ initialisation can be deferred to the initialisation function.

That's all! The seven functions are described  below. Refer to the QIO API definition for their formal specifications.

Initialisation Function QIO_INITFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The initialisation function is a required component of every driver. It is called by the QIO system to prepare the device for first use. It must do six things:

*    Prepare the hardware for use by writing appropriate initial values to registers, etc. The device should be set into a quiescent state and any spurious interrupts generated during initialisation should be cleared.
*    Initialise any software state information used by the other driver functions.
*    Inform the QIO system of the device's characteristics, in particular: rank and power saving class.
*    If needed, initialise the interrupt descriptors and fill in ``numIrqs`` to specify the number to be registered.

If you have multiple devices sharing the same driver, the initialisation function will be called for each device. You should be able to identify the particular device by accessing the ``id`` field of the device pointer argument which is passed to the initialisation function. (The ``id`` field contains the id parameter provided by the caller of ``QIO_init``).

You can assume that the initialisation function will be called with interrupts disabled, before any other driver function for that device. Keep it as short as possible.

Start Function QIO_STARTFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The start function is a required component of every driver. It is called by the QIO system to start a new operation on the device. Its two arguments are a pointer to the device object (``QIO_DEVICE_T *``) and a pointer to the parameter block (``QIO_IOPARS_T *``) defining the desired operation. The interpretation of the parameter block is up to the driver author. This is, after all, the device specific data. Make sure you describe the interpretation well in your driver documentation, so that your driver gets used as you intend.

If you have multiple devices sharing the same driver, you can identify the particular device by accessing the ``id`` field of the device pointer argument.

If everything is working properly, the start function will only be called when the device is genuinely able to accept a new operation (the QIO module does most of the work for you). During development, you may wish to include additional protection and checks, but if you do so, make these conditionally compiled for debugging only.

The start function will be called with interrupts disabled, so it should be kept as short as possible.

ISR Function QIO_ISRFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^

The Interrupt Service Routine (ISR) function is an optional driver component. If you don't provide an ISR function, the QIO system default ISR is used instead. The ISR function is called in response to device interrupts, normally this represents the completion of an operation.

The first important function of an ISR is to acknowledge interrupts, as described in `Handling and acknowledging`_. This is left to the driver writer, in order to provide a mechanism for supporting complex devices with more than one interrupt line and also for situations in which several devices may interrupt on the same line.

The second important function of the ISR is to activate parts of the QIO system by calling the following two functions exactly as shown. Indeed the default ISR does just this and little more.

.. code:: c

	QIO_complete(device, QIO_IOCOMPLETE);
	QIO_start(device);

These two calls must not be made until any required status information has been read and the device is ready to accept a new operation.

There are various circumstances in which a driver author would want to write a custom ISR. The three most important are:

*    You need to perform some device specific "acknowledge" operation to prepare the device for its next operation.
*    You design a driver that takes multiple physical interrupts to service a single logical QIO operation. For example, you might need to design a driver to read or write blocks of data on a device which interrupts on every byte transfer.
*    You need to return additional status information on completion of the operation. For example, the number of bytes actually transferred may be different from the number requested. You return additional status information by writing (or over-writing) fields in the device specific I/O parameter block (``QIO_IOPARS_T``) associated with the operation. The address of the parameter block isn't provided directly to the ISR function. It is provided to the start function, so your driver will need to remember this address. And do some careful tracking in the case of a multi rank device! Sorry about this trickiness, it's a consequence of trying to keep the interrupt processing simple and it isn't often needed.

For "fire and forget" DMA style hardware devices, the default ISR is usually sufficient.

The ISR function will be called with interrupts disabled, so it should be kept as short as possible.

Power Function QIO_POWERFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The power function is required unless you tell the QIO system (via ``QIO_init``) that the power saving class is ``QIO_POWERNONE``, in which case the power function will be ignored even if you provide it.

The power function is called by the QIO system with an argument of either ``QIO_POWERSAVE`` or ``QIO_POWERNORMAL``. The QIO system includes checks to avoid unnecessarily repeated calls with the same value so you don't need to do this yourself. We don't *guarantee* that you won't ever see repeated values, just that these will be infrequent enough not to worry about performance consequences.

The QIO system only calls the power function when it is safe to do so. The power save state of the device will not be altered while there are active operations.

The power function will be called with interrupts disabled, so it should be kept as short as possible.

Cancel Function QIO_CANCELFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The cancel function is an optional component of a driver. If it exists, the QIO system will call it to cancel *all* active operations on a device (in response to user requests or time-out processing). If it doesn't exist, then operations cannot be cancelled once started.

The cancel function need only return the hardware to a state where it is ready to receive a new operation. All other tracking is handled by the QIO system automatically.

The QIO system does not support cancellation of active operations on multi-rank devices. Even if you manage to write a suitable function for a multi-rank device it will be ignored.

The cancel function will be called with interrupts disabled, so it should be kept as short as possible.

Shutdown Function QIO_SHUTFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The shutdown function is an optional component of a driver. If present, it is called by the QIO system when a device driver is unloaded. It should:

*    Set the hardware into an inactive state. You can assume that the next thing to happen to the device (if anything) will be a re-initialisation - the driver initialisation function will be called.
*    If the driver initialisation function assumes any initial values for driver state variables, restore those values. (Driver code may, possibly inadvertently, assume that variables contain zero at start up for example.)
*    Test for, and clear any spurious pending interrupts which may have been generated by the shutdown operation.

The shutdown function does not (indeed should not) need to unroute interrupts. This will be done automatically by the QIO system, using the descriptors embedded in the device structure.

If you have multiple devices sharing the same driver, the shutdown function will be called for each device that is unloaded. You should be able to identify the particular device by accessing the ``id`` field of the device pointer argument which is passed to the shutdown function. (The ``id`` field contains the id parameter provided by the caller of ``QIO_init``).

Many devices are "naturally" quiescent when there are no active QIO operations. Shutdown functions are not normally required in such cases.

You can assume that the shutdown function will be called with interrupts disabled. Keep it as short as possible.

Simulation Control Function QIO_SIMFUNC_T
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The simulation control function is an optional component which allows you to write software simulations of hardware devices. This can be useful if you are waiting for a hardware development or if you want to apply the QIO control model to a software task. The simulation control function is called after device initialisation is complete but before it is first enabled. It will be called again as the first step in a device unload (``QIO_unload``) operation.

The function's ``start`` argument is set to 1 by the QIO system when it is called during device initialisation and to 0 when it is called during device unload.

Typically, simulation will use software interrupts to emulate it's behaviour. Please see `Software interrupts`_.

On device unload, the simulation control function would normally stop the task it started during device initialisation.

Unlike the other driver functions, the simulation control function is called in normal operating context, that is, with interrupts *enabled*.

Driver function limitations
+++++++++++++++++++++++++++

With the exception of the optional Simulation Control Function, device driver functions are executed by the QIO system in interrupt context. Normally, these functions don't interact directly with the other tasks in the system, since such interaction is channelled through the QIO system itself. Occasionally, however, it may be convenient for device drivers to obtain data buffers from pools or mailboxes or to set control semaphores. This is permissible, subject to the following restrictions:

*    The synchronisation object (semaphore etc.) must be a local object. It must not be imported or exported.
*    When testing a synchronisation object (test semaphore, get from mailbox or pool), the time-out value specified must be zero. There must be no possibility of the operation blocking. The operation will always succeed or fail immediately without waiting.
*    The function must not attempt to hibernate.
*    The function must not attempt to create or remove tasks.
