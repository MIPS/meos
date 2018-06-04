A bug was identified where a task struct was not fully initialised, so any
non-static structs (ie., on the stack) would get random values inserted into
some of their fields. This caused the FPU saving code to be invoked on a
non-FPU CPU, and thus crashed.
 In this test, deliberately blast the stack with noise, and then fill the
local task structures with more noise, and then see if we can still create
tasks and schedule OK.

