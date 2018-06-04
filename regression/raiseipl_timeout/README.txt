A long standing bug led to the incorrect paths through MEOS being taken by
tasks with a raised IPL. Although not necessarily serious timeouts would be
permitted when they were impossible to implement. With the bug fixed, the
debug MEOS libraries can raise asserts (as intended) when the API is not
adhered to.

This test raises the IPL. Once in this state only non-blocking calls to a subset
of the MEOS API are permitted. The test checks internal MEOS state to assert
that timeouts were not even attempted when invoking the 4 operations
permitted in this mode.
