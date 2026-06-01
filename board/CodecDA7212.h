/*******************************************************************************/
/* CODEC_DA7212.h - Project Header file for DA7212 CODEC control
* Todd Morton 01/11/2024
*******************************************************************************
 * Public Function Prototypes
*******************************************************************************/
void CODECInit(void);
INT8U CODECReadRegister(INT8U raddr);
void CODECWriteRegister(INT8U raddr, INT8U rval);
void CODECSampleRateSet(INT32U srate);
INT32U CODECSampleRateGet(void);
INT8U CODECSampleSizeGet(void);
void CODECSampleSizeSet(INT8U ssize);
void CODECEnable(void);
void CODECDisable(void);
