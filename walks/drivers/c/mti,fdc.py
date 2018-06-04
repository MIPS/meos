
execfile("clock-bindings.py")
execfile("interrupts.py")
name = node["__name"]

if "mti,tx-buffer" in node:
	tlen = node["mti,tx-buffer"]
else:
	tlen = "CONFIG_DRIVER_FDC_TX_BUFFER"
node["tlen"] = tlen
if "mti,rx-buffer" in node:
	rlen = node["mti,rx-buffer"]
else:
	rlen = "CONFIG_DRIVER_FDC_RX_BUFFER"
node["rlen"] = rlen

# Device specific common
print("""#include "meos/config.h"
#ifdef CONFIG_DRIVER_FDC
#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/fdc/fdc.h\"
#ifdef __cplusplus
}
#endif""")

# Device specialisation

print("""
uint8_t %(name)sTXBuf[%(tlen)s];
uint8_t %(name)sRXBuf[%(rlen)s];
FDC_T %(name)s = {
	.channel = %(channel)s
};
""" % {"name": name, "tlen": node["tlen"], "rlen": node["rlen"], "channel": node["channel"]})

if "mti,debug-log" in node:
	print("UART_T *_DBG_uart = (UART_T *)&%(name)s;" % {"name": name})

print("#endif")
def FDC_close(node, decodeInterrupt):

	def FDC_init():
		print("""#ifdef CONFIG_DRIVER_FDC
FDC_init(&%(name)s, %(name)sTXBuf, %(tlen)s, %(name)sRXBuf, %(rlen)s);""" % {"name": node["__name"], "tlen": node["tlen"], "rlen": node["rlen"]})
		if "mti,debug-log" in node:
			print("_DBG_file = UART_fopen((UART_T*)&%(name)s, \"r+\");" % {"name": node["__name"]})
			print("setvbuf(_DBG_file, NULL, _IOLBF, 0);")
		print("#endif")

	return FDC_init

inits["bsp"][name] = FDC_close(node, decodeInterrupt)
