/*********************************************************************************
 * DA7212_LPI2C.c - This I2C module is written for the LPI2C on Flexcomm 2
 * Connected to P4_0(SDA) and P4_1(SCL) for the DA7212 Codec Arduino board.
 * Configured for SCL of ~100kHz, 7-bit addressing.
 *
 * Todd Morton, 05/29/2024
***********************************************************************************
* Master Include File
*********************************************************************************/
#include "MCUType.h"
#include "DA7212_LPI2C.h"
/*********************************************************************************
* Defines
*********************************************************************************/
#define WR 0x00
#define RD 0x01
#define LPI2C_CMD_TX			0x00
#define LPI2C_CMD_RX			0x01
#define LPI2C_CMD_STOP			0x02
#define LPI2C_CMD_START_SEND	0x04

/*********************************************************************************
* Private Resources
*********************************************************************************/
static INT8U targetAddress = 0x00;
static INT8U i2cGetTargetAddress(void);
/*********************************************************************************
* Function Definitions
**********************************************************************************
* I2CInit(void) - Public
*
*  PARAMETERS: none.
*
*  DESCRIPTION: Initializes LPI2C, on Flexcomm 2 for Controller mode,
*               7-bit addressing, uses P4_0 and P4_1.
*               Assumes pll0 clock at 150MHz/3 = 50MHz
*
*********************************************************************************/
void I2CInit(void){
	//Configure clocks
	SYSCON->AHBCLKCTRLSET[0] = SYSCON_AHBCLKCTRL0_PORT4(1);
	SYSCON->AHBCLKCTRLSET[1] = SYSCON_AHBCLKCTRL1_FC2(1);
	//Set FC2 clock to pll0_div. 150MHz/3 = 50MHz
	SYSCON->FCCLKSEL[2] = SYSCON_FCCLKSEL_SEL(1);			//PLL div clk, 50MHz
	SYSCON->FLEXCOMMCLKDIV[2] = SYSCON_FLEXCOMMXCLKDIV_FLEXCOMMCLKDIV_DIV(0); //div by 1, clear HALT
	//Configure ports and set FC2 to LPI2C
    PORT4->PCR[0]=PORT_PCR_MUX(2)|PORT_PCR_IBE(1)|PORT_PCR_ODE(1);    //ties P4_0 to SDA, enable buffer
    PORT4->PCR[1]=PORT_PCR_MUX(2)|PORT_PCR_IBE(1)|PORT_PCR_ODE(1);    //ties P4_1 to SCL, enable buffer
    LP_FLEXCOMM2->PSELID = LP_FLEXCOMM_PSELID_PERSEL(3); //make flexcomm 2 a I2C
    //Software reset
    LPI2C2->MCR |= LPI2C_MCR_RST_MASK;
    LPI2C2->MCR &= ~LPI2C_MCR_RST_MASK;

    //set bit rate, clk widths, setup, and data valid times
    LPI2C2->MCFGR1 = LPI2C_MCFGR1_PRESCALE(2);	//div clk by 4
    LPI2C2->MCFGR2 = LPI2C_MCFGR2_BUSIDLE(250);	//bus idle timeout
	LPI2C2->MCCR0 = LPI2C_MCCR0_DATAVD(3)|LPI2C_MCCR0_SETHOLD(62)|LPI2C_MCCR0_CLKHI(62)|LPI2C_MCCR0_CLKLO(61);
	LPI2C2->MFCR = LPI2C_MFCR_TXWATER(2);
	//Reset FIFOs
	LPI2C2->MCR |= ((LPI2C_MCR_RRF_MASK)|(LPI2C_MCR_RTF_MASK));
	LPI2C2->MCR &= ~((LPI2C_MCR_RRF_MASK)|(LPI2C_MCR_RTF_MASK));
	//enable I2C2
    LPI2C2->MCR |= LPI2C_MCR_MEN_MASK;
}
/*********************************************************************
* I2CSetTargetAddress(INT8U address) - Public
*
*  PARAMETERS: address - address of target to be addressed.
*
*  DESCRIPTION: Sets target address variable.  This function must be
*   called once prior to communicating with a target.
*
********************************************************************/
void I2CSetTargetAddress(INT8U address){
    targetAddress = address;
}

