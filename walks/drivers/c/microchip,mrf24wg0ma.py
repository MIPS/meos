
execfile("clock-bindings.py")
execfile("interrupts.py")
execfile("lwip.py")

name = node["__name"]

# Device specific common
print("""#ifdef CONFIG_DRIVER_MRF24G
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/mrf24g/mrfnetif.h\"
#ifdef __cplusplus
}
#endif
""")

# Device specialisation
print("""
IRQ_DESC_T %(name)sInt;
MRFNETIF_T %(name)s = {
	.pAddr = %(pAddr)s,
};
#endif""" % {"name": name,  "pAddr": node["reg"][0]})

def MRFNETIF_close(node, decodeInterrupt):

	def MRFNETIF_init():
		intName = node["__name"] + "Int"
		print("#ifdef CONFIG_DRIVER_MRF24G")
		decodeInterrupt(node, node["interrupts"], intName)
		print("""MRFNETIF_dtInit(&%(name)s, &%(intName)s);
#endif""" % {"name": node["__name"], "intName": intName})

	return MRFNETIF_init

inits["bsp"][name] = MRFNETIF_close(node, decodeInterrupt)
