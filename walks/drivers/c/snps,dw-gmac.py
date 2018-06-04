
execfile("clock-bindings.py")
execfile("interrupts.py")
execfile("lwip.py")

name = node["__name"]

if "mac-address" in node:
	mac = "{%(m0)s,%(m1)s,%(m2)s,%(m3)s,%(m4)s,%(m5)s}" % {"m0":node["mac-address"][0], "m1":node["mac-address"][1], "m2":node["mac-address"][2], "m3":node["mac-address"][3], "m4":node["mac-address"][4], "m5":node["mac-address"][5]}
else:
	mac = "{0, 0, 0, 0, 0, 0}"

# Device specific common
print("""#ifdef CONFIG_DRIVER_DWETHGMAC
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/dwethgmac/dwethgmac.h\"
#ifdef __cplusplus
}
#endif
""")

# Device specialisation
print("""
IRQ_DESC_T %(name)sInt;
DWETHGMAC_T %(name)s = {
	.pAddr = %(pAddr)s,
	.mac = %(MAC)s
};
#endif""" % {"name": name, "pAddr": node["reg"][0], "MAC": mac})

def DWETHGMAC_close(node, decodeInterrupt):

	def DWETHGMAC_init():
		intName = node["__name"] + "Int"
		print("#ifdef CONFIG_DRIVER_DWETHGMAC")
		decodeInterrupt(node, node["interrupts"], intName)
		print("""DWETHGMAC_dtInit(&%(name)s, &%(intName)s);
#endif""" % {"name": node["__name"], "intName": intName})

	return DWETHGMAC_init

inits["bsp"][name] = DWETHGMAC_close(node, decodeInterrupt)
