if not "masses" in variables:
	variables["masses"] = 0
	print("""#ifdef CONFIG_MASS
extern void *mass_devices[];
#endif""")

node["mass_n"] = str(int(variables["masses"]))
variables["mass_device" + str(int(variables["masses"]))] = node
variables["masses"] = int(variables["masses"]) + 1
