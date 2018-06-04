
execfile("clock-bindings.py")
execfile("interrupts.py")
name = node["__name"]

if "mti,tx-buffer" in node:
	tlen = node["mti,tx-buffer"]
else:
	tlen = "CONFIG_DRIVER_PIC32_UART_TX_BUFFER"
node["tlen"] = tlen
if "mti,rx-buffer" in node:
	rlen = node["mti,rx-buffer"]
else:
	rlen = "CONFIG_DRIVER_PIC32_UART_RX_BUFFER"
node["rlen"] = rlen

if "clock-frequency" in node:
	clock = node["clock-frequency"]
else:
	clock = getClock(node, 0)

# Device specific common
print("""#ifdef CONFIG_DRIVER_PIC32_UART
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/pic32uart/pic32uart.h\"
#ifdef __cplusplus
}
#endif""")

# Device specialisation

print("""
uint8_t %(name)sTXBuf[%(tlen)s];
uint8_t %(name)sRXBuf[%(rlen)s];
IRQ_DESC_T %(name)sRxInt;
IRQ_DESC_T %(name)sTxInt;
PIC32_UART_T %(name)s = {
	.pAddr = %(pAddr)s,
	.clock = %(clock)s
};""" % {"tlen": tlen, "rlen": rlen, "name": name, "pAddr": node["reg"][0], "clock": clock})

if "mti,debug-log" in node:
	print("UART_T *_DBG_uart = (UART_T *)&%(name)s;" % {"name": name})

print("#endif")

def PIC32_UART_close(node, decodeInterrupt):

	def PIC32_UART_init():
		rIntName = node["__name"] + "RxInt"
		wIntName = node["__name"] + "TxInt"
		print("#ifdef CONFIG_DRIVER_PIC32_UART")
		ints = zip(node["interrupts"][::2], node["interrupts"][1::2])
		decodeInterrupt(node, ints[1], rIntName)
		decodeInterrupt(node, ints[2], wIntName)
		print("PIC32_UART_init(&%(name)s, %(name)sTXBuf, %(tlen)s, %(name)sRXBuf, %(rlen)s, &%(rIntName)s, &%(wIntName)s);" % {"name": node["__name"], "tlen": node["tlen"], "rlen": node["rlen"], "rIntName": rIntName, "wIntName": wIntName})
		if "mti,debug-log" in node:
			print("_DBG_file = UART_fopen((UART_T*)&%(name)s, \"r+\")" % {"name": node["__name"]})
		print("#endif")

	return PIC32_UART_init

inits["bsp"][name] = PIC32_UART_close(node, decodeInterrupt)