/*********************************************************************
* INT8U i2cGetTargetAddress(void) - Public
*
*  PARAMETERS: none.
*
*  RETURN: INT8U - target address.
*
*  DESCRIPTION: Returns current target address.
*
********************************************************************/
static INT8U i2cGetTargetAddress(void){
    return targetAddress;
}
#if 0
/*********************************************************************
* INT8U I2CSendBlock(INT32U *dataToSendPtr,INT8U size) - Public
*
*  PARAMETERS: *dataToSendPtr - pointer to data to be sent.
*               size - number of bytes to be sent.
*  RETURN: error code.  returns 1 if transmit was successful, 0 if failed.
*
*  DESCRIPTION: Sends target address and a series of
*  8-bit bytes to the target.  User must have previously set target
*  address using I2CSetTargetAddress(). Function returns 0 if any
*  bytes are not acknowledged by target.  This is a blocking routine.
*  User is responsible for generating start and stop commands.
*
********************************************************************/
INT8U I2CSendBlock(INT8U *dataToSendPtr,INT8U size){
    INT8U i;
    while(!(I2C1->S & 0x80)){}   //wait for bus to be free.

    I2C1->D = (i2cGetTargetAddress()<<1); //send target address
    for(i=0;i<size;i++){
        while(!(I2C1->S & I2C_S_IICIF_MASK)); //wait for flag
        I2C1->S |= I2C_S_IICIF_MASK; // clear flag

        if(I2C1->S & I2C_S_RXAK_MASK){
            //RX NAK'd
            return 0;
        }
        I2C1->D = dataToSendPtr[i];  //send data byte
    }
    while(!(I2C1->S & I2C_S_IICIF_MASK)); //wait for flag
    I2C1->S |= I2C_S_IICIF_MASK; // clear flag
    if(I2C1->S & I2C_S_RXAK_MASK){
        //RX NAK'd
        return 0;
    }
    return 1;
}
#endif

/****************************************************************************************
* INT8U I2CReadByte(INT8U reg) - Public
*
*  PARAMETERS: INT8U reg - 8-bit register address to read.
*  RETURN: contents of register.
*
*  DESCRIPTION: Sends target address and 8-bit register address to the target. User must
*  have previously set target address using I2CSetTargetAddress().
*  Function returns the contents of the register.
*
****************************************************************************************/
INT8U I2CReadByte(INT8U reg){
    INT8U rval;
    //wait if bus busy from another device
//    while(((LPI2C2->MSR & LPI2C_MSR_BBF_MASK) != 0) && ((LPI2C2->MSR & LPI2C_MSR_MBF_MASK) == 0)){} //wait for bus not busy
    while((LPI2C2->MSR & LPI2C_MSR_TDF_MASK) == 0){} //wait for room in FIFO
    LPI2C2->MTDR = (INT32U)((LPI2C_CMD_START_SEND<<8)|(i2cGetTargetAddress()<<1)|WR);
    LPI2C2->MTDR = (LPI2C_CMD_TX<<8)|reg;
    LPI2C2->MTDR = (LPI2C_CMD_START_SEND<<8)|(i2cGetTargetAddress()<<1)|RD;
    LPI2C2->MTDR = (LPI2C_CMD_RX<<8)|0;         //one byte?
    LPI2C2->MTDR = (LPI2C_CMD_STOP<<8)|0;
    while((LPI2C2->MSR & LPI2C_MSR_RDF_MASK) == 0){} //wait for return in FIFO
    rval = (INT8U)LPI2C2->MRDR;
    return rval;
}

/****************************************************************************************
* void I2CWriteByte(INT8U reg, INT8U rval) - Public
*
*  PARAMETERS: INT8U reg - 8-bit register address to write.
*              INT8U rval - value to write to byte.
*
*  DESCRIPTION: Sends target address and 8-bit register address to the target. User must
*  have previously set target address using I2CSetTargetAddress().
*  Function then writes rval to the register, reg.
*
****************************************************************************************/
void I2CWriteByte(INT8U reg, INT8U rval){

    //wait if bus busy from another device
//    while(((LPI2C2->MSR & LPI2C_MSR_BBF_MASK) != 0) && ((LPI2C2->MSR & LPI2C_MSR_MBF_MASK) == 0)){} //wait for bus not busy
    while((LPI2C2->MSR & LPI2C_MSR_TDF_MASK) == 0){} //wait for room in FIFO

    LPI2C2->MTDR = (INT32U)((LPI2C_CMD_START_SEND<<8)|(i2cGetTargetAddress()<<1)|WR);
    LPI2C2->MTDR = (LPI2C_CMD_TX<<8)|reg;
    LPI2C2->MTDR = (LPI2C_CMD_TX<<8)|rval;
    LPI2C2->MTDR = (LPI2C_CMD_STOP<<8)|0;
}
