import sys
if "T" in defines:
	minust = defines["T"]
else:
	minust = "uhi32.ld"
sys.stdout.write("-T" + minust + " -Wl,-defsym,__use_excpt_boot=0 ")
if "vectorCount" in defines:
		sys.stdout.write("-Wl,-defsym,__isr_vec_count=%s " % str(defines["vectorCount"]))
