
execfile("clock-bindings.py")
execfile("interrupts.py")
execfile("lwip.py")

name = node["__name"]

if "mac-address" in node:
	mac = "{%(m0)s,%(m1)s,%(m2)s,%(m3)s,%(m4)s,%(m5)s}" % {"m0":node["mac-address"][0], "m1":node["mac-address"][1], "m2":node["mac-address"][2], "m3":node["mac-address"][3], "m4":node["mac-address"][4], "m5":node["mac-address"][5]}
else:
	mac = "{0, 0, 0, 0, 0, 0}"

# Device specific common
print("""#ifdef CONFIG_DRIVER_LAN9211
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/lan9211/lan9211.h\"
#ifdef __cplusplus
}
#endif
""")

# Device specialisation
print("""
IRQ_DESC_T %(name)sInt;
LAN9211_T %(name)s = {
	.pAddr = %(pAddr)s,
	.backupMac = %(MAC)s
};
#endif""" % {"name": name, "pAddr": node["reg"][0], "MAC": mac})

def LAN9211_close(node, decodeInterrupt):

	def LAN9211_init():
		intName = node["__name"] + "Int"
		print("#ifdef CONFIG_DRIVER_LAN9211")
		decodeInterrupt(node, node["interrupts"], intName)
		print("""LAN9211_dtInit(&%(name)s, &%(intName)s);
#endif""" % {"name": node["__name"], "intName": intName})

	return LAN9211_init

inits["bsp"][name] = LAN9211_close(node, decodeInterrupt)
