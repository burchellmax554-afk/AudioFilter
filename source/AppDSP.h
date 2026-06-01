/*****************************************************************************************************
* AppDSP.h
*
* 01/15/2024 Todd Morton
*****************************************************************************************************/

/*****************************************************************************************************
* Module definition against multiple inclusion
*****************************************************************************************************/
#ifndef  SAMPLE_PROC_PRESENT
#define  SAMPLE_PROC_PRESENT

/*****************************************************************************************************
* DSP configuration constants.
* Here a buffer is the complete data object, which may be comprised of multiple blocks. For example,
* when using a ping-pong buffer, there are two blocks.
*****************************************************************************************************/
#define DSP_NUM_BLOCKS                  2
#define DSP_SAMPLES_PER_BLOCK           256
#define DSP_BUFFER_BYTES_PER_SAMPLE     4
#define DSP_NUM_IN_CHANNELS             2
#define DSP_NUM_OUT_CHANNELS            2
#define DSP_LEFT_CH                     0
#define DSP_RIGHT_CH                    1

/*****************************************************************************************************
* DSP global sample blocks, typedef
*****************************************************************************************************/
typedef struct{
    q31_t samples[DSP_SAMPLES_PER_BLOCK];
} DSP_BLOCK_T;

typedef enum{LEFT_IN, RIGHT_IN, LEFT_OUT, RIGHT_OUT} BUFF_ID_T;
/*****************************************************************************************************
* Declaration of project wide FUNCTIONS
*****************************************************************************************************/
void DSPInit(void);
void DSPSampleSizeSet(INT8U ssize);
void DSPSampleRateSet(INT32U srate);
INT8U DSPSampleSizeGet(void);
INT32U DSPSampleRateGet(void);
void DSPResume(void);
void DSPSuspend(void);
void DSPSuspendReqSet(void);
INT8U DSPSuspendReqGet(void);
void DSPSuspendedPend(OS_TICK tout, OS_ERR *os_err_ptr);
INT32S *DSPBufferGet(BUFF_ID_T buff_id);

#endif
