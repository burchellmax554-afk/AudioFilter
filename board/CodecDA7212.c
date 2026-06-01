/*****************************************************************************************
 * CodecDA7212.c - Module for configuration and control of
 * Dialog DA7212 Codec. Initial sample rate of 48000sps and sample size of 32 bits is hard coded.
 * Requires I2C and SAI module for communication.
 * Todo: 1) Reorg by moving defines to *.h and following fsl_dialog7212.c names. Only if
 *          it is needed to make these register names public.
 *       2) Get block loading working.
 *
 * Todd Morton, 08/29/2024
 *****************************************************************************************
* Master Include File
*****************************************************************************************/
#include "MCUType.h"
#include "CodecDA7212.h"
#include "DA7212_LPI2C.h"
#include "FRDM_MCXN947_GPIO.h"

#define REG_STATUS1               0x02
#define REG_PLL_STATUS            0x03
#define REG_AUX_L_GAIN_STATUS     0x04
#define REG_AUX_R_GAIN_STATUS     0x05
#define REG_MIC_1_GAIN_STATUS     0x06
#define REG_MIC_2_GAIN_STATUS     0x07
#define REG_MIXIN_L_GAIN_STATUS   0x08
#define REG_MIXIN_R_GAIN_STATUS   0x09
#define REG_ADC_L_GAIN_STATUS     0x0a
#define REG_ADC_R_GAIN_STATUS     0x0b
#define REG_DAC_L_GAIN_STATUS     0x0c
#define REG_DAC_R_GAIN_STATUS     0x0d
#define REG_HP_L_GAIN_STATUS      0x0e
#define REG_HP_R_GAIN_STATUS      0x0f
#define REG_LINE_GAIN_STATUS      0x10
/*************************************/
#define REG_CIF_CTRL              0x1D
#define CIF_REG_SOFT_RESET        0x80
/*************************************/
#define REG_DIG_ROUTING_DAI       0x21
#define REG_SR                    0x22
/*************************************/
#define REG_REFERENCES            0x23
#define BIAS_EN                   0x80
/*************************************/
#define REG_PLL_FRAC_TOP          0x24
#define REG_PLL_FRAC_BOT          0x25
#define REG_PLL_INTEGER           0x26
/*************************************/
#define REG_PLL_CTRL              0x27
#define PLL_EN                    0x80
/*************************************/
#define REG_DAI_CLK_MODE          0x28
#define REG_DAI_CTRL              0x29
#define REG_DIG_ROUTING_DAC       0x2a
#define REG_ALC_CTRL1             0x2b
#define REG_AUX_L_GAIN            0x30
#define REG_AUX_R_GAIN            0x31
#define REG_MIXIN_L_SELECT        0x32
#define REG_MIXIN_R_SELECT        0x33
#define REG_MIXIN_L_GAIN          0x34
#define REG_MIXIN_R_GAIN          0x35
#define REG_ADC_L_GAIN            0x36
#define REG_ADC_R_GAIN            0x37
#define REG_ADC_FILTERS1          0x38
#define REG_MIC_1_GAIN            0x39
#define REG_MIC_2_GAIN            0x3a
#define REG_DAC_FILTERS5          0x40
#define REG_DAC_FILTERS2          0x41
#define REG_DAC_FILTERS3          0x42
#define REG_DAC_FILTERS4          0x43
#define REG_DAC_FILTERS1          0x44
#define REG_DAC_L_GAIN            0x45
#define REG_DAC_R_GAIN            0x46
#define REG_CP_CTRL               0x47
#define REG_HP_L_GAIN             0x48
#define REG_HP_R_GAIN             0x49
#define REG_LINE_GAIN             0x4a
#define REG_MIXOUT_L_SELECT       0x4b
#define REG_MIXOUT_R_SELECT       0x4c
#define REG_SYSTEM_MODES_INPUT    0x50
#define REG_SYSTEM_MODES_OUTPUT   0x51
#define REG_AUX_L_CTRL            0x60
#define REG_AUX_R_CTRL            0x61
#define REG_MICBIAS_CTRL          0x62
#define REG_MIC1_CTRL             0x63
#define REG_MIC2_CTRL             0x64
#define REG_MIXIN_L_CTRL          0x65
#define REG_MIXIN_R_CTRL          0x66
#define REG_ADC_L_CTRL            0x67
#define REG_ADC_R_CTRL            0x68
#define REG_DAC_L_CTRL            0x69
#define REG_DAC_R_CTRL            0x6a
#define REG_HP_L_CTRL             0x6b
#define REG_HP_R_CTRL             0x6c
#define REG_LINE_CTRL             0x6d
#define REG_MIXOUT_L_CTRL         0x6e
#define REG_MIXOUT_R_CTRL         0x6f
#define REG_MIXED_SAMPLE_MODE     0x84
#define REG_LDO_CTRL              0x90
#define REG_GAIN_RAMP_CTRL        0x92
#define REG_MIC_CONFIG            0x93
#define REG_PC_COUNT              0x94
#define REG_CP_VOL_THRESHOLD1     0x95
#define REG_CP_DELAY              0x96
#define REG_CP_DETECTOR           0x97
#define REG_DAI_OFFSET            0x98
#define REG_DIG_CTRL              0x99
#define REG_ALC_CTRL2             0x9a
#define REG_ALC_CTRL3             0x9b
#define REG_ALC_NOISE             0x9c
#define REG_ALC_TARGET_MIN        0x9d
#define REG_ALC_TARGET_MAX        0x9e
#define REG_ALC_GAIN_LIMITS       0x9f
#define REG_ALC_ANA_GAIN_LIMITS   0xa0
#define REG_ALC_ANTICLIP_CTRL     0xa1
#define REG_ALC_ANTICLIP_LEVEL    0xa2
#define REG_ALC_OFFSET_AUTO_M_L   0xa3
#define REG_ALC_OFFSET_AUTO_U_L   0xa4
#define REG_ALC_OFFSET_MAN_M_L    0xa6
#define REG_ALC_OFFSET_MAN_U_L    0xa7
#define REG_ALC_OFFSET_AUTO_M_R   0xa8
#define REG_ALC_OFFSET_AUTO_U_R   0xa9
#define REG_ALC_OFFSET_MAN_M_R    0xab
#define REG_ALC_OFFSET_MAN_U_R    0xac
#define REG_ALC_CIC_OP_LVL_CTRL   0xad
#define REG_ALC_CIC_OP_LVL_DATA   0xae
#define REG_DAC_NG_SETUP_TIME     0xaf
#define REG_DAC_NG_OFF_THRESHOLD  0xb0
#define REG_DAC_NG_ON_THRESHOLD   0xb1
#define REG_DAC_NG_CTRL           0xb2
#define REG_TONE_GEN_CFG1         0xb4
#define REG_TONE_GEN_CFG2         0xb5
#define REG_TONE_GEN_CYCLES       0xb6
#define REG_TONE_GEN_FREQ1_L      0xb7
#define REG_TONE_GEN_FREQ1_U      0xb8
#define REG_TONE_GEN_FREQ2_L      0xb9
#define REG_TONE_GEN_FREQ2_U      0xba
#define REG_TONE_GEN_ON_PER       0xbb
#define REG_TONE_GEN_OFF_PER      0xbc
#define REG_SYSTEM_STATUS         0xe0
#define REG_SYSTEM_ACTIVE         0xfd
/********************************************************************
* Private Function Prototypes
********************************************************************/
static INT8U codecPLLset(INT32U samp_rate, INT32U sys_mclk);
static void codec_Dly500ns(void);
static void codec_DlyMs(INT8U ms);

