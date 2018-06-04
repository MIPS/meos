next_decl = ""
next_in_a_call = ""
next_in_b_call = ""
next_out_call = ""

if next is not None:
	next_decl = "inline static void IPM_startInA" + next["__address"] + "(void);\ninline static void IPM_startInB" + next["__address"] + "(void);\ninline static void IPM_startOut" + next["__address"] + "(void); /* Next link in chain */"
	next_in_a_call = "IPM_startInA" + next["__address"] + "(); /* Chain */"
	next_in_b_call = "IPM_startInB" + next["__address"] + "(); /* Chain */"
	next_out_call = "IPM_startOut" + next["__address"] + "(); /* Chain */"
else:
	next_decl = "/* Last in chain */";
	next_in_a_call = "/* Last in chain */";
	next_in_b_call = "/* Last in chain */";
	next_out_call = "/* Last in chain */";

# If it's our first time, emit common code
if node == first:
	print ("""
	#include "MEOS.h"

	#ifdef CONFIG_FEATURE_IMPEXP

	#define IPMC(X, Y) [((X) * (CONFIG_FEATURE_MAX_PROCESSORS - 1)) + (((Y) < (X)) ? (Y) : (Y) - 1)]
	#define IPMN(X, Y) (((X) * (CONFIG_FEATURE_MAX_PROCESSORS - 1)) + (((Y) < (X)) ? (Y) : (Y) - 1))
	#define MASSAGEO(T, F) ((((unsigned)(T)) > ((unsigned)(F))) ? ((T) - 1) : (T))
	#define MASSAGE(T) MASSAGEO((T), _KRN_schedule->hwThread)

	/* This pointer is shared between all MEOS threads */
	volatile extern uint8_t *__meosrings_start[];

	/* Ordered so that librproc can truncate */
	typedef struct IPM_tag {
		MQGUEST_T inQ;
		MQHOST_T outQ;
		KRN_POOL_T inHPool;
		KRN_POOL_T outHPool;
		MQ_MSG_T inHeader[24];
		MQ_MSG_T outHeader[24];
		IRQ_DESC_T inKick;
		IRQ_DESC_T inShin;
		IRQ_DESC_T outKick;
		IRQ_DESC_T outShin;
		KRN_POOL_T outMPool;
		uint8_t outMessage[24][sizeof(MQ_HOSTBUF_T)];
		uint8_t inRing[8192];
	} IPM_T;

	IPM_T *IPM_config[CONFIG_FEATURE_MAX_PROCESSORS];

	#define STACKSIZE 4000
	static KRN_WQ_T rxq;
	static KRN_TASK_T rxqTasks[1];
	static uint32_t rxqStacks[1][STACKSIZE];
	static KRN_JOB_T rxqJobs[1];
	__attribute__ ((weak)) uint32_t IPM_nix = 0;

	/*
	** FUNCTION:    RPROC_bind
	**
	** DESCRIPTION: Hook for UHI coprocessor interface
	**
	** RETURNS:     void
	*/
	void RPROC_bind() __attribute__((weak));
	void RPROC_bind()
	{
		/* By default, don't do anything */
	}

	inline static void IPM_startInA%(first_address)s(void);
	inline static void IPM_startInB%(first_address)s(void);
	inline static void IPM_startOut%(first_address)s(void);

	/*
	** FUNCTION:    IPM_start
	**
	** DESCRIPTION: Deferred initialisation performed once the scheduler is running.
	**
	** RETURNS:     void
	*/
	void IPM_start()
	{
		/* Zero shared data - this is OK, it will regenerate */
		memset(__meosrings_start, 0, sizeof(uint8_t *)*CONFIG_FEATURE_MAX_PROCESSORS*CONFIG_FEATURE_MAX_PROCESSORS);
		/* RX WQ */
		KRN_initWQ(&rxq, rxqTasks, (uint32_t *) rxqStacks, 1, STACKSIZE,
			   KRN_maxPriority() - 1, rxqJobs, 1);
		/* Try and bind Linux */
		RPROC_bind();
		/* Create all the incoming rings */
		IPM_startInA%(first_address)s();
		IPM_startInB%(first_address)s();
		/*
		 * Create all the outgoing rings - may busy wait until all participating
		 * processors have created their incoming rings.
		 */
		IPM_startOut%(first_address)s();
	}

	/*
	** FUNCTION:	IPM_send
	**
	** DESCRIPTION:	Send a message to another processor.
	**
	** RETURNS:	void
	*/
	void IPM_send(KRN_MSG_T * msg)
	{
		MQHOST_T *q = NULL;
		MQ_MSG_T *out = NULL;
		uint8_t to = msg->to;

		if (IPM_config[MASSAGE(msg->to)])
			q = &IPM_config[MASSAGE(msg->to)]->outQ;
		if (!q)
		{
			q = &IPM_config[0]->outQ; /* Fall back to relaying via Linux (0) */
			to = 0;
		}
		DBG_assert(q, "Trying to send message to non-existant processor\\n");
		if (!q)
			return;
		if ((to == 0) && (IPM_nix))
		{
			do {
				out = MQGUEST_take((MQGUEST_T*)q, 0);
				_KRN_ignoreZeroTimeout();
			} while (!out);

			MQ_MSG_set(out, KRN_MSG_SIZE, &msg->from);

			_IPM_impexpDebug("TX", msg);
			MQGUEST_send((MQGUEST_T*)q, out);
		}
		else
		{
			do {
				out = MQHOST_take(q, 0);
				_KRN_ignoreZeroTimeout();
				if (!out)
					MQHOST_autoRel(q);
			} while (!out);

			MQ_MSG_set(out, KRN_MSG_SIZE, &msg->from);

			_IPM_impexpDebug("TX", msg);
			MQHOST_send(q, out);
		}
	}

	/*
	** FUNCTION:	_IPM_callback
	**
	** DESCRIPTION:	IPI interrupt handler - feed incoming messages upwards.
	**
	** RETURNS:	void
	*/
	void _IPM_callback(void)
	{
		uint32_t from;
		uint32_t to = _KRN_schedule->hwThread;
		MQGUEST_T *mq;
		MQ_MSG_T *in;
		KRN_MSG_T msg;
		for (from = 0; from < CONFIG_FEATURE_MAX_PROCESSORS; from++)
			if (from != to) {
				mq = &IPM_config[MASSAGE(from)]->inQ;
			 	if (mq) {
					while ((in = MQGUEST_recv(mq, 0))) {
						memcpy((void *)&msg.from, (void *)MQ_MSG_data(in),
							   KRN_MSG_SIZE);
						MQGUEST_return(mq, in);
						IPM_recv(&msg);
					}
				}
			}
	}

	/*
	** FUNCTION:	_IPM_qCallback
	**
	** DESCRIPTION:	IPI interrupt handler - feed incoming messages upwards.
	**
	** RETURNS:	void
	*/
	void _IPM_qCallback(MQ_T * mq, void *cbPar)
	{
		/* If this fails, then there must be one in the pipe anyway */
		KRN_queueWQ(&rxq, (KRN_TASKFUNC_T *) _IPM_callback, mq, "IPM cb", 0);
	}

	/*
	** FUNCTION:	IPM_rewriteRings
	**
	** DESCRIPTION:	Rewrite ring data.
	**
	** RETURNS:	void
	*/
	static inline void IPM_rewriteRings(void)
	{
		uint32_t from;
		uint32_t to = _KRN_schedule->hwThread;

		/*
		 * The process of multithread loading might result in __meosrings_start
		 * being zeroed. Regenerate it from our internal tables.
		 */
		for (from = 0; from < CONFIG_FEATURE_MAX_PROCESSORS; from++)
			if ((from != to) && (IPM_config[MASSAGE(from)]))
				__meosrings_start IPMC(from, to) = IPM_config[MASSAGE(from)]->inRing;
	}

	static uint32_t IPM_intIn = 0;

	#endif
	""" % {"first_address" : first["__address"]})

