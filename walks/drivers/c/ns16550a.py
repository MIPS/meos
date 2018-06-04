
execfile("clock-bindings.py")
execfile("interrupts.py")
name = node["__name"]

if "mti,tx-buffer" in node:
	tlen = node["mti,tx-buffer"]
else:
	tlen = "CONFIG_DRIVER_NS16550A_TX_BUFFER"
node["tlen"] = tlen
if "mti,rx-buffer" in node:
	rlen = node["mti,rx-buffer"]
else:
	rlen = "CONFIG_DRIVER_NS16550A_RX_BUFFER"
node["rlen"] = rlen

if "clock-frequency" in node:
	clock = node["clock-frequency"]
else:
	clock = getClock(node, 0)

if "reg-io-width" in node:
	width = node["reg-io-width"]
else:
	width = 1

if "reg-shift" in node:
	pitch = node["reg-shift"]
else:
	pitch = 1

# Device specific common
print("""#ifdef CONFIG_DRIVER_NS16550A
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/ns16550a/ns16550a.h\"
#ifdef __cplusplus
}
#endif""")

# Device specialisation

print("""
uint8_t %(name)sTXBuf[%(tlen)s];
uint8_t %(name)sRXBuf[%(rlen)s];
IRQ_DESC_T %(name)sInt;
NS16550A_T %(name)s = {
	.pAddr = %(pAddr)s,
	.clock = %(clock)s,
	.width = %(width)s,
	.pitch = %(pitch)s
};""" % {"tlen": tlen, "rlen": rlen, "name": name, "pAddr": node["reg"][0], "clock": clock, "width": width, "pitch" : pitch})

if "mti,debug-log" in node:
	print("UART_T *_DBG_uart = (UART_T *)&%(name)s;" % {"name": name})

print("#endif")

def NS16550A_close(node, decodeInterrupt):

	def NS16550A_init():
		intName = node["__name"] + "Int"
		print("#ifdef CONFIG_DRIVER_NS16550A")
		decodeInterrupt(node, node["interrupts"], intName)
		print("NS16550A_init(&%(name)s, %(name)sTXBuf, %(tlen)s, %(name)sRXBuf, %(rlen)s, &%(intName)s);" % {"name": node["__name"], "tlen": node["tlen"], "rlen": node["rlen"], "intName": intName})
		if "mti,debug-log" in node:
			print("_DBG_file = UART_fopen((UART_T*)&%(name)s, \"r+\")" % {"name": node["__name"]})
		print("#endif")

	return NS16550A_init

inits["bsp"][name] = NS16550A_close(node, decodeInterrupt)
