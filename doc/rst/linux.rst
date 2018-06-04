MIPS
~~~~

MEOS Linux/MIPS support is experimental and not fully tested.

FPU
~~~

FPU context storage is handled by Linux.

Performance counters
~~~~~~~~~~~~~~~~~~~~

Counter 0 returns the number of nanoseconds as per `CLOCK_MONOTONIC`.

IRQ specification
~~~~~~~~~~~~~~~~~

Linux interrupts are a simulation, and do not map to system interrupts. The Linux IRQ implementation provides 32 simulated software interrupts.

IRQ specialisation
~~~~~~~~~~~~~~~~~~

There is no specialisation for Linux interrupts.