# Per instance
print("""
#ifdef CONFIG_FEATURE_IMPEXP

IPM_T IPM%(node_address)s;

%(next_decl)s

/*
** FUNCTION:    IPM_startInA%(node_address)s
**
** DESCRIPTION: Create incoming ring
**
** RETURNS:     void
*/
inline static void IPM_startInA%(node_address)s()
{
	const uint32_t from = 0x%(node_address)s;
	uint32_t to = _KRN_schedule->hwThread;
	if (from == to)
	{
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
		IPM_intIn = %(node_address)s;
#else
		IPM_intIn = %(node_irq)s;
#endif
	}
	else if (IPM_config[MASSAGE(from)] == NULL)
	{
#ifdef CONFIG_ARCH_MIPS_BASELINE_MT
		IRQ_ipi(%(node_address)s, &IPM%(node_address)s.inKick);
		IRQ_ipi(%(node_address)s, &IPM%(node_address)s.outKick);
#else
		IRQ_ipi(%(node_irq)s, &IPM%(node_address)s.inKick);
		IRQ_ipi(%(node_irq)s, &IPM%(node_address)s.outKick);
#endif

		KRN_initPool(&IPM%(node_address)s.inHPool, &IPM%(node_address)s.inHeader, 24, sizeof(MQ_MSG_T));
		KRN_initPool(&IPM%(node_address)s.outHPool, &IPM%(node_address)s.outHeader, 24, sizeof(MQ_MSG_T));
		KRN_initPool(&IPM%(node_address)s.outMPool, &IPM%(node_address)s.outMessage, 24, sizeof(MQ_HOSTBUF_T));

		__meosrings_start IPMC(from, to) = IPM%(node_address)s.inRing;
	}
	%(next_in_a_call)s
}

/*
** FUNCTION:    IPM_startInB%(node_address)s
**
** DESCRIPTION: Create incoming ring
**
** RETURNS:     void
*/
inline static void IPM_startInB%(node_address)s()
{
	const uint32_t from = 0x%(node_address)s;
	uint32_t to = _KRN_schedule->hwThread;
	if ((from != to) && (IPM%(node_address)s.outQ.mq.size == 0)) {
		IRQ_ipi(IPM_intIn, &IPM%(node_address)s.inShin);
		IRQ_ipi(IPM_intIn, &IPM%(node_address)s.outShin);

		MQGUEST_init(&IPM%(node_address)s.inQ, &IPM%(node_address)s.inHPool, IPM%(node_address)s.inRing, 4096, &IPM%(node_address)s.inShin, &IPM%(node_address)s.inKick);
		IPM_config[MASSAGE(from)] = &IPM%(node_address)s;
	}
	%(next_in_b_call)s
}

/*
** FUNCTION:    IPM_startOut%(node_address)s
**
** DESCRIPTION: Create outgoing ring
**
** RETURNS:     void
*/
inline static void IPM_startOut%(node_address)s()
{
	uint32_t from = _KRN_schedule->hwThread;
	volatile uint8_t *ring;
	const uint32_t to = %(node_address)s;
	if ((from != to) && (IPM%(node_address)s.outQ.mq.size == 0)) {
		while ((ring = __meosrings_start IPMC(from, to)) == 0)
		{
			IPM_rewriteRings();
		}
		IPM_rewriteRings();
		MQHOST_init(&IPM%(node_address)s.outQ, &IPM%(node_address)s.outHPool, (void*)ring, 4096, &IPM%(node_address)s.outShin, &IPM%(node_address)s.outKick, &IPM%(node_address)s.outMPool);
		MQGUEST_setCallback(&IPM%(node_address)s.inQ, _IPM_qCallback, NULL);
	}
	%(next_out_call)s
}

#endif
""" % {"node_address" : node["__address"], "node_irq" : node["interrupts"][1] if "interrupts" in node else "0", "next_decl" : next_decl, "next_in_a_call" : next_in_a_call, "next_in_b_call" : next_in_b_call, "next_out_call" : next_out_call})
