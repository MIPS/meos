#include "MEOS.h"

/* Priority levels - highest priority reserved for timers */
#define PRIORITIES 5
#define MAX_PRIORITY (PRIORITIES -1)
/* Stack fill value - 32 bit value used to initialise stacks. Required to be non-zeroto enable stack KRN_stackInfo() - this incurs a slight performance penalty on task start up. */
#define STACKFILL	0
/* Interrupt stack size - should be sized to allow exceptions within interrupts */
#define ISTACKSIZE 2048
/* Timer task stack size */
#define TSTACKSIZE 2048
/* Number of entries in trace buffer - 0 to disable */
#define TRACEBUFFERSIZE 0

/* Declare storage for kernel configuration */
static KRN_SCHEDULE_T scheduler;
static KRN_TASKQ_T schedulerQueues[PRIORITIES];
static uint32_t interruptStack[ISTACKSIZE];
#if TRACEBUFFERSIZE == 0
	#define traceBuffer NULL
#else
	KRN_TRACE_T traceBuffer[TRACEBUFFERSIZE];
#endif
static uint32_t timerStack[TSTACKSIZE];

/* Declare handle for primary task */
KRN_TASK_T *primaryTask;

int main(int argc, const char *argv[])
{
	/* Specify kernel storage */
	KRN_reset(&scheduler, schedulerQueues, MAX_PRIORITY, STACKFILL, interruptStack, ISTACKSIZE, traceBuffer, TRACEBUFFERSIZE);
	/* Start scheduling */
	primaryTask = KRN_startOS("Background Task");
	/* Start timers */
	(void)KRN_startTimerTask("Timer Task", timerStack, TSTACKSIZE)

	/* Initialise board - this configures middleware and device drivers */
	BSP_init();

	/* Your code goes here! */
	DBG_logF("Hello world!\n");

	/* Terminates program, regardless of other tasks */
	return 0;
}
