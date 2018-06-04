

def SEAD3_init():
	print("""	{
		uint32_t *SEAD3_CFG = (uint32_t *) 0xbb100110;
		DBG_assert(((*SEAD3_CFG) & 2), "No GIC!");
	}""")

# Broken SEAD-3 bitstreams frequently fail to advertise a GIC
#if not findFirst("mti,gic") is None:
#	inits["bsp"]["/"] = SEAD3_init
