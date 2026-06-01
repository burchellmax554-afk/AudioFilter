/*****************************************************************************************
* A simple DSP demo program that uses uCOS-III.
* 04/26/16, Todd Morton
* 03/28/2020, Todd Morton
*****************************************************************************************/
#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "FRDM_MCXN947_GPIO.h"
#include "AppDSP.h"
#include "FRDM_MCXN947ClkCfg.h"
#include "BasicIO.h"
#include "DSPShell.h"
#include "os_app_hooks.h"
/*****************************************************************************************
* Allocate task control blocks
*****************************************************************************************/
static OS_TCB appTaskStartTCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
static CPU_STK appTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes. 
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
static void  appStartTask(void *p_arg);

/*****************************************************************************************
* main()
*****************************************************************************************/
void main(void) {

    OS_ERR  os_err;

    FRDM_MCXN947InitBootClock();
    BIOOpen(BIO_BIT_RATE_115200);	//Startup BasicIO for asserts
    OSInit(&os_err);                    /* Initialize uC/OS-III                         */
    OSTaskCreate((OS_TCB     *)&appTaskStartTCB,            /* Create the start task    */
                 (CPU_CHAR   *)"Start Task",
                 (OS_TASK_PTR ) appStartTask,
                 (void       *) 0,
                 (OS_PRIO     ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK    *)&appTaskStartStk[0],
                 (CPU_STK     )(APP_CFG_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);

    assert(!os_err);
    OSStart(&os_err);               /*Start multitasking(i.e. give control to uC/OS)    */
    while(1){}                 /* Should never get here                            */
    
}

/*****************************************************************************************
* STARTUP TASK
* This should run once and be suspended. Could restart everything by resuming.
* (Resuming not tested)
*****************************************************************************************/
static void appStartTask(void *p_arg) {

    OS_ERR os_err;
    (void)p_arg;                        /* Avoid compiler warning for unused variable   */

    OS_CPU_SysTickInitFreq(SystemCoreClock);
    OSStatTaskCPUUsageInit(&os_err);
    GpioDBugBitsInit();
    DSPInit();
    DSPShell_Init();
    while(1){

        OSTaskSuspend((OS_TCB *)0, &os_err);
    }
}


/******************************************************************************************/
