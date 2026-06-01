#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "CodecSAI.h"
#include "CodecDA7212.h"
#include "FRDM_MCXN947_GPIO.h"
#include "AppDSP.h"
#include "Codec_eDMA.h"
#include "arm_math.h"

/******************************************************************************************/
/* FFT configuration */
#define FFT_LENGTH DSP_SAMPLES_PER_BLOCK

/******************************************************************************************/
/* LMS adaptive filter configuration */
#define LMS_NUM_TAPS      32
#define LMS_MU            0.005f

/******************************************************************************************/
/* DMA audio buffers */
static DSP_BLOCK_T dspInBuffer[DSP_NUM_IN_CHANNELS][DSP_NUM_BLOCKS];
static DSP_BLOCK_T dspOutBuffer[DSP_NUM_OUT_CHANNELS][DSP_NUM_BLOCKS];

static INT8U dspSuspendReqFlag = 0;
static OS_SEM dspSuspended;

/*******************************************************************************************
* LMS adaptive filter globals
*
* Left input  = noisy signal d[n] + w'[n]
* Right input = reference noise w[n]
*
* Adaptive FIR estimates w'[n] from w[n].
* Error signal:
*
*     e[n] = d[n] + w'[n] - y[n]
*
* Output right channel = e[n], the recovered/cleaned signal.
*******************************************************************************************/
static float32_t lmsCoeff[LMS_NUM_TAPS];
static float32_t wHistory[LMS_NUM_TAPS];

static float32_t lmsMu = LMS_MU;

/* Debug/view variables */
static float32_t currentNoisySample = 0.0f;
static float32_t currentNoiseRefSample = 0.0f;
static float32_t currentEstimatedNoise = 0.0f;
static float32_t currentErrorSample = 0.0f;

/*******************************************************************************************
* FFT globals for cleaned RIGHT output verification
*******************************************************************************************/
static q31_t fftInputRight[FFT_LENGTH];
static q31_t fftResultRight[FFT_LENGTH * 2];
static q31_t fftMagRight[FFT_LENGTH];

static arm_rfft_instance_q31 fftInstanceRight;

static q31_t peakMagRight = 0;
static uint32_t peakIndexRight = 0;
static uint32_t peakBinRight = 0;
static INT32U peakFreqRight = 0;
static INT32U fftResolutionHz = 0;

/*******************************************************************************************
* Private Function Prototypes
*******************************************************************************************/
static void dspTask(void *p_arg);
static CPU_STK dspTaskStk[APP_CFG_DSP_TASK_STK_SIZE];
static OS_TCB dspTaskTCB;

/*******************************************************************************************
* DSPInit()
*******************************************************************************************/
void DSPInit(void){
    OS_ERR os_err;

    OSTaskCreate(&dspTaskTCB,
                "DSP Task ",
                dspTask,
                (void *) 0,
                APP_CFG_DSP_TASK_PRIO,
                &dspTaskStk[0],
                (APP_CFG_DSP_TASK_STK_SIZE / 10u),
                APP_CFG_DSP_TASK_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                &os_err);

    OSSemCreate(&dspSuspended, "stream suspended", 0, &os_err);

    DMAInit(&dspInBuffer[0][0], &dspOutBuffer[0][0]);
    SAIInit(32);
    SAI_RX_ENABLE();
    SAI_TX_ENABLE();
    CODECInit();

    /*************************************************************************
    * Clear LMS coefficients and reference-noise history buffer.
    *************************************************************************/
    for(uint32_t i = 0; i < LMS_NUM_TAPS; i++){
        lmsCoeff[i] = 0.0f;
        wHistory[i] = 0.0f;
    }

    /*************************************************************************
    * Initialize FFT for optional debugger verification.
    *************************************************************************/
    arm_rfft_init_q31(&fftInstanceRight, FFT_LENGTH, 0, 1);

    fftResolutionHz = DSPSampleRateGet() / FFT_LENGTH;
}

