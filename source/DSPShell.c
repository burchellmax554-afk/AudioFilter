/*
*******************************************************************************************
* DSPShell sets up the DSP commands for uC-Shell.
*
*******************************************************************************************
* Filename      : DSPShell.c
* Programmer(s) : Todd Morton, 04/12/2017
* 				  Jennifer Ngo, 05/28/2017
* 				  Todd Morton, 04/04/2019 Add sample rate and sample size commands
*				  Todd Morton, 08/29/2024 Modify for 7212 CODEC
********************************************************************************************/
/********************************************************************************************
*                                   INCLUDE FILES
********************************************************************************************/
#include "MCUType.h"
#include "os.h"
#include "stdlib.h"
#include "lib_str.h"
#include "Terminal.h"
#include "AppDSP.h"
#include "DSPShell.h"
#include "CodecDA7212.h"
#include "BasicIO.h"

/*********************************************************************************************
*                                       LOCAL DEFINES
*********************************************************************************************/

/*********************************************************************************************
*                                 ARGUMENT ERROR MESSAGES
*********************************************************************************************/
const INT8C dspshCmdMsgNotRec[] = {"Command not recognized: "};
const INT8C dspshCmdMsgRateNotRec[] = {"Rate not recognized. Allowable rates:\n\r96000,88200,48000,44100,32000,24000,22050,16000,12000,8000\n\r"};
const INT8C dspshCmdMsgSizeNotRec[] = {"Size not recognized. Allowable sizes: 24,20,16"};
const INT8C dspshCmdMsgCRdUsage[] = {"Usage: dsp_codec_rd reg (reg in hex)\n\r"};
const INT8C dspshCmdMsgCWrUsage[] = {"Usage: dsp_codec_wr reg value (reg and value in hex)\n\r"};
const INT8C dspshCmdMsgLoadUsage[] = {"Usage: dsp_load buffer\n\r where buffer is l_in, r_in, l_out, r_out\n\r"};

/*********************************************************************************************
*                               COMMAND EXPLANATION MESSAGES
*********************************************************************************************/
const INT8C dspshCmdMsgNL[] = {"\n\r"};
const INT8C dspshCmdMsgListFs[] = {"dsp_fs - display or set sample rate\n\r"};
const INT8C dspshCmdMsgListN[] = {"dsp_n - display or set sample size in bits\n\r"};
const INT8C dspshCmdMsgListCRd[] = {"dsp_codec_rd - display the contents of a CODEC register\n\r"};
const INT8C dspshCmdMsgListCWr[] = {"dsp_codec_wr - write to a CODEC register\n\r"};
const INT8C dspshCmdMsgListLoad[] = {"dsp_load - load the contents of a buffer\n\r"};

/*********************************************************************************************
*                                      LOCAL CONSTANTS
*********************************************************************************************/

/*********************************************************************************************
*                                     LOCAL DATA TYPES
*********************************************************************************************/

/*********************************************************************************************
*                                   LOCAL GLOBAL VARIABLES
*********************************************************************************************/

/*********************************************************************************************
*                                 LOCAL FUNCTION PROTOTYPES
*********************************************************************************************/

static  CPU_INT16S  DSPList(CPU_INT16U        argc,
                            CPU_CHAR         *argv[],
                            SHELL_OUT_FNCT    out_fnct,
                            SHELL_CMD_PARAM  *pcmd_param);

static CPU_INT16S dspShellSampleRate(CPU_INT16U argc,
                                     CPU_CHAR *argv[],
                                     SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param);

static CPU_INT16S dspShellSampleSize(CPU_INT16U argc,
                                     CPU_CHAR *argv[],
                                     SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param);

static CPU_INT16S dspshCodecRegRead(CPU_INT16U argc,
                                     CPU_CHAR *argv[],
                                     SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param);

static CPU_INT16S dspshCodecRegWrite(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param);

static CPU_INT16S dspshBufferLoad(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param);

static  SHELL_CMD  dspshCmdTbl[] = {
        {"dsp_help", DSPList},
		{"dsp_codec_rd", dspshCodecRegRead},{"dsp_codec_wr", dspshCodecRegWrite},
		{"dsp_fs", dspShellSampleRate},{"dsp_n", dspShellSampleSize},
		{"dsp_load", dspshBufferLoad},
		{0,         0           }
};

