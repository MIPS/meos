# This file provides a template for implementing block devices for MASS.

execfile("mti,mass-prologue.py")

# Device specialisation
print("""
#if defined(CONFIG_MASS) && defined(_CONFIG_TEMPLATE_GUARD)
#error Change _CONFIG_TEMPLATE_GUARD

TEMPLATE_DISK_T %(__name)s = {
	#error Add data here
}

void %(__name)s_init()
{
	TEMPLATE_DISK_init(&%(__name)s, ...);
	#error Change and extend initialisation code here
}

#endif	/* CONFIG_MASS */

""" % node)

execfile("mti,mass-epilogue.py")