/********************************************************************
* Function Definitions
*********************************************************************
* CODECInit(void) - Public
*  DESCRIPTION: Configures PORTB bit 10, Resets and configures I2C
*               and CODEC.
*
*  PARAMETERS: none.
*
*  RETURN: none.
********************************************************************/
void CODECInit(void){

    I2CSetTargetAddress(0x1A); //CODEC target address
    I2CInit();
    CODECWriteRegister(REG_CIF_CTRL, CIF_REG_SOFT_RESET);


    CODECWriteRegister(REG_SR,0x0b);	//48000Hz
    CODECWriteRegister(REG_REFERENCES, BIAS_EN);
    CODECWriteRegister(REG_LDO_CTRL,0x80);
    codec_DlyMs(40U);
    /*Input amp, ADC inputs */
    CODECWriteRegister(REG_MIXOUT_L_SELECT,0x08); //check these to make sure it is not passing through
    CODECWriteRegister(REG_MIXOUT_R_SELECT,0x08);
    CODECWriteRegister(REG_DAC_L_CTRL,0x50); //this is new
    CODECWriteRegister(REG_DAC_R_CTRL,0x50); //this is new
    CODECWriteRegister(REG_HP_L_CTRL,0xb9); //was 0x89
    CODECWriteRegister(REG_HP_R_CTRL,0xb9);
    CODECWriteRegister(REG_MIXOUT_L_CTRL,0xb8);
    CODECWriteRegister(REG_MIXOUT_R_CTRL,0xb8);
    CODECWriteRegister(REG_CP_VOL_THRESHOLD1, 0x32);
    CODECWriteRegister(REG_SYSTEM_STATUS, 0x00); //new
    CODECWriteRegister(REG_DAC_L_GAIN, 0x67); //new
    CODECWriteRegister(REG_DAC_R_GAIN, 0x67); //new
    CODECWriteRegister(REG_MIXIN_R_SELECT,0x01);
    CODECWriteRegister(REG_MIXIN_L_SELECT,0x01);
    CODECWriteRegister(REG_MIXIN_R_GAIN,0x03);//new
    CODECWriteRegister(REG_MIXIN_L_GAIN,0x03);
    CODECWriteRegister(REG_ADC_R_GAIN,0x6f);//new
    CODECWriteRegister(REG_ADC_L_GAIN,0x6f);
    CODECWriteRegister(REG_AUX_R_CTRL,0xb0); //was 0x84
    CODECWriteRegister(REG_AUX_L_CTRL,0xb0);
    CODECWriteRegister(REG_MIXIN_R_CTRL,0x88);//new
    CODECWriteRegister(REG_MIXIN_L_CTRL,0x88);
    CODECWriteRegister(REG_ADC_R_CTRL,0x50);//new
    CODECWriteRegister(REG_ADC_L_CTRL,0x50);

    CODECWriteRegister(REG_SYSTEM_MODES_INPUT,0xf0);
    /*HP out, DAC outputs, reduce clicks */
    CODECWriteRegister(REG_SYSTEM_MODES_OUTPUT,0xf7);

    (void)codecPLLset(48000,50000000);
    /*Clock mode and PLL: master mode, 64 BCLK per WCLK */
    CODECWriteRegister(REG_DAI_CLK_MODE,0x81);
    /*DAI Configuration: enable DAI,I2S mode, 32-bits per channel*/
    CODECWriteRegister(REG_DAI_CTRL,0xcc);
    CODECWriteRegister(REG_DIG_ROUTING_DAC,0x32); //reset value
    /* Enable charge pump with default settings, was 0xb1 */
    CODECWriteRegister(REG_CP_CTRL,0xb1);

}
/********************************************************************
* INT8U codecPLLset(INT32U samp_rate, INT32U sys_mclk) - Private
*
*  PARAMETERS: sys_mclk - 5MHz-54MHz in units of Hz
*              samp_rate - Desired Sample rate - 8000, 12000, 16000,
*                          24000, 32000, 48000, 96000, 11025, 22050,
*                          44100, 88200.
*  RETURN: 1 if error
********************************************************************/
INT8U codecPLLset(INT32U samp_rate, INT32U sys_mclk) {
    INT8U indiv_bits;
    INT8U indiv;
    INT8U pll_err;
    INT32U pll_out_freq;
    INT32U pll_div;
    INT8U pll_ctrl = 0x00;
    INT16U pll_frac;
    INT8U pll_frac_top;
    INT8U pll_frac_bot;
    INT8U pll_integer;
    INT8U pll_stat;

    /* Read PLL configuration and clear INDIV bits */
    pll_ctrl = CODECReadRegister(REG_PLL_CTRL);


    /* Workout input divider based on MCLK rate
     * Does not implement 32k mode
     * */
    if (sys_mclk < 2000000) {
        pll_err = 1;
    } else if ((2000000 <= sys_mclk) && (sys_mclk < 10000000)) {
        indiv_bits = 0x00;
        indiv = 2;
    } else if ((10000000 <= sys_mclk) && (sys_mclk < 20000000)) {
        indiv_bits = 0x01;
        indiv = 4;
    } else if ((20000000 <= sys_mclk) && (sys_mclk < 40000000)) {
        indiv_bits = 0x02;
        indiv = 8;
    } else if ((40000000 <= sys_mclk) && (sys_mclk <= 50000000)) {
        indiv_bits = 0x03;
        indiv = 16;
    } else {
        pll_err = 1;
    }

    switch (samp_rate) {
    case 8000:
    	pll_out_freq = 98304000;
        break;
    case 12000:
    	pll_out_freq = 98304000;
        break;
    case 16000:
    	pll_out_freq = 98304000;
        break;
    case 24000:
    	pll_out_freq = 98304000;
        break;
    case 32000:
    	pll_out_freq = 98304000;
        break;
    case 48000:
    	pll_out_freq = 98304000;
        break;
    case 96000:
    	pll_out_freq = 98304000;
        break;
    case 22050:
    	pll_out_freq = 90317000;
        break;
    case 44100:
    	pll_out_freq = 90317000;
        break;
    case 88200:
    	pll_out_freq = 90317000;
        break;
    default:
        pll_err = 1;;
    }

    /* PLL feedback divider is a Q13 value */
    pll_div = (INT32U)(((INT64U)(pll_out_freq * indiv) << 13U) / sys_mclk);

    /* extract integer and fractional */
    pll_integer    = (INT8U)(pll_div >> 13U);
    pll_frac = (INT16U)(pll_div - ((INT32U)pll_integer << 13U));
    pll_frac_top    = (INT8U)(pll_frac >> 8U);
    pll_frac_bot = (INT8U)(pll_frac & 0xFFU);


    /* Write PLL dividers */
    CODECWriteRegister(REG_PLL_FRAC_TOP, pll_frac_top);
    CODECWriteRegister(REG_PLL_FRAC_BOT, pll_frac_bot);
    CODECWriteRegister(REG_PLL_INTEGER, pll_integer);

    /* Enable PLL */
    pll_ctrl = (INT8U)(pll_ctrl & ~(0x3 << 2));
    pll_ctrl |=  (PLL_EN | (indiv_bits<<2));
    CODECWriteRegister(REG_PLL_CTRL, pll_ctrl);

    /* wait for PLL lock bits */
    do {
		pll_stat = CODECReadRegister(REG_PLL_STATUS);
    }while((pll_stat & 1U) == 0U);


    return pll_err;
}
/********************************************************************
** lcdDly500ns(void)
*   Delays, at least, 500ns
*   Designed for 150MHz max clock.
 *  Tdly >= (54ns)i (at 150MHz)
 * Currently set to ~540ns with i=10.
********************************************************************/
static void codec_Dly500ns(void){
    INT32U i;
    for(i=0;i<10;i++){
    }
}

