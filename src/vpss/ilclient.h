/*
 *  Copyright (c) 2010-2011, Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  Contact information for paper mail:
 *  Texas Instruments
 *  Post Office Box 655303
 *  Dallas, Texas 75265
 *  Contact information:
 *  http://www-k.ext.ti.com/sc/technical-support/product-information-centers.htm?
 *  DCMP=TIHomeTracking&HQS=Other+OT+home_d_contact
 *  ============================================================================
 *
 */

#ifndef __OMX_ILCLIENT_H__
#define __OMX_ILCLIENT_H__

/******************************************************************************\
 *      Includes
\******************************************************************************/
#include <stdint.h>
#include "semp.h"
#include <vpss/es_parser.h>
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Component.h"

/******************************************************************************/

#ifdef __cplusplus              /* required for headers that might */
extern "C"
{                               /* be compiled by a C++ compiler */
#endif

#define IL_CLIENT_MAX_NUM_IN_BUFS 8
#define IL_CLIENT_MAX_NUM_OUT_BUFS 32

#define H264_PADX    (32)
#define H264_PADY    (24)

#define OMX_TEST_INIT_STRUCT_PTR(_s_, _name_)                                  \
          memset((_s_), 0x0, sizeof(_name_));                                  \
          (_s_)->nSize = sizeof(_name_);                                       \
          (_s_)->nVersion.s.nVersionMajor = 0x1;                               \
          (_s_)->nVersion.s.nVersionMinor = 0x1;                               \
          (_s_)->nVersion.s.nRevision  = 0x0;                                  \
          (_s_)->nVersion.s.nStep   = 0x0;

#define OMX_INIT_PARAM(param)                                                  \
        {                                                                      \
          memset ((param), 0, sizeof (*(param)));                              \
          (param)->nSize = sizeof (*(param));                                  \
          (param)->nVersion.s.nVersionMajor = 1;                               \
          (param)->nVersion.s.nVersionMinor = 1;                               \
        }

#define ERROR(...)                                                             \
          if (1)                                                               \
          {                                                                    \
            fprintf(stderr, "ERROR: %s: %d: ", __FILE__, __LINE__);            \
            fprintf(stderr, __VA_ARGS__);                                      \
            fprintf(stderr, "\n");                                             \
            exit (1);                                                          \
          }

/** Number of input buffers in the H264 Decoder IL Client */
#define NUM_OF_IN_BUFFERS 4

/** Number of output buffers in the H264 Decoder IL Client */
#define NUM_OF_OUT_BUFFERS 8

typedef struct IL_CLIENT_COMP_PRIVATE_T IL_CLIENT_COMP_PRIVATE;
typedef struct IL_CLIENT_GFX_PRIVATE_T IL_CLIENT_GFX_PRIVATE;

typedef struct IL_CLIENT_SNT_CONNECT_T
{
  OMX_U32 remotePort;
  OMX_S32 remotePipe[2];      /* for making ETB / FTB calls to connected comp 
                               */
  IL_CLIENT_COMP_PRIVATE *remoteClient;       /* For allocate / use buffer */
} IL_CLIENT_SNT_CONNECT;

typedef struct IL_CLIENT_INPORT_PARAMS_T
{
  IL_CLIENT_SNT_CONNECT connInfo;
  OMX_S32 ipBufPipe[2];       /* input pipe */
  OMX_BUFFERHEADERTYPE *pInBuff[IL_CLIENT_MAX_NUM_OUT_BUFS];
  OMX_U32 nBufferCountActual;
  OMX_U32 nBufferSize;
} IL_CLIENT_INPORT_PARAMS;

typedef struct IL_CLIENT_OUTPORT_PARAMS_T
{
  IL_CLIENT_SNT_CONNECT connInfo;
  OMX_S32 opBufPipe[2];       /* output pipe */
  OMX_BUFFERHEADERTYPE *pOutBuff[IL_CLIENT_MAX_NUM_OUT_BUFS];
  OMX_U32 nBufferCountActual;
  OMX_U32 nBufferSize;
} IL_CLIENT_OUTPORT_PARAMS;

typedef struct IL_CLIENT_COMP_PRIVATE_T
{
  IL_CLIENT_INPORT_PARAMS *inPortParams;
  IL_CLIENT_OUTPORT_PARAMS *outPortParams;    /* Common o/p port params */
  OMX_U32 numInport;
  OMX_U32 numOutport;
  OMX_U32 startOutportIndex;
  OMX_S32 localPipe[2];
  OMX_HANDLETYPE handle;
  pthread_t inDataStrmThrdId;
  pthread_t outDataStrmThrdId;
  pthread_t connDataStrmThrdId;
  semp_t *done_sem;
  semp_t *eos;
  semp_t *port_sem;
  pthread_attr_t ThreadAttr;
  /* 
 IL_CLIENT_FILEIO_PARAMS sFileIOBuff[IL_CLIENT_MAX_NUM_FILE_OUT_BUFS]; *//* file Input/Output buffers */
} IL_CLIENT_COMP_PRIVATE_T;

typedef struct IL_CLIENT_GFX_PRIVATE_T
{
  pthread_t ThrdId;
  pthread_attr_t ThreadAttr;
  uint32_t terminateGfx;
  uint32_t terminateDone;
  uint32_t gfxId;
} IL_CLIENT_GFX_PRIVATE_T;

typedef enum
{
  IL_CLIENT_PIPE_CMD_ETB,
  IL_CLIENT_PIPE_CMD_FTB,
  IL_CLIENT_PIPE_CMD_EBD,
  IL_CLIENT_PIPE_CMD_FBD,
  IL_CLIENT_PIPE_CMD_EXIT,
  IL_CommandMax = 0X7FFFFFFF
} IL_CLIENT_PIPE_CMD_TYPE;

typedef enum
{
  ILCLIENT_INPUT_PORT,
  ILCLIENT_OUTPUT_PORT,
  ILCLIENT_PortMax = 0X7FFFFFFF
} ILCLIENT_PORT_TYPE;

typedef struct IL_CLIENT_PIPE_MSG_T
{
  IL_CLIENT_PIPE_CMD_TYPE cmd;
  OMX_BUFFERHEADERTYPE *pbufHeader;   /* used for EBD/FBD */
  OMX_BUFFERHEADERTYPE bufHeader;     /* used for ETB/FTB */
} IL_CLIENT_PIPE_MSG;


/* ========================================================================== */
/** IL_Client is the structure definition for the  decoder->sc->display IL Client
*
* @param pHandle               OMX Handle  
* @param pComponent            Component Data structure
* @param pCb                   Callback function pointer
* @param eState                Current OMX state
* @param pInPortDef            Structure holding input port definition
* @param pOutPortDef           Structure holding output port definition
* @param eCompressionFormat    Format of the input data
* @param pH264                 <TBD>
* @param pInBuff               Input Buffer pointer
* @param pOutBuff              Output Buffer pointer
* @param IpBuf_Pipe            Input Buffer Pipe
* @param OpBuf_Pipe            Output Buffer Pipe
* @param fIn                   File pointer of input file
* @param fInFrmSz              File pointer of Frame Size file (unused)
* @param fOut                  Output file pointer
* @param ColorFormat           Input color format
* @param nWidth                Width of the input vector
* @param nHeight               Height of the input vector
* @param nEncodedFrm           Total number of encoded frames
*/
/* ========================================================================== */
typedef struct IL_Client
{
  OMX_HANDLETYPE pCapHandle, pScHandle, pDisHandle, pctrlHandle, pNfHandle;
  OMX_COMPONENTTYPE *pComponent;
  OMX_CALLBACKTYPE pCb;
  OMX_STATETYPE eState;
  OMX_U8 eCompressionFormat;
  OMX_VIDEO_PARAM_AVCTYPE *pH264;
  FILE *fIn;
  FILE *fInFrmSz;
  FILE *fOut;
  OMX_COLOR_FORMATTYPE ColorFormat;
  OMX_U32 nWidth;
  OMX_U32 nHeight;
  OMX_U32 nDecStride;
  OMX_U32 nFrameRate;
  OMX_U32 nEncodedFrms;
  OMX_U32 displayId;
  H264_ParsingCtx pc;
  VC1_ParsingCtx pcvc1;
  H263_ParsingCtx pch263;
  MPEG4_ParsingCtx pcmpeg4;
  MPEG2_ParsingCtx pcmpeg2;
  OMX_VIDEO_CODINGTYPE  codingType;
  void *fieldBuf;
  IL_CLIENT_COMP_PRIVATE *capILComp;
  IL_CLIENT_COMP_PRIVATE *scILComp;
  IL_CLIENT_COMP_PRIVATE *disILComp;
  IL_CLIENT_COMP_PRIVATE *nfILComp;
  IL_CLIENT_GFX_PRIVATE gfx;
} IL_Client;

void IL_ClientInit (IL_Client **pAppData, int width, int height,
                    int frameRate, int displayId, OMX_VIDEO_CODINGTYPE coding);

void IL_ClientDeInit (IL_Client *pAppData);

OMX_STRING IL_ClientErrorToStr (OMX_ERRORTYPE error);

OMX_ERRORTYPE IL_ClientProcessPipeCmdETB (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE IL_ClientProcessPipeCmdFTB (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE IL_ClientProcessPipeCmdEBD (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE IL_ClientProcessPipeCmdFBD (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE IL_ClientCbFillBufferDone (OMX_HANDLETYPE hComponent,
                                         OMX_PTR ptrAppData,
                                         OMX_BUFFERHEADERTYPE *pBuffer);
OMX_ERRORTYPE IL_ClientCbEmptyBufferDone (OMX_HANDLETYPE hComponent,
                                          OMX_PTR ptrAppData,
                                          OMX_BUFFERHEADERTYPE *pBuffer);
OMX_ERRORTYPE IL_ClientCbEventHandler (OMX_HANDLETYPE hComponent,
                                              OMX_PTR ptrAppData,
                                              OMX_EVENTTYPE eEvent,
                                              OMX_U32 nData1, OMX_U32 nData2,
                                              OMX_PTR pEventData);
void IL_ClientSIGINTHandler(int sig);

#ifdef __cplusplus              /* matches __cplusplus construct above */
}
#endif

#endif

/*--------------------------------- end of file -----------------------------*/
