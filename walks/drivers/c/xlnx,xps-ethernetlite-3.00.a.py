
execfile("clock-bindings.py")
execfile("interrupts.py")
execfile("lwip.py")

name = node["__name"]

if "mac-address" in node:
	mac = "{%(m0)s,%(m1)s,%(m2)s,%(m3)s,%(m4)s,%(m5)s}" % {"m0":node["mac-address"][0], "m1":node["mac-address"][1], "m2":node["mac-address"][2], "m3":node["mac-address"][3], "m4":node["mac-address"][4], "m5":node["mac-address"][5]}
else:
	mac = "{0x00, 0x00, 0x5e, 0x00, 0xfa, 0xce}"

phy = "1"
if "phy-handle" in node:
	findPH(root, findUp(node, "interrupt-parent"))
	phynode = findPH(root, node["phy-handle"])
	if "reg" in phynode:
		phy = phynode["reg"]
	elif "__address" in phynode:
		phy = phynode["__address"]

# Device specific common
print("""#ifdef CONFIG_DRIVER_XPSETHLITE
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/xpsethlite/xpsethlite.h\"
#ifdef __cplusplus
}
#endif
""")

# Device specialisation
print("""
IRQ_DESC_T %(name)sInt;
XPSETHLITE_T %(name)s = {
	.pAddr = %(pAddr)s,
	.mac = %(MAC)s,
	.phy = %(PHY)s,
};
#endif""" % { "name": name, "pAddr": node["reg"][0], "MAC": mac, "PHY": phy})

def XPSETHLITE_close(node, decodeInterrupt):

	def XPSETHLITE_init():
		intName = node["__name"] + "Int"
		print("#ifdef CONFIG_DRIVER_XPSETHLITE")
		decodeInterrupt(node, node["interrupts"], intName)
		print("""XPSETHLITE_dtInit(&%(name)s, &%(intName)s);
#endif""" % {"name": node["__name"], "intName": intName})

	return XPSETHLITE_init

inits["bsp"][name] = XPSETHLITE_close(node, decodeInterrupt)
