
execfile("clock-bindings.py")
execfile("interrupts.py")

name = node["__name"]

# Device specific common
print("""#ifdef __cplusplus
extern "C" {
#endif
#include "MEOS.h"
#ifdef __cplusplus
}
#endif""")

# Device specialisation
print("""
IRQ_DESC_T %(name)sInt;
XPSINTC_T %(name)s = {
	.pAddr = 0x%(pAddr)08X,
};""" % { "name": name, "pAddr": long(node["reg"][0]) & 0xFFFFFFFF})

def XPSINTC_close(node, decodeInterrupt):

	def XPSINTC_init():
		intName = node["__name"] + "Int"
		decodeInterrupt(node, node["interrupts"], intName)
		print("XPSINTC_init(&%(name)s, &%(intName)s);" % {"name": node["__name"], "intName": intName})

	return XPSINTC_init

inits["bsp"][name] = XPSINTC_close(node, decodeInterrupt)

def decodeXPSINTCInterrupt(interrupt, descName):
	print("\t%(descName)s.intNum = %(intNum)s;" % {"descName": descName, "intNum": interrupt})

variables["interruptParent"]["xlnx,xps-intc-1.00.a"] = decodeXPSINTCInterrupt
