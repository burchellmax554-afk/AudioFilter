/*****************************************************************************************************
* SAI.c
*
* Initializes the SAI transmit and receive. In this case SAI1, used to connect to the DA7212 codec
* arduino board connected to the FRDM-MCXN947.
* Generates a 50MHz MCLK for the CODEC. The CODEC then generates the BCLK and WCLK's.
* To change sample size, both the CODEC and the SAI module must be set to the correct size.
* 05/29/2024 Todd Morton
*****************************************************************************************************/
/*****************************************************************************************************
* Include files
*****************************************************************************************************/
#include "MCUType.h"
#include "CodecSAI.h"
/*****************************************************************************************************
* Private Defines
*****************************************************************************************************/
#define FIFO_TX_WM          4       //SAI transmit FIFO watermark
#define FIFO_RX_WM          4       //SAI receive FIFO watermark

/*****************************************************************************************************
* void SAIInit(INT8U ssize)

*  PARAMETERS: sizecode:
*   0x00: 16-bit samples
*   0x01: 20-bit samples
*   0x10: 24-bit samples
*   0x11: 32-bit samples

* Setup for the DA7212 CODEC on the FRDM-N947 board.
* 05/29/2024 Todd Morton
*****************************************************************************************************/
void SAIInit(INT8U ssize){

    INT8U sai_word_size;

    switch(ssize){
    case 16:
        sai_word_size = 15; //for 16-bit words
        break;
    case 20:
        sai_word_size = 19; //for 20-bit words
        break;
    case 24:
        sai_word_size = 23; //for 24-bit words
        break;
    case 32:
        sai_word_size = 31; //for 32-bit words
        break;
    default:
        sai_word_size = 31;
        break;
    }
    
	//Configure clocks
	SYSCON->AHBCLKCTRLSET[0] = SYSCON_AHBCLKCTRL0_PORT1(1)|SYSCON_AHBCLKCTRL0_PORT3(1);
	SYSCON->AHBCLKCTRLSET[2] = SYSCON_AHBCLKCTRL2_SAI1(1);
	//Set FC2 clock to pll0_div. 150MHz/3 = 50MHz
	SYSCON->SAI1CLKSEL = SYSCON_SAI1CLKSEL_SEL(1);			//PLL clk, 150MHz
	SYSCON->SAI1CLKDIV = SYSCON_SAI1CLKDIV_DIV(2); //div by 3, clear HALT, 50MHz
	//Configure ports
    PORT3->PCR[16]=PORT_PCR_MUX(10)|PORT_PCR_IBE(1);    //ties P3_16 to SAI1, BCLK, input
    PORT3->PCR[17]=PORT_PCR_MUX(10)|PORT_PCR_IBE(1);    //ties P3_17 to SAI1, WCLK, input
    PORT3->PCR[20]=PORT_PCR_MUX(10)|PORT_PCR_IBE(1);    //ties P3_20 to SAI1, TX, output
    PORT3->PCR[21]=PORT_PCR_MUX(10)|PORT_PCR_IBE(1);    //ties P0_31 to SAI1, RX, input
    PORT1->PCR[21]=PORT_PCR_MUX(10)|PORT_PCR_IBE(1);    //ties P1_21 to SAI1, MCLK, output

    // Provide a MCLK output for the DA7212 CODEC. In this case it is set to fMCLK = 50MHz.
    //MCLK Divider uses system clock as input and MCLK output is enabled.
    SAI1->MCR |= I2S_MCR_MOE_MASK;

	/****************************************************************************
	 * SAI Transmitter Initialization
	 ****************************************************************************/
	SAI1->TCR1 = I2S_TCR1_TFW(FIFO_TX_WM);

    // For using TX BCLK and FS CLK in synch mode, TX must be Asynch and Rx synch. BCLK generated externally
    SAI1->TCR2 = I2S_TCR2_BCP_MASK;          // BCP = drive on falling edge, sample on rising edge

	/*transmit data channel 1 enabled */
	SAI1->TCR3 = I2S_TCR3_WDFL(0)|I2S_TCR3_TCE(1);
//
	SAI1->TCR4 = I2S_TCR4_FRSZ(1)  |     // 2 (1+1) words in a frame
	             I2S_TCR4_SYWD(sai_word_size) |     // bits in a word
	             I2S_TCR4_MF_MASK  |     // MSB First
	             I2S_TCR4_FSE_MASK |     // one bit early
	             I2S_TCR4_FSP_MASK;      // frame active low

//	/*24-bit first and following words */
	SAI1->TCR5 = I2S_TCR5_WNW(sai_word_size) |      // word N width, 32 bits
	             I2S_TCR5_W0W(sai_word_size) |      // word 0 width, 32 bits
	             I2S_TCR5_FBT(31);       // left justify for q31 data type

	/* enable DMA transfer on FIFO WM and reset FIFO*/
	SAI1->TCSR = I2S_TCSR_FRDE_MASK|I2S_TCSR_FR_MASK;

    /****************************************************************************
     * SAI Receiver Initialization
     ****************************************************************************/
	SAI1->RCR1 = I2S_RCR1_RFW(FIFO_RX_WM);
	SAI1->RCR2 = I2S_RCR2_SYNC(1);   //Rx synch with Tx
	SAI1->RCR3 = I2S_RCR3_RCE(1);

    SAI1->RCR4 = I2S_RCR4_FRSZ(1)  |     // 2 words in a frame
                 I2S_RCR4_SYWD(sai_word_size) |
                 I2S_RCR4_MF_MASK  |     // MSB first
                 I2S_RCR4_FSE_MASK |     // one bit early
                 I2S_RCR4_FSP_MASK;      // frame active low

    SAI1->RCR5 = I2S_RCR5_WNW(sai_word_size) |      // word N width, 32 bits
                 I2S_RCR5_W0W(sai_word_size) |      // word 0 width, 32 bits
                 I2S_RCR5_FBT(31);       // left justify for q31 data type

	SAI1->RCSR = I2S_RCSR_FRDE_MASK|I2S_RCSR_FR_MASK;
}
/*****************************************************************************************************
* void SAIWordSizeSet(INT8U ssize)

* Set SAI word size.
* 05/21/2024 Todd Morton
*****************************************************************************************************/