/********************************************************************
** lcdDlyms(INT8U ms)
*   Delays, at least, ms milliseconds. Maximum ~255ms.
*   Note, based on lcdDly500ns() so not very accurate but always
*   greater than ms milliseconds
********************************************************************/
static void codec_DlyMs(INT8U ms){
    INT32U cnt;
    for(cnt=ms*2000;cnt>0;cnt--){
        codec_Dly500ns();
    }
}

/********************************************************************
* CODECReadRegister() - Public
*  DESCRIPTION: Reads a CODEC register
*
*  PARAMETERS: INT8U raddr - register address
*
*  RETURN: INT8U -  Contents of register
********************************************************************/
INT8U CODECReadRegister(INT8U raddr){
    INT8U reg_in;
    reg_in = I2CReadByte(raddr);
    return reg_in;
}


/********************************************************************
* CODECWriteRegister() - Public
*  DESCRIPTION: Reads a CODEC register
*
*  PARAMETERS: INT8U raddr - register address
*              INT8U rval - value to write to register
*
********************************************************************/
void CODECWriteRegister(INT8U raddr, INT8U rval){

    I2CWriteByte(raddr, rval);

}

/*********************************************************************
* CODECSampleRateSet(INT32U srate) - Public
*
*  PARAMETERS: srate: 96kHz, 88.2kHz, 48kHz, 44.1kHz, 32kHz, 24kHz,
*                     22.05kHz, 16kHz, 12kHz, 8kHz

*  RETURN: none
*  DESCRIPTION: Sets ADC and DAC sample rates.
*  TDM, 01/15/2024
********************************************************************/
void CODECSampleRateSet(INT32U srate){
	INT8U ratecode;
	switch(srate){
	case 96000:
		ratecode = 0x0f;
		break;
	case 88200:
		ratecode = 0x0e;
		break;
	case 48000:
		ratecode = 0x0b;
		break;
	case 44100:
		ratecode = 0x0a;
		break;
	case 32000:
		ratecode = 0x09;
		break;
	case 24000:
		ratecode = 0x07;
		break;
	case 22050:
		ratecode = 0x06;
		break;
	case 16000:
		ratecode = 0x05;
		break;
	case 12000:
		ratecode = 0x03;
		break;
	case 8000:
		ratecode = 0x01;
		break;
	default:
		ratecode = 0x0b;
		break;

	}
	codecPLLset(srate, 50000000);
    CODECWriteRegister(REG_SR, ratecode);
}

