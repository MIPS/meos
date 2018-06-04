Configuration options
=====================

.. include:: config.rst

Change Log
==========

.. include:: changelog.rst

Devicetrees and MEOS
=====================

.. include:: dt.rst

Manual Integration
==================

Single Threaded
~~~~~~~~~~~~~~~

If you would prefer to integrate MEOS into your existing build system, there is a configuration discovery tool to assist with compiler integration. This is called ``$BUILD_DIR/bin/meos-config``. Invoking it with ``--cflags`` will returns the flags needed for compilation, and invoking it with ``--libs`` will provide the flags for linking.

The easiest way to use this in a standard make file would be to add lines like:

.. code:: make

	CFLAGS+=$(shell $(BUILD_DIR)/bin/meos-config --cflags)
	LDFLAGS+=$(shell $(BUILD_DIR)/bin/meos-config --libs)

C/C++ code that uses MEOS should include the primary header file:

.. code:: c

	#include "MEOS.h"

You should then be able to use any configured functionality.

Multi Threaded
~~~~~~~~~~~~~~

``meos-config`` can also assist with multithreaded builds. The following commands create the code and build fragments required to initialise the import/export system as per the configuration specified in the BSP used to configure MEOS.


To generate the required C code, execute the following, replacing ``N`` with the required processor index:

.. code:: sh

	$(BUILD_DIR)/bin/meos-config --cpu=N --impexpc=FILENAME

This will write the required C code to initialise the import/export mechanism into ``FILENAME``. You should compile this and link it with your application.

To generate the required linker script, execute the following, replacing ``N`` with the required processor index:

.. code:: sh

	$(BUILD_DIR)/bin/meos-config --cpu=N --impexpld=``FILENAME``

This will write the required linker script to initialise the import/export mechanism into ``FILENAME``. Use the ``@`` feature of ld to utilise this. Note that this will also interpret an "mti,embedded-cpu" devicetree node. If you do not want this behaviour, either omit the node, or override the values appropriately after the ``@``.

If you wish to use librproc, add ``--rproc`` to the call to ``meos-config``.

Platform specific notes
-----------------------

.. include:: platspec.rst

API Reference
-------------

.. include:: manual.html.rst

Support API Reference
---------------------

.. include:: supman.html.rst
