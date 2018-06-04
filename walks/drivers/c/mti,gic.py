execfile("interrupts.py")

if "reg" in node:
	reg = "uint32_t *_GIC = (uint32_t *)(KSEG1_BASE + %(reg)s);" % {"reg": node["reg"][0]}
else:
	reg = ""
if "mti,no-reset" in node:
	reset = "uint32_t _IRQ_reset = 0;"
else:
	reset = ""

print("""#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/irq/irq.h\"
const uint32_t _IRQ_linuxOffset = 7;
%(reg)s
%(reset)s
void _IRQ_sharedShunt(int32_t intNum);
#ifdef __cplusplus
}
#endif""" % {"reg": reg, "reset": reset})

def decodeGICInterrupt(interrupt, descName):
	if int(interrupt[0]) != 0: # GIC_SHARED
		raise ValueError("Only GIC_SHARED interrupts supported")
	if int(interrupt[2]) == 2:
			polarity = "IRQ_FALLING_EDGE"
			trigger = "IRQ_EDGE_TRIGGERED"
	elif int(interrupt[2]) == 3:
			polarity = "0"
			trigger = "IRQ_EDGE_DOUBLE_TRIGGERED"
	elif int(interrupt[2]) == 4:
			polarity = "IRQ_ACTIVE_HIGH"
			trigger = "IRQ_LEVEL_SENSITIVE"
	elif int(interrupt[2]) == 8:
			polarity = "IRQ_ACTIVE_LOW"
			trigger = "IRQ_LEVEL_SENSITIVE"
	else:
		polarity = "IRQ_RISING_EDGE"
		trigger = "IRQ_EDGE_TRIGGERED"
	print("""%(descName)s.intNum = IRQ_MULTIPLEXED;
%(descName)s.isrFunc = _IRQ_sharedShunt;
%(descName)s.impSpec.extNum = %(extNum)s;
%(descName)s.impSpec.polarity = %(polarity)s;
%(descName)s.impSpec.trigger = %(trigger)s;""" % {"descName": descName, "extNum": interrupt[1], "polarity": polarity, "trigger": trigger})

variables["interruptParent"]["mti,gic"] = decodeGICInterrupt