/*********************************************************************
* CODECSampleRateGet(void) - Public
*
*  PARAMETERS: srate: 96kHz, 88.2kHz, 48kHz, 44.1kHz, 32kHz, 24kHz,
*                     22.05kHz, 16kHz, 12kHz, 8kHz

*  RETURN: current sample rate
*  DESCRIPTION: Gets ADC and DAC sample rates.
*  TDM, 01/15/2024
********************************************************************/
INT32U CODECSampleRateGet(void){
	INT8U ratecode;
	INT32U srate;
    ratecode = CODECReadRegister(REG_SR);

	switch(ratecode){
	case 0x0f:
		srate = 96000;
		break;
	case 0x0e:
		srate = 88200;
		break;
	case 0x0b:
		srate = 48000;
		break;
	case 0x0a:
		srate = 44100;
		break;
	case 0x09:
		srate = 32000;
		break;
	case 0x07:
		srate = 24000;
		break;
	case 0x06:
		srate = 22050;
		break;
	case 0x05:
		srate = 16000;
		break;
	case 0x03:
		srate = 12000;
		break;
	case 0x01:
		srate = 8000;
		break;
	default:
		srate = 0x00;
		break;

	}
	return srate;
}

/*********************************************************************
* CODECSampleSizeSet(INT8U ssize) - Public
*
*  PARAMETERS: rateCode - 0x0 to 0x3 correstponds to:
*   00: 16-bit samples
*   01: 20-bit samples
*   10: 24-bit samples
*   11: 32-bit samples note: actually 24-bits, zero extended.
*
*  RETURN:  none
*  DESCRIPTION: Sets ADC and DAC sample sizes.
*  Notes:
*      1. The 32-bit setting is exactly the same as the 24-bit setting.
*         This is because the internal word size of the CODEC is 24-bits.
*      2. This just changes the significant bits of the samples. The
*         actual DAI word size is set by DAI_BCLKS_PER_WCLK in DAI_CLK_MODE.
********************************************************************/
void CODECSampleSizeSet(INT8U ssize){
	INT8U sizecode;
	INT8U reg;
	switch(ssize){
	case 16:
		sizecode = 0x00;
		break;
	case 20:
		sizecode = 0x01;
		break;
	case 24:
		sizecode = 0x02;
		break;
	case 32:
		sizecode = 0x03;
		break;
	default:
		sizecode = 0x03;
		break;

	}
	//change DAI data size, must be read-modify-write.
	reg = CODECReadRegister(REG_DAI_CTRL);
	reg = (INT8U)(reg & ~0x0c); 						//clear size bits
    reg = reg | (sizecode<<2);				//set size bits
	CODECWriteRegister(REG_DAI_CTRL, reg);
}

