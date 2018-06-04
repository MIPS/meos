execfile("interrupts.py")


print("""#ifdef __cplusplus
extern "C" {
#endif
#include \"meos/irq/irq.h\"
const uint32_t _IRQ_linuxOffset = 0;
#ifdef __cplusplus
}
#endif""")
def decodePICInterrupt(interrupt, descName): 
	if int(interrupt[1]) == 2:
			polarity = "IRQ_FALLING_EDGE"
	else:
		polarity = "IRQ_RISING_EDGE"
	print("""%(descName)s.intNum = %(intNum)s;
%(descName)s.impSpec.polarity = %(polarity)s;""" % {"descName": descName, "intNum": interrupt[0], "polarity": polarity})

variables["interruptParent"]["microchip,pic32mzda-evic"] = decodePICInterrupt
