/*******************************************************************************************
 * Codec_eDMA.c
 * This version sets up the eDMA (DMAMUX0, DMA0) in the MCXN947 MCU.
 * Currently set up for ping-pong buffers on both stereo inputs and outputs.
 * 05/22/2024 Todd Morton
 ******************************************************************************************/

/*******************************************************************************************
* Include files
*******************************************************************************************/
#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "AppDSP.h"
#include "CodecSAI.h"
#include "Codec_eDMA.h"
#include "FRDM_MCXN947_GPIO.h"
/*******************************************************************************************
* Module Defines
*******************************************************************************************/
#define DMA_IN_CH            2
#define DMA_OUT_CH           0
#define DMA_BYTES_PER_BLOCK      (DSP_SAMPLES_PER_BLOCK*DSP_BUFFER_BYTES_PER_SAMPLE)
#define DMA_BYTES_PER_BUFFER     (DSP_NUM_BLOCKS*DSP_NUM_IN_CHANNELS*DMA_BYTES_PER_BLOCK)
#define DMA_IN_CHANNEL_OFFSET    (DSP_NUM_BLOCKS*DMA_BYTES_PER_BLOCK)
#define DMA_OUT_CHANNEL_OFFSET   (DSP_NUM_BLOCKS*DMA_BYTES_PER_BLOCK)

typedef struct{
    INT8U index;
    OS_SEM flag;
}DMA_BLOCK_RDY;
/*******************************************************************************************
* Private Functions Declarations
*******************************************************************************************/
static DMA_BLOCK_RDY dmaInBlockRdy;
void EDMA_0_CH2_IRQHandler(void);