void SAIWordSizeSet(INT8U ssize){
    INT8U sai_word_size;

    switch(ssize){
    case 16:
        sai_word_size = 15; //for 16-bit words
        break;
    case 20:
        sai_word_size = 19; //for 20-bit words
        break;
    case 24:
        sai_word_size = 23; //for 24-bit words
        break;
    case 32:
        sai_word_size = 31; //for 32-bit words
        break;
    default:
        sai_word_size = 31;
        break;
    }

    SAI_RX_DISABLE();
    SAI_TX_DISABLE();

    SAI1->TCR4 = ((SAI1->TCR4 & ~I2S_TCR4_SYWD_MASK)|I2S_TCR4_SYWD(sai_word_size));

    SAI1->TCR5 = (SAI1->TCR5 & ~(I2S_TCR5_WNW_MASK|I2S_TCR5_W0W_MASK))|(I2S_TCR5_WNW(sai_word_size) |        // word N width, 32 bits
                  I2S_TCR5_W0W(sai_word_size));         // word 0 width, 32 bits

    SAI1->RCR4 = (SAI1->RCR4 & ~I2S_RCR4_SYWD_MASK)|I2S_RCR4_SYWD(sai_word_size);

    SAI1->RCR5 = (SAI1->RCR5 & ~(I2S_RCR5_WNW_MASK|I2S_RCR5_W0W_MASK))|I2S_RCR5_WNW(sai_word_size) |        // word N width, 32 bits
                  I2S_RCR5_W0W(sai_word_size);
    SAI_TX_ENABLE();
    SAI_RX_ENABLE();

}
/*****************************************************************************************************
* void SAISuspend()

* Suspend DMA requests from SAI. The source (CODEC) should be disabled first.
*
* 08/28/2024 Todd Morton
*****************************************************************************************************/
void SAISuspend(void){
	/*disable DMA transfer requests*/
	SAI1->TCSR &= ~I2S_TCSR_FRDE_MASK;
	SAI1->RCSR &= ~I2S_RCSR_FRDE_MASK;
}

/*****************************************************************************************************
* void SAIResume()

* Resume DMA requests from SAI and reset FIFO
*
* 08/28/2024 Todd Morton
*****************************************************************************************************/
void SAIResume(void){
	/*resume DMA transfer requests*/
	SAI1->TCSR |= I2S_TCSR_FRDE_MASK|I2S_TCSR_FR_MASK;
	SAI1->RCSR |= I2S_RCSR_FRDE_MASK|I2S_RCSR_FR_MASK;
}