/*********************************************************************************************
* DSPShell_Init()
*
* Description : Initialize Shell, Terminal, sh_shell and DSPShell for DSP shell commands.
* Todd Morton, 04/12/2017
*********************************************************************************************/

CPU_BOOLEAN DSPShell_Init(void){
    SHELL_ERR    err;
    CPU_BOOLEAN  ok;

    Shell_Init();
    Terminal_Init();
    Shell_CmdTblAdd((CPU_CHAR *)"dsp", dspshCmdTbl, &err);

    ok = (err == SHELL_ERR_NONE) ? DEF_OK : DEF_FAIL;
    return (ok);
}

/*********************************************************************************************
********************************************************************************************
*                                    COMMAND FUNCTIONS
*******************************************************************************************
********************************************************************************************/

/*********************************************************************************************
*                                    DSPList()
*
* Description : List all of the DSP parameters
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
* Jennifer Ngo, 05/29/2017
*********************************************************************************************/

static CPU_INT16S DSPList(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                          SHELL_CMD_PARAM *pcmd_param) {

    switch (argc) {
        case 1:
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgListFs,sizeof(dspshCmdMsgListFs),pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgListN,sizeof(dspshCmdMsgListN),pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgListCRd,sizeof(dspshCmdMsgListCRd),pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgListCWr,sizeof(dspshCmdMsgListCWr),pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgListLoad,sizeof(dspshCmdMsgListLoad),pcmd_param->pout_opt);
             break;
        case 2:
        default:
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNotRec, sizeof(dspshCmdMsgNotRec), pcmd_param->pout_opt);
             (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
             break;
    }
    return (SHELL_ERR_NONE);
}
/*********************************************************************************************
*                                    dspShellSampleRate()
*
* Description : configures the sample rate for DA7212 CODEC.
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
*********************************************************************************************/

static CPU_INT16S dspShellSampleRate(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param) {
    CPU_CHAR *param1;
    INT32U srate;
    CPU_CHAR srate_strg[6];
    INT8U rate_err = 0;

    switch (argc) {
        case 1:         //if no argument return current sample rate
            srate = DSPSampleRateGet();
            (void)Str_FmtNbr_Int32U ((INT32U)srate, 5, DEF_NBR_BASE_DEC,'\0', DEF_YES, DEF_YES, srate_strg);
            (void)out_fnct((CPU_CHAR *)srate_strg, 6, pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            break;
        case 2:
            param1 = (CPU_CHAR *)argv[1];

            if(!Str_Cmp(param1,"96000")){
                DSPSampleRateSet(96000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"88200")){
                DSPSampleRateSet(88200);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"48000")){
                DSPSampleRateSet(48000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"44100")){
                DSPSampleRateSet(44100);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"32000")){
                DSPSampleRateSet(32000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"24000")){
                DSPSampleRateSet(24000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"22050")){
                DSPSampleRateSet(22050);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"16000")){
                DSPSampleRateSet(16000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"12000")){
                DSPSampleRateSet(12000);
                srate = DSPSampleRateGet();
            }else if(!Str_Cmp(param1,"8000")){
                DSPSampleRateSet(8000);
                srate = DSPSampleRateGet();
            }else{
                rate_err = 1;
            }
            if(rate_err == 1){
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgRateNotRec, sizeof(dspshCmdMsgRateNotRec), pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            }else{
                (void)Str_FmtNbr_Int32U ((INT32U)srate, 5, DEF_NBR_BASE_DEC,'\0', DEF_YES, DEF_YES, srate_strg);
                (void)out_fnct((CPU_CHAR *)srate_strg, 6, pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            }
            break;
        default:
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNotRec, sizeof(dspshCmdMsgNotRec), pcmd_param->pout_opt);
             (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
             break;
    }
    return (SHELL_ERR_NONE);
}


/*********************************************************************************************
*                                    dspShellSampleSize()
*
* Description : configures the parameters.
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
*********************************************************************************************/