/*******************************************************************************************
* Global Variables
*******************************************************************************************/
/*******************************************************************************************
* Function Code
********************************************************************************************
DMAInInit
    Initializes DMA for an input stream from ADC0 to ping-pong buffers
    Parameters: none
    Return: none
*******************************************************************************************/
void DMAInit(DSP_BLOCK_T *dsp_in_buf, DSP_BLOCK_T *dsp_out_buf){
    OS_ERR os_err;

    OSSemCreate(&dmaInBlockRdy.flag, "Block Ready", 0, &os_err);

    // dmaInBlockRdy.index indicates the buffer currently not being used by the DMA in the Ping-Pong scheme.
    // It uses the DONE bit in the CSR to determine where the DMA is at. If DONE is 1, the DMA just finished
    // the [1] block and will start filling the [0] block. So, when DONE is one, we want the processing to use
    // the [1] block to avoid collisions with the DMA.
    // Since the DMA starts with the [0] block, initialize the index to the [1] block.

    dmaInBlockRdy.index = 1;

    //enable DMA0 clock
	SYSCON->AHBCLKCTRLSET[0] = SYSCON_AHBCLKCTRL0_DMA0(1);

    //Minor Loop Mapping Enabled, Round Robin Arbitration, Debug enabled
    DMA0->MP_CSR = DMA_MP_CSR_ERCA(1) | DMA_MP_CSR_EDBG(1);

    /**** START: DMA config for Input DMA  */

    //source address is I2S receive data register
    DMA0->CH[DMA_IN_CH].TCD_SADDR = DMA_TCD_SADDR_SADDR(&SAI1->RDR[0]);

    //No offset for source data address.  Always read RDR
    DMA0->CH[DMA_IN_CH].TCD_SOFF = DMA_TCD_SOFF_SOFF(0);

    //Source and destination data size
    DMA0->CH[DMA_IN_CH].TCD_ATTR = DMA_TCD_ATTR_SMOD(0) | DMA_TCD_ATTR_SSIZE(2)
    		                     | DMA_TCD_ATTR_DMOD(0) | DMA_TCD_ATTR_DSIZE(2);

    //Destination Minor Loop Offset is enabled.  After each minor loop, the destination
    //pointer jumps back to the next sample in the first channel buffer
    // NBYTES = channels*bytes per sample.
    DMA0->CH[DMA_IN_CH].TCD_NBYTES_MLOFFYES = DMA_TCD_NBYTES_MLOFFYES_DMLOE(1) | DMA_TCD_NBYTES_MLOFFYES_SMLOE(0)
                                            | DMA_TCD_NBYTES_MLOFFYES_MLOFF(-(DMA_BYTES_PER_BUFFER)+DSP_BUFFER_BYTES_PER_SAMPLE)
                                            | DMA_TCD_NBYTES_MLOFFYES_NBYTES(DSP_NUM_IN_CHANNELS*DSP_BUFFER_BYTES_PER_SAMPLE);

    //No adjustment to source address at end of major loop.
    DMA0->CH[DMA_IN_CH].TCD_SLAST_SDA = DMA_TCD_SLAST_SDA_SLAST_SDA(0);

    //destination buffer address
    DMA0->CH[DMA_IN_CH].TCD_DADDR = DMA_TCD_DADDR_DADDR(dsp_in_buf);

    DMA0->CH[DMA_IN_CH].TCD_DOFF = DMA_TCD_DOFF_DOFF(DMA_IN_CHANNEL_OFFSET);

    //Set minor loop iteration counters to number of minor loops in the major loop
    DMA0->CH[DMA_IN_CH].TCD_CITER_ELINKNO = DMA_TCD_CITER_ELINKNO_ELINK(0)|DMA_TCD_CITER_ELINKNO_CITER(DSP_NUM_BLOCKS*DSP_SAMPLES_PER_BLOCK);

    DMA0->CH[DMA_IN_CH].TCD_BITER_ELINKNO = DMA_TCD_BITER_ELINKNO_ELINK(0)|DMA_TCD_BITER_ELINKNO_BITER(DSP_NUM_BLOCKS*DSP_SAMPLES_PER_BLOCK);

    //After Major loop, jump back to the beginning of each channel buffer
    DMA0->CH[DMA_IN_CH].TCD_DLAST_SGA = DMA_TCD_DLAST_SGA_DLAST_SGA(-(DMA_IN_CHANNEL_OFFSET+DMA_BYTES_PER_BUFFER-DSP_BUFFER_BYTES_PER_SAMPLE));

	//Enable interrupt at half filled Rx buffer and end of major loop.
	//This allows "ping-pong" buffer processing.
	DMA0->CH[DMA_IN_CH].TCD_CSR = DMA_TCD_CSR_BWC(3) | DMA_TCD_CSR_INTHALF(1) | DMA_TCD_CSR_INTMAJOR(1);


    /**** START: DMA config for DMA Out  */

    //source address
    DMA0->CH[DMA_OUT_CH].TCD_SADDR = DMA_TCD_SADDR_SADDR(dsp_out_buf);

    //No offset for single channel
    DMA0->CH[DMA_OUT_CH].TCD_SOFF = DMA_TCD_SOFF_SOFF(DMA_OUT_CHANNEL_OFFSET);

    //Source data size
    DMA0->CH[DMA_OUT_CH].TCD_ATTR = DMA_TCD_ATTR_SMOD(0) | DMA_TCD_ATTR_SSIZE(2)
    		                      | DMA_TCD_ATTR_DMOD(0) | DMA_TCD_ATTR_DSIZE(2);

    //Destination Minor Loop Offset is enabled.  After each minor loop, the destination
    //pointer jumps back to the next sample in the first channel buffer
    // NBYTES = channels*bytes per sample.
    DMA0->CH[DMA_OUT_CH].TCD_NBYTES_MLOFFYES = DMA_TCD_NBYTES_MLOFFYES_DMLOE(0) | DMA_TCD_NBYTES_MLOFFYES_SMLOE(1)
                                             | DMA_TCD_NBYTES_MLOFFYES_MLOFF(-(DMA_BYTES_PER_BUFFER)+DSP_BUFFER_BYTES_PER_SAMPLE)
                                             | DMA_TCD_NBYTES_MLOFFYES_NBYTES(DSP_NUM_IN_CHANNELS*DSP_BUFFER_BYTES_PER_SAMPLE);

    //No adjustment to destination address at end of major loop.
    DMA0->CH[DMA_OUT_CH].TCD_DLAST_SGA = DMA_TCD_DLAST_SGA_DLAST_SGA(0);

    DMA0->CH[DMA_OUT_CH].TCD_DOFF = DMA_TCD_DOFF_DOFF(0);

    //Source buffer address
    DMA0->CH[DMA_OUT_CH].TCD_DADDR = DMA_TCD_DADDR_DADDR(&SAI1->TDR[0]);

    //Set minor loop iteration counters to number of minor loops in the major loop
    DMA0->CH[DMA_OUT_CH].TCD_CITER_ELINKNO = DMA_TCD_CITER_ELINKNO_ELINK(0)|DMA_TCD_CITER_ELINKNO_CITER(DSP_NUM_BLOCKS*DSP_SAMPLES_PER_BLOCK);
    DMA0->CH[DMA_OUT_CH].TCD_BITER_ELINKNO = DMA_TCD_BITER_ELINKNO_ELINK(0)|DMA_TCD_BITER_ELINKNO_BITER(DSP_NUM_BLOCKS*DSP_SAMPLES_PER_BLOCK);

    //After Major loop, jump back to the beginning of each channel buffer
    DMA0->CH[DMA_OUT_CH].TCD_SLAST_SDA = DMA_TCD_SLAST_SDA_SLAST_SDA(-(DMA_IN_CHANNEL_OFFSET+DMA_BYTES_PER_BUFFER-DSP_BUFFER_BYTES_PER_SAMPLE));

    //No output channel interrupts
    DMA0->CH[DMA_OUT_CH].TCD_CSR = DMA_TCD_CSR_BWC(3);


    //Output source is SAI1-TX
    DMA0->CH[DMA_OUT_CH].CH_MUX = DMA_CH_MUX_SRC(102);
    //Input source is SAI1-RX
    DMA0->CH[DMA_IN_CH].CH_MUX = DMA_CH_MUX_SRC(101);

    //enable DMA Rx interrupt
    NVIC_EnableIRQ(EDMA_0_CH2_IRQn);

    //All set to go, enable DMA channel(s)!
    DMA0->CH[DMA_IN_CH].CH_CSR = DMA_CH_CSR_ERQ(1);
    DMA0->CH[DMA_OUT_CH].CH_CSR = DMA_CH_CSR_ERQ(1);

}

