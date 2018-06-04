


def LWIP_init():
	print("""#ifdef CONFIG_LWIP
	LWIP_init(&lwip);
	tcpip_init(NULL, NULL);
#endif""")

if not "_lwip_initialised" in variables:
	variables["_lwip_initialised"] = True
	inits["bsp"]["__lwip"] = LWIP_init
	print("""#ifdef CONFIG_LWIP
#ifdef __cplusplus
extern "C" {
#endif
#include \"lwip/sys.h\"
#include \"lwip/tcpip.h\"
#ifdef __cplusplus
}
#endif
static LWIP_T lwip;
#endif""")
