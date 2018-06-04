


def mass_epilogue_close(node):
	def mass_epilogue():
		m = variables["masses"]
		if m < 0:
			return

		print("#ifdef CONFIG_MASS")

		for i in range(m):
			n = variables["mass_device" + str(i)]
			if "mti,fixed-partition" in node["compatible"]:
				print("""
	#ifndef CONFIG_MASS_PARTITION
	#error BSP requires MASS Partitioning support (CONFIG_MASS_PART)
	#endif

	MASSPART_T %(name)s;

	void %(name)s_init()
	{
		MASS_initPartition(&%(name)s, &%(parent)s, 0, %(start)sllu, 0, %(start)sllu + %(size)sllu);
	}
	""" % {"name":n["__name"], "parent":n["__parent"]["__parent"]["__name"], "start":n["reg"][0], "size":n["reg"][1]})

		print("void *MASSMUX_masses[%s] = {" % str(m))
		for i in range(m):
			comma = "," if i == m else ""
			print("	&" + variables["mass_device" + str(i)]["__name"] + comma)
		print("};")

		print("#endif	/* CONFIG_MASS */")
		variables["masses"] = -variables["masses"]

	return mass_epilogue

epilogues["mass"] = mass_epilogue_close(node)

def mass_close(node):

	def mass_init():
		print("#ifdef CONFIG_MASS")
		m = -variables["masses"]
		for i in range(m):
			n = variables["mass_device" + str(i)]
			if not "mti,fixed-partition" in n["compatible"]:
				print("	" + variables["mass_device" + str(i)]["__name"] + "_init();")
		for i in range(m):
			n = variables["mass_device" + str(i)]
			if "mti,fixed-partition" in n["compatible"]:
				print("	" + variables["mass_device" + str(i)]["__name"] + "_init();")
		print("#endif	/* CONFIG_MASS */")

	return mass_init

inits["bsp"]["_mass"] = mass_close(node)
