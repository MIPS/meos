
execfile("clock-bindings.py")

print("#include \"MEOS.h\"")

def MIPS_close(node, speed):

	def MIPS_init():
		print("""{
	uint32_t div;
	asm (\"rdhwr %%0, $3\":\"=r\"(div)::);
	TMR_setClockSpeed(%(speed)s / (div * 1000000));
}""" % {"name": node["__name"], "speed": speed})

	return MIPS_init

clock = getClock(node, 0)
if clock:
	inits["bsp"][node["__name"]] = MIPS_close(node, clock)
