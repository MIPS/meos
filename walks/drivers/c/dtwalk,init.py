def bsp_open():
	print("""#ifdef CONFIG_LOPRI
LOPRI_T lopriInstance;
#endif
void BSP_init()
{
#ifdef CONFIG_LOPRI
LOPRI_init(&lopriInstance);
#endif""")

def bsp_close():
	print("}")

if not "bsp" in inits:
	inits["bsp"] = collections.OrderedDict()
	inits["bsp"]["__open"] = bsp_open
	inits["bsp"]["__close"] = bsp_close