/****************************************************************************************
 * DMA Interrupt Handler to indicate the end of a ping-pong block
 ***************************************************************************************/
void EDMA_0_CH2_IRQHandler(void){
    OS_ERR os_err;
    OSIntEnter();
    DB1_TURN_ON();
    DMA0->CH[DMA_IN_CH].CH_INT = DMA_CH_INT_INT(1);
    if((DMA0->CH[DMA_IN_CH].CH_CSR & DMA_CH_CSR_DONE_MASK) != 0){
    	dmaInBlockRdy.index = 1;      //set buffer index to opposite of DMA
	    OSSemPost(&(dmaInBlockRdy.flag),OS_OPT_POST_1,&os_err);
    	if(DSPSuspendReqGet()){       //suspend stream if requested
    		DSPSuspend();
    	}else{}
    }else{
    	dmaInBlockRdy.index = 0;
        OSSemPost(&(dmaInBlockRdy.flag),OS_OPT_POST_1,&os_err);
    }
    DB1_TURN_OFF();
    OSIntExit();
}
/****************************************************************************************
 * DMA signal when full or half full
 * 08/30/2015 TDM
 ***************************************************************************************/
INT8U DMAInPend(OS_TICK tout, OS_ERR *os_err_ptr){

    OSSemPend(&(dmaInBlockRdy.flag), tout, OS_OPT_PEND_BLOCKING,(void *)0, os_err_ptr);
    return dmaInBlockRdy.index;
}
/****************************************************************************************
 * Suspend DMA.
 * Must suspend source of DMA hardware requests before calling this.
 * 08/28/2024 TDM
 ***************************************************************************************/
void DMASuspend(void){

	while((DMA0->MP_HRS & (1 << DMA_IN_CH)) != 0){}		//wait for all hardware request to be handled
	while((DMA0->MP_HRS & (1 << DMA_OUT_CH)) != 0){}
    DMA0->CH[DMA_IN_CH].CH_CSR &= ~DMA_CH_CSR_ERQ(1); 	//disable DMA requests
    DMA0->CH[DMA_OUT_CH].CH_CSR &= ~DMA_CH_CSR_ERQ(1);

}
/****************************************************************************************
 * DMAResume.
 * Resume DMA hardware requests. Must be done before source of DMA hardware requests are enabled.
 * 08/28/2024 TDM
 ***************************************************************************************/
void DMAResume(void){

    //All set to go, enable DMA channel(s)!
    DMA0->CH[DMA_IN_CH].CH_CSR |= DMA_CH_CSR_ERQ(1);
    DMA0->CH[DMA_OUT_CH].CH_CSR |= DMA_CH_CSR_ERQ(1);

}
