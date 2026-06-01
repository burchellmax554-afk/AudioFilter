/****************************************************************************************
 * DA7212_LPI2C.c - LPI2C module
 * **************************************************************************************
 * Public Function Prototypes
 ****************************************************************************************/
void I2CInit(void);
void I2CSetTargetAddress(INT8U address);
void I2CSendStart(void);
void I2CSendStop(void);
INT8U I2CSendBlock(INT8U *dataToSendPtr,INT8U size);
INT8U I2CSendByte(INT8U data);
void I2CWriteByte(INT8U reg, INT8U rval);
INT8U I2CReadByte(INT8U raddr);
/***************************************************************************************/
