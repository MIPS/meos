Device drivers
==============

MEOS provides drivers for a number of hardware devices.

For most devices, you only need to enable the driver and supporting middleware in the MEOS configuration, and ensure the device is specified in your devicetree file. ``BSP_init()`` will then take care of any initialisation required.

You may need to further configure middleware before the device becomes usable.

librproc
========

librproc allows Linux to boot and communicate with a baremetal MEOS application via remoteproc. It handles discovery and synchronisation with a Linux primary system, and ensures the resultant ELF contains the appropriate resource table entries required by the remoteproc loader.


If your primary system is Linux based, and all MEOS processors are booted via remoteproc, you do not need to use the devicetree to specify the interprocessor interface.

Instead, build your application with librproc. The library will automatically be made available when you build a compatible MEOS. If you are usingthe MEOS project build system, add the following to activate this feature:

.. code:: make

	RPROC := Y

Otherwise, pass ``--rproc`` to meos-config when you are requesting linker parameters.

If you wish to use a hybrid system, with some processors not under remoteproc control, you will need to define devicetree nodes for all MEOS processors.

LOPRI
=====

LOPRI implements a common lowest priorty work queue. The module declares a work queue called ``LOPRI`` to which jobs may be submitted using ``KRN_queueWQ``. This is used by drivers and utility libraries to combine low priority support tasks together, and consequently eliminate the memory used by their stacks. Please see `The lopri module`_ for further details.

QIO
===

.. include:: qio.rst

MVZ/VIO
=======

.. include:: mvzvio.rst

SNTP
====

A Simple Network Time Protocol client, used to seed the ``SRTC`` module and periodically resynchronise it. Please see `The sntp module`_ for further details.

SRTC
====

A software Real Time Clock, implemented using system timers. May be seeded using ``SNTP``. Please see `The srtc module`_ for further details.

TFTP
====

An implementation of the Trivial File Transfer Protocol, allowing data to be read/write from a locally networked server. Please see `The tftp module`_ for further details.


MASS
====

Provides general purpose functions for manipulating mass storage, i.e. ROM, flash, fixed disk. It also provides a couple of simple implementations:

``MASSRAM``
	A memory backed virtual drive. This allows the creation of RAM disks, or can be flagged as read-only to use directly addressable ROM/flash as a disk.

``MASSPART``
	Splits another ``MASS`` device into one or more predefined, static partitions.

Please see `The mass module`_ for further details.

CPIO
====

A simple read only filesystem. The file system image should be created with the Unix cpio archiver. The raw data from the file should then be placed at the start of a ``MASS`` device or partition. The data may then be accessed using the functions of the ``CPIO`` module. Optionally, data size may be traded for code size, by enabling `zlib`_ support in MEOS, and compressing the resultant cpio archive with gzip before using the raw data with ``MASS``.  Please see `The cpio module`_ for further details.

Newtron
=======

The Newtron flash filing system, hosted on a MASS device. Once a ``MASS`` compatible device has been mounted with ``NEWTRON_init``, simply use as usual. All libraries and include paths should be set up by meos-config. Please see `http://mynewt.apache.org/os/modules/fs/nffs/nffs/ <http://mynewt.apache.org/os/modules/fs/nffs/nffs/>`_ and `The Newtron module`_ for further details.

lwIP
====

The light weight IP networking stack. Please see `http://lwip.wikia.com/wiki/LwIP_Wiki <http://lwip.wikia.com/wiki/LwIP_Wiki>`_. The board support package will initialise the core of LWIP and install all configured packet drivers. Interfaces must however be configured prior to use. All libraries and include paths should be setup by meos-config, simply use as usual.

FatFs
=====

A FAT compatible file system. Please see `http://elm-chan.org/fsw/ff/00index_e.html <http://elm-chan.org/fsw/ff/00index_e.html>`_. ``MASS`` compatible devices defined in the Devicetree will be mapped to FatFs drive numbers in order of appearance. All libraries and include paths should be set up by meos-config, simply use as usual.

zlib
====

The zlib compression library. Please see `http://www.zlib.net/ <http://www.zlib.net/>`_. All libraries and include paths should be set
up by meos-config, simply use as usual.