/*********************************************************************
* CODECSampleSizeGet() - Public
*
*  PARAMETERS: rateCode - 0x0 to 0x3 correstponds to:
*   00: 16-bit samples
*   01: 20-bit samples
*   10: 24-bit samples
*   11: 32-bit samples
*
*  RETURN:  none
*  DESCRIPTION: Gets ADC and DAC sample sizes.
********************************************************************/
INT8U CODECSampleSizeGet(void){
	INT8U sizecode;
	INT8U ssize;

	sizecode = CODECReadRegister(REG_DAI_CTRL);
	sizecode = (sizecode & 0x0c)>>2;
	switch(sizecode){
	case 0x00:
		ssize = 16;
		break;
	case 0x01:
		ssize = 20;
		break;
	case 0x02:
		ssize = 24;
		break;
	case 0x03:
		ssize = 32;
		break;
	default:
		ssize = 0;
		break;

	}
	return ssize;
}


/*********************************************************************
* CODECEnable(void) - Public
*
*  PARAMETERS: none.
*
*  RETURN:  none.
*  DESCRIPTION: Turn on DAI.
*
********************************************************************/
void CODECEnable(void){
	INT8U reg;
	reg = CODECReadRegister(REG_DAI_CTRL);
    reg = reg | 0x80;				//set enable bit
	CODECWriteRegister(REG_DAI_CTRL, reg);
}

/*********************************************************************
* CODECDisable(void) - Public
*
*  PARAMETERS: none.
*
*  RETURN:  none.
*  DESCRIPTION: Turn off DAI.
*
********************************************************************/
void CODECDisable(void){
	INT8U reg;
	reg = CODECReadRegister(REG_DAI_CTRL);
    reg = (INT8U)(reg & ~0x80);				//clear enable bit
	CODECWriteRegister(REG_DAI_CTRL, reg);
}