/*******************************************************************************************
* dspTask
*
* Adaptive noise cancellation using LMS FIR filter.
*
* Per sample:
*
*   noisy = left input  = d[n] + w'[n]
*   ref   = right input = w[n]
*
*   y[n] = dot(lmsCoeff, wHistory)
*   e[n] = noisy - y[n]
*
*   lmsCoeff[i] = lmsCoeff[i] + mu * e[n] * wHistory[i]
*
* Output:
*   left output  = original noisy signal for comparison
*   right output = cleaned/error signal e[n]
*******************************************************************************************/
static void dspTask(void *p_arg){

    OS_ERR os_err;
    INT8U buffer_index;

    (void)p_arg;

    while(1){

        DB0_TURN_OFF();
        buffer_index = DMAInPend(0, &os_err);
        DB0_TURN_ON();

        for(uint32_t n = 0; n < DSP_SAMPLES_PER_BLOCK; n++){

            /*****************************************************************
            * Convert Q31 input samples to float.
            *****************************************************************/
            q31_t noisyQ31 = dspInBuffer[DSP_LEFT_CH][buffer_index].samples[n];
            q31_t refQ31   = dspInBuffer[DSP_RIGHT_CH][buffer_index].samples[n];

            float32_t noisy = ((float32_t)noisyQ31) / 2147483648.0f;
            float32_t ref   = ((float32_t)refQ31)   / 2147483648.0f;

            currentNoisySample = noisy;
            currentNoiseRefSample = ref;

            /*****************************************************************
            * Update reference noise history buffer.
            * wHistory[0] is newest sample.
            *****************************************************************/
            for(int32_t i = LMS_NUM_TAPS - 1; i > 0; i--){
                wHistory[i] = wHistory[i - 1];
            }
            wHistory[0] = ref;

            /*****************************************************************
            * Adaptive FIR output:
            *
            * y[n] = dot(f, wvec)
            *
            * This is the estimated noise component.
            *****************************************************************/
            float32_t y = 0.0f;

            for(uint32_t i = 0; i < LMS_NUM_TAPS; i++){
                y += lmsCoeff[i] * wHistory[i];
            }

            currentEstimatedNoise = y;

            /*****************************************************************
            * Error signal:
            *
            * e[n] = d[n] + w'[n] - y[n]
            *
            * This should become the cleaned/recovered signal.
            *****************************************************************/
            float32_t e = noisy - y;

            currentErrorSample = e;

            /*****************************************************************
            * LMS coefficient update based on assignment Equation (1):
            *
            * f_i(n+1) = f_i(n) + mu * w(n-i) * e(n)
            *
            * In code:
            *
            * lmsCoeff[i] = lmsCoeff[i] + lmsMu * e * wHistory[i];
            *****************************************************************/
            for(uint32_t i = 0; i < LMS_NUM_TAPS; i++){
                lmsCoeff[i] = lmsCoeff[i] + lmsMu * e * wHistory[i];
            }

            /*****************************************************************
            * Limit output to avoid clipping.
            *****************************************************************/
            if(e > 0.999999f){
                e = 0.999999f;
            }else if(e < -1.0f){
                e = -1.0f;
            }

            /*****************************************************************
            * Convert output back to Q31.
            *
            * Left output  = noisy input for comparison.
            * Right output = cleaned signal e[n].
            *****************************************************************/
            q31_t cleanQ31 = (q31_t)(e * 2147483647.0f);

            dspOutBuffer[DSP_LEFT_CH][buffer_index].samples[n] = noisyQ31;
            dspOutBuffer[DSP_RIGHT_CH][buffer_index].samples[n] = cleanQ31;

            fftInputRight[n] = cleanQ31;
        }

        /*************************************************************************
        * Optional FFT verification on cleaned RIGHT output.
        *************************************************************************/
        arm_rfft_q31(&fftInstanceRight,
                     &fftInputRight[0],
                     &fftResultRight[0]);

        arm_cmplx_mag_q31(&fftResultRight[0],
                          &fftMagRight[0],
                          FFT_LENGTH);

        arm_max_q31(&fftMagRight[1],
                    (FFT_LENGTH / 2) - 1,
                    &peakMagRight,
                    &peakIndexRight);

        peakBinRight = peakIndexRight + 1u;
        peakFreqRight = (peakBinRight * DSPSampleRateGet()) / FFT_LENGTH;
    }
}

/*******************************************************************************************
* DSPSampleSizeSet
*******************************************************************************************/
void DSPSampleSizeSet(INT8U ssize){

    (void)CODECSampleSizeSet(ssize);

}

/*******************************************************************************************
* DSPSampleSizeGet
*******************************************************************************************/
INT8U DSPSampleSizeGet(void){

	INT8U ssize;
	ssize = CODECSampleSizeGet();
	return ssize;

}

/*******************************************************************************************
* DSPSampleRateGet
*******************************************************************************************/
INT32U DSPSampleRateGet(void){

	INT32U srate;
	srate = CODECSampleRateGet();
	return srate;

}

/*******************************************************************************************
* DSPSampleRateSet
*******************************************************************************************/
void DSPSampleRateSet(INT32U srate){

    (void)CODECSampleRateSet(srate);

    fftResolutionHz = srate / FFT_LENGTH;
}

/*******************************************************************************************
* DSPResume
*******************************************************************************************/
void DSPResume(void){

    dspSuspendReqFlag = 0;
    DMAResume();
    SAIResume();
    CODECEnable();
}

/*******************************************************************************************
* DSPSuspendReqSet
*******************************************************************************************/
void DSPSuspendReqSet(void){

    dspSuspendReqFlag = 1;

}

/*******************************************************************************************
* DSPSuspendReqGet
*******************************************************************************************/
INT8U DSPSuspendReqGet(void){

    return dspSuspendReqFlag;

}

/*******************************************************************************************
* DSPSuspend
*******************************************************************************************/
void DSPSuspend(void){

	OS_ERR os_err;

	CODECDisable();
	SAISuspend();
	DMASuspend();
	OSSemPost(&dspSuspended, OS_OPT_POST_1, &os_err);

}

/****************************************************************************************
 * DSP signal when buffer is full and DSP stream is suspended.
 ***************************************************************************************/
void DSPSuspendedPend(OS_TICK tout, OS_ERR *os_err_ptr){

    OSSemPend(&dspSuspended, tout, OS_OPT_PEND_BLOCKING, (void *)0, os_err_ptr);

}

/****************************************************************************************
 * Return a pointer to the requested buffer
 ***************************************************************************************/
INT32S *DSPBufferGet(BUFF_ID_T buff_id){

    INT32S *buf_ptr = (void*)0;

    if(buff_id == LEFT_IN){
        buf_ptr = (INT32S *)&dspInBuffer[DSP_LEFT_CH][0];
    }else if(buff_id == RIGHT_IN){
        buf_ptr = (INT32S *)&dspInBuffer[DSP_RIGHT_CH][0];
    }else if(buff_id == LEFT_OUT){
        buf_ptr = (INT32S *)&dspOutBuffer[DSP_LEFT_CH][0];
    }else if(buff_id == RIGHT_OUT){
        buf_ptr = (INT32S *)&dspOutBuffer[DSP_RIGHT_CH][0];
    }else{
    }

    return buf_ptr;
}


