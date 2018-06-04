execfile("interrupts.py")

if "mti,ipti" in node:
	ipti = "const uint32_t _IPTI = %s;" % node["mti,ipti"]
else:
	ipti = ""

print("""#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/irq/irq.h\"
const uint32_t _IRQ_linuxOffset = 0;
%s
#ifdef __cplusplus
}
#endif""" % ipti)

def decodeCPUInterrupt(interrupt, descName):
	print("%(descName)s.intNum = %(intNum)s;" % {"descName": descName, "intNum": interrupt[0]})

variables["interruptParent"]["mti,cpu-interrupt-controller"] = decodeCPUInterrupt