static CPU_INT16S dspShellSampleSize(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param) {
    CPU_CHAR *param1;
    INT16U ssize;
    CPU_CHAR ssize_strg[6];
    INT8U size_err = 0;

    switch (argc) {
        case 1:         //if no argument return current sample size
            ssize = DSPSampleSizeGet();
            (void)Str_FmtNbr_Int32U ((INT32U)ssize, 5, DEF_NBR_BASE_DEC,'\0', DEF_YES, DEF_YES, ssize_strg);
            (void)out_fnct((CPU_CHAR *)ssize_strg, 6, pcmd_param->pout_opt);
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            break;
        case 2:
            param1 = (CPU_CHAR *)argv[1];

            if(!Str_Cmp(param1,"16")){
                DSPSampleSizeSet(16);
                ssize = DSPSampleSizeGet();
            }else if(!Str_Cmp(param1,"20")){
                DSPSampleSizeSet(20);
                ssize = DSPSampleSizeGet();
            }else if(!Str_Cmp(param1,"24")){
                DSPSampleSizeSet(24);
                ssize = DSPSampleSizeGet();
            }else{
                size_err = 1;
            }
            if(size_err == 1){
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgSizeNotRec, sizeof(dspshCmdMsgSizeNotRec), pcmd_param->pout_opt);
                (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            }else{
                (void)Str_FmtNbr_Int32U ((INT32U)ssize, 5, DEF_NBR_BASE_DEC,'\0', DEF_YES, DEF_YES, ssize_strg);
                (void)out_fnct((CPU_CHAR *)ssize_strg, sizeof(ssize_strg), pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            }
            break;
        default:
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNotRec, sizeof(dspshCmdMsgNotRec), pcmd_param->pout_opt);
             (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
             break;
    }
    return (SHELL_ERR_NONE);
}

/*********************************************************************************************
*                                    dspshCodecRead()
*
* Description : configures the parameters.
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
*********************************************************************************************/

static CPU_INT16S dspshCodecRegRead(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param) {
    INT8U c_reg = 0;
    INT8U reg_val;
    INT8C reg_val_strg[4];

    switch (argc) {
        case 1:         //if no argument return usage
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgCRdUsage, sizeof(dspshCmdMsgCRdUsage), pcmd_param->pout_opt);
            break;
        case 2:
            c_reg = (INT8U)Str_ParseNbr_Int32U(argv[1],(void *)0,16);
            reg_val = CODECReadRegister(c_reg);

			(void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
			(void)out_fnct((CPU_CHAR *)" ",2,pcmd_param->pout_opt);
			(void)Str_FmtNbr_Int32U ((INT8U)reg_val, 3, DEF_NBR_BASE_HEX,'\0', DEF_YES, DEF_YES,reg_val_strg);
			(void)out_fnct(reg_val_strg, (CPU_INT16U)Str_Len(reg_val_strg), pcmd_param->pout_opt);
			(void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            break;
        default:
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgCRdUsage, sizeof(dspshCmdMsgCRdUsage), pcmd_param->pout_opt);
            break;
    }
    return (SHELL_ERR_NONE);
}
/*********************************************************************************************
*                                    dspshCodecWrite()
*
* Description : configures the parameters.
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
*********************************************************************************************/

static CPU_INT16S dspshCodecRegWrite(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param) {
    INT8U c_reg = 0;
    INT32U reg_val;
    INT8C reg_val_strg[4];

    switch (argc) {
        case 1:         //if no argument return usage
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgCWrUsage, sizeof(dspshCmdMsgCWrUsage), pcmd_param->pout_opt);
            break;
        case 2:         //if no argument return usage
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgCWrUsage, sizeof(dspshCmdMsgCWrUsage), pcmd_param->pout_opt);
            break;
        case 3:
            c_reg = (INT8U)Str_ParseNbr_Int32U(argv[1],(void *)0,16);
            reg_val = (INT8U)Str_ParseNbr_Int32U(argv[2],(void *)0,16);

			(void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
			(void)out_fnct((CPU_CHAR *)" ",2,pcmd_param->pout_opt);
			(void)out_fnct(argv[2], (CPU_INT16U)Str_Len(argv[2]), pcmd_param->pout_opt);
			CODECWriteRegister(c_reg, (INT8U)reg_val);
			reg_val = CODECReadRegister(c_reg);     /*print out actual value after write */
			(void)Str_FmtNbr_Int32U ((INT8U)reg_val, 2, DEF_NBR_BASE_HEX,'0', DEF_YES, DEF_YES,reg_val_strg);
			(void)out_fnct((CPU_CHAR *)" ",2,pcmd_param->pout_opt);
			(void)out_fnct(reg_val_strg, (CPU_INT16U)Str_Len(reg_val_strg), pcmd_param->pout_opt);
			(void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            break;
        default:
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgCRdUsage, sizeof(dspshCmdMsgCRdUsage), pcmd_param->pout_opt);
            break;
        }
            return (SHELL_ERR_NONE);
}
/*********************************************************************************************
*                                    dspshBufferLoad()
*
* Description : Sends contents of buffer to the terminal (load).
*
* Argument(s) : argc            The number of arguments.
*
*               argv            Array of arguments.
*
*               out_fnct        The output function.
*
*               pcmd_param      Pointer to the command parameters.
*
* Return(s)   : SHELL_EXEC_ERR, if an error is encountered.
*               SHELL_ERR_NONE, otherwise.
*
* Caller(s)   : Shell, in response to command execution.
*
* Note(s)     : none.
*********************************************************************************************/

