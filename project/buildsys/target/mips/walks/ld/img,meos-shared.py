import ctypes, sys

sys.stdout.write("-Wl,-defsym,__meosrings_start=" + str(ctypes.c_uint32(int(node["mti,vmem"][0])).value) + " ")
