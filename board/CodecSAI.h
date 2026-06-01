/*****************************************************************************************************
* Module definition against multiple inclusion
*****************************************************************************************************/
#ifndef SAI_DEF
#define SAI_DEF
/*****************************************************************************************************
* Include files
*****************************************************************************************************/
#include "MCUType.h"
/*****************************************************************************************************
* Definition of project wide MACROS / #DEFINE-CONSTANTS
*****************************************************************************************************/
#define SAI_TX_ENABLE()	    (SAI1->TCSR |= I2S_TCSR_TE_MASK)
#define SAI_TX_DISABLE()	(SAI1->TCSR &= ~I2S_TCSR_TE_MASK)

#define SAI_RX_ENABLE()	    (SAI1->RCSR |= I2S_RCSR_RE_MASK)
#define SAI_RX_DISABLE()	(SAI1->RCSR &= ~I2S_RCSR_RE_MASK)

/*****************************************************************************************************
* Declaration of project wide FUNCTIONS
*****************************************************************************************************/
void SAIInit(INT8U ssize);
void SAIWordSizeSet(INT8U ssize);
void SAISuspend(void);
void SAIResume(void);

#endif
