import ctypes, sys
if node["__address"] == defines["processor"]:
	try:
		if not "mti,vstart" in node:
			node["mti,vstart"] = node["mti,vmem"][0]
	except:
		pass
	if "mti,vmem" in node:
		sys.stdout.write("-Wl,-defsym,__memory_base=" + str(ctypes.c_uint32(int(node["mti,vmem"][0])).value) + " ")
		sys.stdout.write("-Wl,-defsym,__memory_size=" + str(ctypes.c_uint32(int(node["mti,vmem"][1])).value) + " ")
	if "mti,vstart" in node:
		sys.stdout.write("-Wl,-defsym,__app_start="+ str(ctypes.c_uint32(int(node["mti,vstart"])).value) + " ")
