execfile("mti,mass-prologue.py")
node["pAddr"] = node["reg"][0]
node["vAddr"] = node["mti,vmem"][0]
if not "mti,sector-size" in node:
	node["mti,sector-size"] = 512
node["nSectors"] = str(int(node["reg"][1]) / int(node["mti,sector-size"]))


print("""
#ifdef CONFIG_MASS
#ifndef CONFIG_MASS_RAM
#error BSP requires MASS RAM/ROM disk support (CONFIG_MASS_RAM)
#endif

MASSRAM_T %(__name)s;

void %(__name)s_init()
{
	MEM_map((uintptr_t)%(pAddr)s, %(nSectors)sull * %(mti,sector-size)sull, (void*)%(vAddr)s);
	MASS_initRam(&%(__name)s, (void *)%(vAddr)s, %(nSectors)sull * %(mti,sector-size)sull, 0);
}
#endif
""" % node)

execfile("mti,mass-epilogue.py")
