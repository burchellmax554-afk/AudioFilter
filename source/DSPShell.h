/*
*********************************************************************************************************
*                                               uC/Shell
*                                            Shell utility
*
*                           (c) Copyright 2007-2013; Micrium, Inc.; Weston, FL
*
*                   All rights reserved.  Protected by international copyright laws.
*                   Knowledge of the source code may not be used to write a similar
*                   product.  This file may only be used in accordance with a license
*                   and should not be redistributed in any way.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                         GENERAL SHELL COMMANDS
*
* Filename      : sh_shell.h
* Version       : V1.03.01
* Programmer(s) : BAN
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                 MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               SH_SHELL present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  DSP_SHELL_PRESENT                                       /* See Note #1.                                         */
#define  DSP_SHELL_PRESENT


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_BOOLEAN  DSPShell_Init(void);



#endif                                                          /* End of SH_SHELL module include (see Note #1).        */