static CPU_INT16S dspshBufferLoad(CPU_INT16U argc, CPU_CHAR *argv[], SHELL_OUT_FNCT out_fnct,
                                     SHELL_CMD_PARAM *pcmd_param) {
    CPU_CHAR *param1;
    INT8U load_err = 0;
    OS_ERR os_err;
    INT32S *buffer_ptr;
    INT32U i;
    CPU_CHAR sample_strg[14] = {"0000000000000"};
    FP32 sample_float;

    switch (argc) {
        case 1:         //if no argument return usage
            (void)out_fnct((CPU_CHAR *)dspshCmdMsgLoadUsage, sizeof(dspshCmdMsgCWrUsage), pcmd_param->pout_opt);
            break;
        case 2:
            param1 = (CPU_CHAR *)argv[1];

            if(!Str_Cmp(param1,"l_in")){
                buffer_ptr = DSPBufferGet(LEFT_IN);
            }else if(!Str_Cmp(param1,"r_in")){
                buffer_ptr = DSPBufferGet(RIGHT_IN);
            }else if(!Str_Cmp(param1,"l_out")){
                buffer_ptr = DSPBufferGet(LEFT_OUT);
            }else if(!Str_Cmp(param1,"r_out")){
                buffer_ptr = DSPBufferGet(RIGHT_OUT);
            }else{
                load_err = 1;
            }
            if(load_err == 1){
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNotRec, sizeof(dspshCmdMsgNotRec), pcmd_param->pout_opt);
                (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
            }else{
                DSPSuspendReqSet();
                DSPSuspendedPend(0,&os_err); //wait for dsp stream to be suspended so it is safe to read buffer

                (void)out_fnct((CPU_CHAR *)"[",2,pcmd_param->pout_opt); //open with '[' for MATLAB
                for(i=0;i < (DSP_NUM_BLOCKS*DSP_SAMPLES_PER_BLOCK - 1);i++){
                    sample_float = ((FP32)*buffer_ptr++)/2147483648;    //convert from q_31 to float
                    (void)Str_FmtNbr_32(sample_float,2,9,'\0',DEF_YES, sample_strg);
                    (void)out_fnct((CPU_CHAR *)sample_strg, sizeof(sample_strg), pcmd_param->pout_opt);
                    (void)out_fnct((CPU_CHAR *)",",2,pcmd_param->pout_opt); //comma seperated
                }
                sample_float = ((FP32)*buffer_ptr)/2147483648;          //last sample and closing ']'
                (void)Str_FmtNbr_32(sample_float,2,9,'\0',DEF_YES, sample_strg);
                (void)out_fnct((CPU_CHAR *)sample_strg, sizeof(sample_strg), pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)"]",2,pcmd_param->pout_opt);
                (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
                DSPResume();
            }
            break;
        default:
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNotRec, sizeof(dspshCmdMsgNotRec), pcmd_param->pout_opt);
             (void)out_fnct(argv[1], (CPU_INT16U)Str_Len(argv[1]), pcmd_param->pout_opt);
             (void)out_fnct((CPU_CHAR *)dspshCmdMsgNL,sizeof(dspshCmdMsgNL),pcmd_param->pout_opt);
             break;
    }
    return (SHELL_ERR_NONE);
}
