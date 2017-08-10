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

/**
 *******************************************************************************
 *  @file  decode_display_test.c
 *  @brief This file contains all Functions related to Test Application
 *
 *         This is the example IL Client support to create, configure & chaining
 *         of single channel omx-components using  non tunneling 
 *         mode
 *
 *  @rev 1.0
 *******************************************************************************
 */

/*******************************************************************************
*                             Compilation Control Switches
*******************************************************************************/
/*None*/

/*******************************************************************************
*                             INCLUDE FILES
*******************************************************************************/

/*--------------------- system and platform files ----------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <xdc/std.h>

/*-------------------------program files -------------------------------------*/
#include "ti/omx/interfaces/openMaxv11/OMX_Core.h"
#include "ti/omx/interfaces/openMaxv11/OMX_Component.h"
#include "ilclient.h"
#include "ilclient_utils.h"
#include "es_parser.h"
#include <ti/omx/comp/vfcc/omx_vfcc.h>
#include <ti/omx/comp/vdec/omx_vdec.h>
#include <ti/omx/comp/vfpc/omx_vfpc.h>
#include <ti/omx/comp/vfdc/omx_vfdc.h>
#include <ti/omx/comp/ctrl/omx_ctrl.h>

OMX_U8 PADX;
OMX_U8 PADY;
extern void IL_ClientFbDevAppTask (void *threadsArg);
OMX_BOOL gILClientExit = OMX_FALSE;
typedef void *(*ILC_StartFcnPtr) (void *);
/* ========================================================================== */
/**
* IL_ClientCbEventHandler() : This method is the event handler implementation to 
* handle events from the OMX Derived component
*
* @param hComponent        : Handle to the component
* @param ptrAppData        : 
* @param eEvent            :
* @param nData1            :
* @param nData2            :
* @param pEventData        :
*
*  @return      
*  OMX_ErrorNone = Successful 
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */
OMX_ERRORTYPE IL_ClientCbEventHandler (OMX_HANDLETYPE hComponent,
                                              OMX_PTR ptrAppData,
                                              OMX_EVENTTYPE eEvent,
                                              OMX_U32 nData1, OMX_U32 nData2,
                                              OMX_PTR pEventData)
{
  IL_CLIENT_COMP_PRIVATE *comp;

  comp = ptrAppData;

  printf ("got event");
  if (eEvent == OMX_EventCmdComplete)
  {
    if (nData1 == OMX_CommandStateSet)
    {
      printf ("State changed to: ");
      switch ((int) nData2)
      {
        case OMX_StateInvalid:
          printf ("OMX_StateInvalid \n");
          break;
        case OMX_StateLoaded:
          printf ("OMX_StateLoaded \n");
          break;
        case OMX_StateIdle:
          printf ("OMX_StateIdle \n");
          break;
        case OMX_StateExecuting:
          printf ("OMX_StateExecuting \n");
          break;
        case OMX_StatePause:
          printf ("OMX_StatePause\n");
          break;
        case OMX_StateWaitForResources:
          printf ("OMX_StateWaitForResources\n");
          break;
      }
      /* post an semaphore, so that in IL Client we can confirm the state
         change */
      semp_post (comp->done_sem);
    }
    else if (OMX_CommandFlush == nData1) {
     printf(" OMX_CommandFlush completed \n");
      semp_post (comp->done_sem);
     }
    
    else if (OMX_CommandPortEnable || OMX_CommandPortDisable)
    {
      printf ("Enable/Disable Event \n");
      semp_post (comp->port_sem);
    }
  }
  else if (eEvent == OMX_EventBufferFlag)
  {
    printf ("OMX_EventBufferFlag \n");
    if ((int) nData2 == OMX_BUFFERFLAG_EOS)
    {
      printf ("got EOS event \n");
      semp_post (comp->eos);
    }
  }
  else if (eEvent == OMX_EventError)
  {
    printf ("*** unrecoverable error: %s (0x%lx) \n",
            IL_ClientErrorToStr (nData1), nData1);
  }
  else
  {
    printf ("unhandled event, param1 = %i, param2 = %i \n", (int) nData1,
            (int) nData2);
  }

  return OMX_ErrorNone;
}

/* ========================================================================== */
/**
* IL_ClientCbEmptyBufferDone() : This method is the callback implementation to 
* handle EBD events from the OMX Derived component
*
* @param hComponent        : Handle to the component
* @param ptrAppData        : app pointer, which was passed during the getHandle
* @param pBuffer           : buffer header, for the buffer which is consumed
*
*  @return      
*  OMX_ErrorNone = Successful 
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientCbEmptyBufferDone (OMX_HANDLETYPE hComponent,
                                          OMX_PTR ptrAppData,
                                          OMX_BUFFERHEADERTYPE *pBuffer)
{
  IL_CLIENT_COMP_PRIVATE *thisComp = (IL_CLIENT_COMP_PRIVATE *) ptrAppData;
  IL_CLIENT_INPORT_PARAMS *inPortParamsPtr;
  IL_CLIENT_PIPE_MSG localPipeMsg;

  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int retVal = 0;

  inPortParamsPtr = thisComp->inPortParams + pBuffer->nInputPortIndex;

  /* if the buffer is from file i/o, write the free buffer header into ipbuf
     pipe, else keep it in its local pipe. From local pipe It would be given to 
     remote component as "consumed buffer " */

  if (inPortParamsPtr->connInfo.remotePipe[0] == NULL)
  {
    /* write the empty buffer pointer to input pipe */
    retVal = write (inPortParamsPtr->ipBufPipe[1], &pBuffer, sizeof (pBuffer));

    if (sizeof (pBuffer) != retVal)
    {
      printf ("Error writing into Input buffer i/p Pipe!\n");
      eError = OMX_ErrorNotReady;
      return eError;
    }
  }
  else
  {
    /* Create a message that EBD is done and this buffer is ready to be
       recycled. This message will be read in buffer processing thread and
       remote component will be indicated about its status */
    localPipeMsg.cmd = IL_CLIENT_PIPE_CMD_EBD;
    localPipeMsg.pbufHeader = pBuffer;
    retVal = write (thisComp->localPipe[1],
                    &localPipeMsg, sizeof (IL_CLIENT_PIPE_MSG));
    if (sizeof (IL_CLIENT_PIPE_MSG) != retVal)
    {
      printf ("Error writing into local Pipe!\n");
      eError = OMX_ErrorNotReady;
      return eError;
    }
  }

  return eError;
}

/* ========================================================================== */
/**
* IL_ClientCbFillBufferDone() : This method is the callback implementation to 
* handle FBD events from the OMX Derived component
*
* @param hComponent        : Handle to the component
* @param ptrAppData        : app pointer, which was passed during the getHandle
* @param pBuffer           : buffer header, for the buffer which is produced
*
*  @return      
*  OMX_ErrorNone = Successful 
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientCbFillBufferDone (OMX_HANDLETYPE hComponent,
                                         OMX_PTR ptrAppData,
                                         OMX_BUFFERHEADERTYPE *pBuffer)
{
  IL_CLIENT_COMP_PRIVATE *thisComp = (IL_CLIENT_COMP_PRIVATE *) ptrAppData;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamsPtr;
  IL_CLIENT_PIPE_MSG localPipeMsg;

  OMX_ERRORTYPE eError = OMX_ErrorNone;
  int retVal = 0;

  /* get the pipe corrsponding to this port, portIndex is part of bufferheader
     structure */
  outPortParamsPtr =
    thisComp->outPortParams + (pBuffer->nOutputPortIndex -
                               thisComp->startOutportIndex);

  /* if the buffer is from file i/o, write the free buffer header into outbuf
     pipe, else keep it in its local pipe. From local pipe It would be given to 
     remote component as "filled buffer " */
  if (outPortParamsPtr->connInfo.remotePipe[0] == NULL)
  {
    /* write the empty buffer pointer to input pipe */
    retVal = write (outPortParamsPtr->opBufPipe[1], &pBuffer, sizeof (pBuffer));

    if (sizeof (pBuffer) != retVal)
    {
      printf ("Error writing to Input buffer i/p Pipe!\n");
      eError = OMX_ErrorNotReady;
      return eError;
    }
  }
  else
  {
    /* Create a message that FBD is done and this buffer is ready to be used by 
       other compoenent. This message will be read in buffer processing thread
       and and remote component will be indicated about its status */
    localPipeMsg.cmd = IL_CLIENT_PIPE_CMD_FBD;
    localPipeMsg.pbufHeader = pBuffer;
    retVal = write (thisComp->localPipe[1],
                    &localPipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

    if (sizeof (IL_CLIENT_PIPE_MSG) != retVal)
    {
      printf ("Error writing to local Pipe!\n");
      eError = OMX_ErrorNotReady;
      return eError;
    }
  }
  return eError;
}


/* ========================================================================== */
/**
* IL_ClientConnInConnOutTask() : This task function is for passing buffers from
* one component to other connected component. This functions reads from local
* pipe of a particular component , and takes action based on the message in the
* pipe. This pipe is written by callback ( EBD/FBD) function from component and
* from other component threads, which writes into this pipe for buffer 
* communication.
*
* @param threadsArg        : Handle to a particular component
*
*/
/* ========================================================================== */

void IL_ClientConnInConnOutTask (void *threadsArg)
{
  IL_CLIENT_PIPE_MSG pipeMsg;
  IL_CLIENT_COMP_PRIVATE *thisComp = (IL_CLIENT_COMP_PRIVATE *) threadsArg;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  /* Initially pipes will not have any buffers, so components needs to be given 
     empty buffers for output ports. Input bufefrs are given by other
     component, or file read task */
  IL_ClientUseInitialOutputResources (thisComp);

  for (;;)
  {
    /* Read from its own local Pipe */
    read (thisComp->localPipe[0], &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

    /* check the function type */

    switch (pipeMsg.cmd)
    {
      case IL_CLIENT_PIPE_CMD_EXIT:
        printf ("exiting thread\n ");
        pthread_exit (thisComp);
        break;
      case IL_CLIENT_PIPE_CMD_ETB:
        err = IL_ClientProcessPipeCmdETB (thisComp, &pipeMsg);
        /* If not in proper state, bufers may not be accepted by component */
        if (OMX_ErrorNone != err)
        {
          write (thisComp->localPipe[1], &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));
          printf (" ETB: wait \n");
          /* since in this example we are changing states in other thread it
             will return error for giving ETB/FTB calls in non-execute state.
             Since example is shutting down, we exit the thread */
          pthread_exit (thisComp);
          /* if error is incorrect state operation, wait for state to change */
          /* waiting mechanism should be implemented here */
        }
  
        break;
      case IL_CLIENT_PIPE_CMD_FTB:
        err = IL_ClientProcessPipeCmdFTB (thisComp, &pipeMsg);
  
        if (OMX_ErrorNone != err)
        {
          write (thisComp->localPipe[1], &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));
          printf (" FTB: wait \n");
          /* if error is incorrect state operation, wait for state to change
             waiting mechanism should be implemented here */
          /* since in this example we are changing states in other thread it
             will return error for giving ETB/FTB calls in non-execute state.
             Since example is shutting down, we exit the thread */
          pthread_exit (thisComp);
        }
        break;
  
      case IL_CLIENT_PIPE_CMD_EBD:
        IL_ClientProcessPipeCmdEBD (thisComp, &pipeMsg);
  
        break;

      case IL_CLIENT_PIPE_CMD_FBD:
        IL_ClientProcessPipeCmdFBD (thisComp, &pipeMsg);
        break;

      default:
        break;
    } /* switch () */
  } /* for (;;) */
}

/* ========================================================================== */
/**
* IL_ClientProcessPipeCmdETB() : This function passes the buffers to component
* for consuming. This buffer will come from other component as an output. To 
* consume it, IL client finds its buffer header (for consumer component), and 
* calls ETB call.
* @param thisComp        : Handle to a particular component
* @param pipeMsg         : message structure, which is written in response to 
*                          callbacks
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientProcessPipeCmdETB (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferIn;

  /* search its own buffer header based on submitted by connected comp */
  IL_ClientUtilGetSelfBufHeader (thisComp, pipeMsg->bufHeader.pBuffer,
                                 ILCLIENT_INPUT_PORT,
                                 pipeMsg->bufHeader.nInputPortIndex,
                                 &pBufferIn);

  /* populate buffer header */
  pBufferIn->nFilledLen = pipeMsg->bufHeader.nFilledLen;
  pBufferIn->nOffset = pipeMsg->bufHeader.nOffset;
  pBufferIn->nTimeStamp = pipeMsg->bufHeader.nTimeStamp;
  pBufferIn->nFlags = pipeMsg->bufHeader.nFlags;
  pBufferIn->hMarkTargetComponent = pipeMsg->bufHeader.hMarkTargetComponent;
  pBufferIn->pMarkData = pipeMsg->bufHeader.pMarkData;
  pBufferIn->nTickCount = 0;

  /* call etb to the component */
  err = OMX_EmptyThisBuffer (thisComp->handle, pBufferIn);
  return (err);
}

/* ========================================================================== */
/**
* IL_ClientProcessPipeCmdFTB() : This function passes the buffers to component
* for consuming. This buffer will come from other component as consumed at input
* To  consume it, IL client finds its buffer header (for consumer component),
* and calls FTB call.
* @param thisComp        : Handle to a particular component
* @param pipeMsg         : message structure, which is written in response to 
*                          callbacks
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientProcessPipeCmdFTB (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferOut;

  /* search its own buffer header based on submitted by connected comp */
  IL_ClientUtilGetSelfBufHeader (thisComp, pipeMsg->bufHeader.pBuffer,
                                 ILCLIENT_OUTPUT_PORT,
                                 pipeMsg->bufHeader.nOutputPortIndex,
                                 &pBufferOut);

  /* call etb to the component */
  err = OMX_FillThisBuffer (thisComp->handle, pBufferOut);

  return (err);
}

/* ========================================================================== */
/**
* IL_ClientProcessPipeCmdEBD() : This function passes the bufefrs to component
* for consuming. This empty buffer will go to other component to be reused at 
* output port.
* @param thisComp        : Handle to a particular component
* @param pipeMsg         : message structure, which is written in response to 
*                          callbacks
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientProcessPipeCmdEBD (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferIn;
  IL_CLIENT_PIPE_MSG remotePipeMsg;
  IL_CLIENT_INPORT_PARAMS *inPortParamsPtr;
  int retVal = 0;

  pBufferIn = pipeMsg->pbufHeader;

  /* find the input port structure (pipe) */
  inPortParamsPtr = thisComp->inPortParams + pBufferIn->nInputPortIndex;

  remotePipeMsg.cmd = IL_CLIENT_PIPE_CMD_FTB;
  remotePipeMsg.bufHeader.pBuffer = pBufferIn->pBuffer;
  remotePipeMsg.bufHeader.nOutputPortIndex =
    inPortParamsPtr->connInfo.remotePort;

  /* write the fill buffer message to remote pipe */
  retVal = write (inPortParamsPtr->connInfo.remotePipe[1],
                  &remotePipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  if (sizeof (IL_CLIENT_PIPE_MSG) != retVal)
  {
    printf ("Error writing to remote Pipe!\n");
    err = OMX_ErrorNotReady;
    return err;
  }

  return (err);
}

/* ========================================================================== */
/**
* IL_ClientProcessPipeCmdFBD() : This function passes the bufefrs to component
* for consuming. This buffer will go to other component to be consumed at input
* port.
* @param thisComp        : Handle to a particular component
* @param pipeMsg         : message structure, which is written in response to 
*                          callbacks
*
*/
/* ========================================================================== */

OMX_ERRORTYPE IL_ClientProcessPipeCmdFBD (IL_CLIENT_COMP_PRIVATE *thisComp,
                                          IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferOut;
  IL_CLIENT_PIPE_MSG remotePipeMsg;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamsPtr;
  int retVal = 0;
  pBufferOut = pipeMsg->pbufHeader;

  remotePipeMsg.cmd = IL_CLIENT_PIPE_CMD_ETB;
  remotePipeMsg.bufHeader.pBuffer = pBufferOut->pBuffer;

  outPortParamsPtr =
    thisComp->outPortParams + (pBufferOut->nOutputPortIndex -
                               thisComp->startOutportIndex);

  /* populate buffer header */
  remotePipeMsg.bufHeader.nFilledLen = pBufferOut->nFilledLen;
  remotePipeMsg.bufHeader.nOffset = pBufferOut->nOffset;
  remotePipeMsg.bufHeader.nTimeStamp = pBufferOut->nTimeStamp;
  remotePipeMsg.bufHeader.nFlags = pBufferOut->nFlags;
  remotePipeMsg.bufHeader.hMarkTargetComponent =
    pBufferOut->hMarkTargetComponent;
  remotePipeMsg.bufHeader.pMarkData = pBufferOut->pMarkData;
  remotePipeMsg.bufHeader.nTickCount = pBufferOut->nTickCount;
  remotePipeMsg.bufHeader.nInputPortIndex =
    outPortParamsPtr->connInfo.remotePort;

  /* write the fill buffer message to remote pipe */
  retVal = write (outPortParamsPtr->connInfo.remotePipe[1],
                  &remotePipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  if (sizeof (IL_CLIENT_PIPE_MSG) != retVal)
  {
    printf ("Error writing to remote Pipe!\n");
    err = OMX_ErrorNotReady;
    return err;
  }

  return (err);
}

/* ========================================================================== */
/**
* IL_ClientSIGINTHandler() : This function is the SIGINT handler that will be
* called when the user invokes CTRL-C. This is for demonstration purpose. Also
* it assumes that the OMX chain is in EXECUTING state when CTRL-C is invoked
*
* @param sig             : Signal identifier
*/
/* ========================================================================== */
void IL_ClientSIGINTHandler(int sig)
{
 gILClientExit = OMX_TRUE;
}

/* ========================================================================== */
/**
* Decode_Display_Test() : This method is the IL Client implementation for 
* connecting decoder, scalar and display OMX components. This function creates
* configures, and connects the components. it manages the buffer communication.
*
*  @param args         : arg pointer for parameters e.g. width, height, framerate
*  @return      
*  OMX_ErrorNone = Successful 
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

/* Main IL Client application to create , intiate and connect components */

IL_Client *pAppData = NULL;

Int Decode_Display_Example (IL_ARGS *args)
{

  OMX_U32 i;
  OMX_S32 ret_value;

  OMX_ERRORTYPE eError = OMX_ErrorNone;
  IL_CLIENT_PIPE_MSG pipeMsg;
  OMX_VIDEO_CODINGTYPE coding;
  gILClientExit = OMX_FALSE;

  pAppData = NULL;
  printf (" Setting Pads \n");
/*
  if(!strcmp(args->codec_name,"h264"))
  {
      PADX = 32;
      PADY = 24;
      coding = OMX_VIDEO_CodingAVC;
  }
  else if(!strcmp(args->codec_name,"vc1"))
  {
      PADX = 32;
      PADY = 40;
      coding = OMX_VIDEO_CodingWMV;
  }
  else if(!strcmp(args->codec_name,"h263"))
  {
      PADX = 16;
      PADY = 16;
      coding = OMX_VIDEO_CodingH263;
  }
  else if(!strcmp(args->codec_name,"mpeg4"))
  {
      PADX = 16;
      PADY = 16;
      coding = OMX_VIDEO_CodingMPEG4;
  }
  else if(!strcmp(args->codec_name,"mpeg2"))
  {
      PADX = 8;
      PADY = 8;
      coding = OMX_VIDEO_CodingMPEG2;
  }
  else if(!strcmp(args->codec_name,"mjpeg"))
  {
      PADX = 0;
      PADY = 0;
      coding = OMX_VIDEO_CodingMJPEG;
  }
*/
  printf (" Client Init \n");

  /* Initialize application specific data structures and buffer management
     data structure */
    IL_ClientInit (&pAppData, args->width, args->height, args->frame_rate, 
                 args->display_id, coding) ;

  printf (" Callback init \n");

  /* Initialize application / IL Client callback functions */
  /* Callbacks are passed during getHandle call to component, Component uses
     these callaback to communicate with IL Client */
  /* event handler is to handle the state changes , omx commands and any
     message for IL client */
  pAppData->pCb.EventHandler = IL_ClientCbEventHandler;

  /* Empty buffer done is data callback at the input port, where component lets 
     the application know that buffer has been consumned, this is not
     applicable if there is no input port in the component */
  pAppData->pCb.EmptyBufferDone = IL_ClientCbEmptyBufferDone;

  /* fill buffer done is callback at the output port, where component lets the
     application know that an output buffer is available with the processed data 
   */
  pAppData->pCb.FillBufferDone = IL_ClientCbFillBufferDone;

  printf (" Get capture handle \n");

/******************************************************************************/
  /* Create the capture Component, component handle would be returned component 
     name is unique and fixed for a componnet, callback are passed to
     component in this function. component would be in loaded state post this
     call */

  eError =
    OMX_GetHandle (&pAppData->pCapHandle,
                   (OMX_STRING) "OMX.TI.VPSSM3.VFCC", pAppData->capILComp,
                   &pAppData->pCb);

  printf (" capture compoenent is created \n");

  if ((eError != OMX_ErrorNone) || (pAppData->pCapHandle == NULL))
  {
    printf ("Error in Get Handle function : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  pAppData->capILComp->handle = pAppData->pCapHandle;

  /* Configute the capture componet */
  /* calling OMX_Setparam in this function */
  IL_ClientSetCaptureParams (pAppData);

  printf ("enable capture output port \n");

  OMX_SendCommand (pAppData->pCapHandle, OMX_CommandPortEnable,
                   OMX_VFCC_OUTPUT_PORT_START_INDEX, NULL);

  semp_pend (pAppData->capILComp->port_sem);

#if 0
/******************************************************************************/
  /* Create the H264Decoder Component, component handle would be returned
     component name is unique and fixed for a component, callbacks are passed
     to component in this function. Component would be loaded state post this
     call */

  eError =
    OMX_GetHandle (&pAppData->pDecHandle,
                   (OMX_STRING) "OMX.TI.DUCATI.VIDDEC", pAppData->decILComp,
                   &pAppData->pCb);

  printf (" decoder compoenent is created \n");

  if ((eError != OMX_ErrorNone) || (pAppData->pDecHandle == NULL))
  {
    printf ("Error in Get Handle function : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  pAppData->decILComp->handle = pAppData->pDecHandle;

  /* Configute the decoder componet */
  /* calling OMX_Setparam in this function */
  IL_ClientSetDecodeParams (pAppData);
#endif

/******************************************************************************/
  /* Create Scalar component, it creatd OMX compponnet for scalar writeback ,
     Int this client we are passing the same callbacks to all the component */

  eError =
    OMX_GetHandle (&pAppData->pScHandle,
                   (OMX_STRING) "OMX.TI.VPSSM3.VFPC.INDTXSCWB",
                   pAppData->scILComp, &pAppData->pCb);

  if ((eError != OMX_ErrorNone) || (pAppData->pScHandle == NULL))
  {
    printf ("Error in Get Handle function : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }
  pAppData->scILComp->handle = pAppData->pScHandle;

  printf (" scalar compoenent is created \n");

  /* omx calls are made in this function for setting the parameters for scalar
     component, For clarity purpose it is written as separate function */

  eError = IL_ClientSetScalarParams (pAppData);
  if ((eError != OMX_ErrorNone))
  {    
    goto EXIT;
  }

  /* enable input and output port */
  /* as per openmax specs all the ports should be enabled by default but EZSDK
     OMX component does not enable it hence we manually need to enable it. */

  printf ("enable scalar input port \n");
  OMX_SendCommand (pAppData->pScHandle, OMX_CommandPortEnable,
                   OMX_VFPC_INPUT_PORT_START_INDEX, NULL);

  /* wait for both ports to get enabled, event handler would be notified from
     the component after enabling the port, which inturn would post this
     semaphore */
  semp_pend (pAppData->scILComp->port_sem);

  printf ("enable scalar output port \n");
  OMX_SendCommand (pAppData->pScHandle, OMX_CommandPortEnable,
                   OMX_VFPC_OUTPUT_PORT_START_INDEX, NULL);
  semp_pend (pAppData->scILComp->port_sem);

/*****************************************************************************/
  /* Noise filter is used to convert 422 o/p to 420 for SD display only */
  if( pAppData->displayId == 2) {
  /* Create NF component, it creatd OMX compponnet for Dei writeback ,
     Int this client we are passing the same callbacks to all the component */

  eError =
    OMX_GetHandle (&pAppData->pNfHandle,
                   (OMX_STRING) "OMX.TI.VPSSM3.VFPC.NF",
                   pAppData->nfILComp, &pAppData->pCb);

  if ((eError != OMX_ErrorNone) || (pAppData->pNfHandle == NULL))
  {
    printf ("Error in Get Handle function : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }
  pAppData->nfILComp->handle = pAppData->pNfHandle;

  printf (" NF compoenent is created \n");

  /* omx calls are made in this function for setting the parameters for Dei
     component, For clarity purpose it is written as separate function */

  IL_ClientSetNfParams (pAppData);

  /* enable input and output port */
  /* as per openmax specs all the ports should be enabled by default but EZSDK
     OMX component does not enable it hence we manually need to enable it. */

  printf ("enable NF input port \n");
  OMX_SendCommand (pAppData->pNfHandle, OMX_CommandPortEnable,
                   OMX_VFPC_INPUT_PORT_START_INDEX, NULL);

  /* wait for both ports to get enabled, event handler would be notified from
     the component after enabling the port, which inturn would post this
     semaphore */
  semp_pend (pAppData->nfILComp->port_sem);

  printf ("enable NF output port \n");
  OMX_SendCommand (pAppData->pNfHandle, OMX_CommandPortEnable,
                   OMX_VFPC_OUTPUT_PORT_START_INDEX, NULL);
  semp_pend (pAppData->nfILComp->port_sem);
 }
/*****************************************************************************/

/******************************************************************************/
/* Create and Configure the display component. It will use VFDC component on  */
/* media controller.                                                          */
/******************************************************************************/

  /* Create the display component */
  /* getting display component handle */
  eError =
    OMX_GetHandle (&pAppData->pDisHandle, "OMX.TI.VPSSM3.VFDC",
                   pAppData->disILComp, &pAppData->pCb);
  if (eError != OMX_ErrorNone)
    ERROR ("failed to get handle\n");

  printf ("found handle %p for component %s \n", pAppData->pDisHandle,
          "OMX.TI.VPSSM3.VFDC");

  pAppData->disILComp->handle = pAppData->pDisHandle;

  printf (" got display handle \n");
  /* getting display controller component handle, Display contrller is
     implemented as an OMX component, however it does not have any input or
     output ports. It is used only for controling display hw */
  eError =
    OMX_GetHandle (&pAppData->pctrlHandle, "OMX.TI.VPSSM3.CTRL.DC",
                   pAppData->disILComp, &pAppData->pCb);
  if (eError != OMX_ErrorNone)
    ERROR ("failed to get handle\n");

  printf ("found handle %p for component %s\n", pAppData->pctrlHandle,
          "OMX.TI.VPSSM3.CTRL.DC");

  /* omx calls are made in this function for setting the parameters for display 
     component, For clarity purpose it is written as separate function */

  eError = IL_ClientSetDisplayParams(pAppData);
  if ((eError != OMX_ErrorNone))
  {    
    goto EXIT;
  }

  /* as per openmax specs all the ports should be enabled by default but EZSDK
     OMX component does not enable it hence we manually need to enable it */
  printf ("enable input port \n");

  OMX_SendCommand (pAppData->pDisHandle, OMX_CommandPortEnable,
                   OMX_VFDC_INPUT_PORT_START_INDEX, NULL);

  /* wait for port to get enabled, event handler would be notified from the
     component after enabling the port, which inturn would post this semaphore */
  semp_pend (pAppData->disILComp->port_sem);

/******************************************************************************/

  /* Connect the decoder to scalar, This application uses 'pipe' to pass the
     buffers between different components. each compponent has a local pipe,
     which It reads for taking buffers. By connecting this functions informs
     about local pipe to other component, so that other component can pass
     buffers to this 'remote' pipe */

  printf (" connect call for capture-scalar \n ");

  IL_ClientConnectComponents (pAppData->capILComp, OMX_VFCC_OUTPUT_PORT_START_INDEX,
                              pAppData->scILComp,
                              OMX_VFPC_INPUT_PORT_START_INDEX);


  if( pAppData->displayId == 2) {
  
  printf (" connect call for scalar - nf \n ");
  /* connect scalar output to Noise filter */
  IL_ClientConnectComponents (pAppData->scILComp,
                              OMX_VFPC_OUTPUT_PORT_START_INDEX,
                              pAppData->nfILComp,
                              OMX_VFPC_INPUT_PORT_START_INDEX);
  
  printf (" connect call for nf - display \n ");
                              
  /* Connect Noise filter output ( 420SP only) to SD display */                            
  IL_ClientConnectComponents (pAppData->nfILComp,
                              OMX_VFPC_OUTPUT_PORT_START_INDEX,
                              pAppData->disILComp,
                              OMX_VFDC_INPUT_PORT_START_INDEX);
 }
  else {
  printf (" connect call for scalar-display \n ");
  /* Connect the scalar to display */
  IL_ClientConnectComponents (pAppData->scILComp,
                              OMX_VFPC_OUTPUT_PORT_START_INDEX,
                              pAppData->disILComp,
                              OMX_VFDC_INPUT_PORT_START_INDEX);
  }
/******************************************************************************/
    
  /* OMX_SendCommand expecting OMX_StateIdle, after this command component
     would create codec, and will wait for all buffers to be allocated as per
     omx buffers are created during loaded to Idle transition ( if ports are
     enabled ) */
  eError =
    OMX_SendCommand (pAppData->pCapHandle, OMX_CommandStateSet,
                     OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

 /* Allocate I/O Buffers; componnet would allocated buffers and would return
     the buffer header containing the pointer to buffer */
  for (i = 0; i < pAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {
    eError = OMX_AllocateBuffer (pAppData->pCapHandle,
                                 &pAppData->capILComp->outPortParams->
                                 pOutBuff[i],
                                 OMX_VFCC_OUTPUT_PORT_START_INDEX, pAppData,
                                 pAppData->capILComp->outPortParams->
                                 nBufferSize);

    if (eError != OMX_ErrorNone)
    {
      printf
        ("Capture: Error in OMX_AllocateBuffer() : %s \n",
         IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }
  printf (" Capture outport buffers allocated \n ");

  semp_pend (pAppData->capILComp->done_sem);

  printf (" Capture is in IDLE state \n");
#if 0
  /* Allocate I/O Buffers; component would allocate buffers and would return
     the buffer header containing the pointer to buffer */
  for (i = 0; i < pAppData->decILComp->inPortParams->nBufferCountActual; i++)
  {
    eError = OMX_AllocateBuffer (pAppData->pDecHandle,
                                 &pAppData->decILComp->
                                 inPortParams->pInBuff[i],
                                 OMX_VIDDEC_INPUT_PORT, pAppData,
                                 pAppData->decILComp->
                                 inPortParams->nBufferSize);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error in OMX_AllocateBuffer()- Input Port State set : %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }

  printf (" decoder inport buffers allocated \n ");


  /* buffer alloaction for output port */
  for (i = 0; i < pAppData->decILComp->outPortParams->nBufferCountActual; i++)
  {
    eError = OMX_AllocateBuffer (pAppData->pDecHandle,
                                 &pAppData->decILComp->
                                 outPortParams->pOutBuff[i],
                                 OMX_VIDDEC_OUTPUT_PORT, pAppData,
                                 pAppData->decILComp->
                                 outPortParams->nBufferSize);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error in OMX_AllocateBuffer()-Output Port State set : %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }

  printf (" decoder outport buffers allocated \n ");

  /* Wait for initialization to complete.. Wait for Idle stete of component
     after all buffers are alloacted componet would chnage to idle */
  semp_pend (pAppData->decILComp->done_sem);

  printf (" decoder state IDLE \n ");
  #endif
/******************************************************************************/
  eError =
    OMX_SendCommand (pAppData->pScHandle, OMX_CommandStateSet,
                     OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  /* since capture is connected to scalar, buffers are supplied by capture to
     scalar, so scalar does not allocate the buffers. Howvere it is informed to 
     use the buffers created by capture. scalar component would create only
     buffer headers coreesponding to these buffers */

  for (i = 0; i < pAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {

    eError = OMX_UseBuffer (pAppData->pScHandle,
                            &pAppData->scILComp->inPortParams->pInBuff[i],
                            OMX_VFPC_INPUT_PORT_START_INDEX,
                            pAppData->scILComp,
                            pAppData->capILComp->outPortParams->nBufferSize,
                            pAppData->capILComp->outPortParams->pOutBuff[i]->
                            pBuffer);

    if (eError != OMX_ErrorNone)
    {
      printf ("Error in OMX_UseBuffer()-input Port State set : %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }
  printf (" Scalar input port use buffer done \n ");

  /* in SDK conventionally output port allocates the buffers, scalar would
     create the buffers which would be consumed by display component */
  /* buffer alloaction for output port */
  for (i = 0; i < pAppData->scILComp->outPortParams->nBufferCountActual; i++)
  {
    eError = OMX_AllocateBuffer (pAppData->pScHandle,
                                 &pAppData->scILComp->
                                 outPortParams->pOutBuff[i],
                                 OMX_VFPC_OUTPUT_PORT_START_INDEX, pAppData,
                                 pAppData->scILComp->
                                 outPortParams->nBufferSize);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error in OMX_AllocateBuffer()-Output Port State set : %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }

  printf (" scalar outport buffers allocated \n ");

  semp_pend (pAppData->scILComp->done_sem);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error %s:    WaitForState has timed out \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }
  printf (" scalar state IDLE \n ");

/******************************************************************************/
  if (pAppData->displayId == 2) {
    eError =
      OMX_SendCommand (pAppData->pNfHandle, OMX_CommandStateSet,
                       OMX_StateIdle, NULL);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }

    /* since nf is connected to scalar, buffers are supplied by scalar to
       nf, so nf does not allocate the buffers. However it is informed to 
       use the buffers created by scalar. nf component would create only
       buffer headers corresponding to these buffers */

    for (i = 0; i < pAppData->scILComp->outPortParams->nBufferCountActual; i++)
    {

      eError = OMX_UseBuffer (pAppData->pNfHandle,
                              &pAppData->nfILComp->inPortParams->pInBuff[i],
                              OMX_VFPC_INPUT_PORT_START_INDEX,
                              pAppData->nfILComp,
                              pAppData->scILComp->outPortParams->nBufferSize,
                              pAppData->scILComp->outPortParams->pOutBuff[i]->
                              pBuffer);

      if (eError != OMX_ErrorNone)
      {
        printf ("Error in OMX_UseBuffer()-input Port State set : %s \n",
                IL_ClientErrorToStr (eError));
        goto EXIT;
      }
    }
    printf (" Noise Filter input port use buffer done \n ");

    /* in SDK conventionally output port allocates the buffers, nf would
       create the buffers which would be consumed by display component */
    /* buffer alloaction for output port */
    for (i = 0; i < pAppData->nfILComp->outPortParams->nBufferCountActual; i++)
    {
      eError = OMX_AllocateBuffer (pAppData->pNfHandle,
                                   &pAppData->nfILComp->
                                   outPortParams->pOutBuff[i],
                                   OMX_VFPC_OUTPUT_PORT_START_INDEX, pAppData,
                                   pAppData->nfILComp->
                                   outPortParams->nBufferSize);
      if (eError != OMX_ErrorNone)
      {
        printf ("Error in OMX_AllocateBuffer()-Output Port State set : %s \n",
                IL_ClientErrorToStr (eError));
        goto EXIT;
      }
    }

    printf (" Noise Filter outport buffers allocated \n ");

    semp_pend (pAppData->nfILComp->done_sem);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error %s:    WaitForState has timed out \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
    printf (" Noise Filter state IDLE \n ");
  }
/******************************************************************************/


  /* control component does not allocate any data buffers, It's interface is
     though as it is omx componenet */
  eError =
    OMX_SendCommand (pAppData->pctrlHandle, OMX_CommandStateSet,
                     OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" ctrl-dc state IDLE \n ");

  eError =
    OMX_SendCommand (pAppData->pDisHandle, OMX_CommandStateSet,
                     OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }
  /* Since display has only input port and buffers are already created by
     scalar/nf components, only use_buffer call is used at input port. there is 
     no output port in the display component */
  /* Display input will have same number of buffers as scalar output port */

  /* In case of SD display, noise filter is connnected to display */
  if(pAppData->displayId == 2) {  
    for (i = 0; i < pAppData->nfILComp->outPortParams->nBufferCountActual; i++)
    {

      eError = OMX_UseBuffer (pAppData->pDisHandle,
                              &pAppData->disILComp->inPortParams->pInBuff[i],
                              OMX_VFDC_INPUT_PORT_START_INDEX,
                              pAppData->disILComp,
                              pAppData->nfILComp->outPortParams->nBufferSize,
                              pAppData->nfILComp->outPortParams->pOutBuff[i]->
                              pBuffer);
  }
 } 
  else {
    for (i = 0; i < pAppData->scILComp->outPortParams->nBufferCountActual; i++)
    {
      eError = OMX_UseBuffer (pAppData->pDisHandle,
                              &pAppData->disILComp->inPortParams->pInBuff[i],
                              OMX_VFDC_INPUT_PORT_START_INDEX,
                              pAppData->disILComp,
                              pAppData->scILComp->outPortParams->nBufferSize,
                              pAppData->scILComp->outPortParams->pOutBuff[i]->
                              pBuffer);
  }
    if (eError != OMX_ErrorNone)
    {
      printf ("Error in Display OMX_UseBuffer()- %s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display state IDLE \n ");

  /* change state tho execute, so that component can accept buffers from IL
     client. Please note the ordering of components is from consumer to
     producer component i.e. display-scalar-(nf)-decoder */
  eError =
    OMX_SendCommand (pAppData->pctrlHandle, OMX_CommandStateSet,
                     OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display control state execute \n ");

  /* change state to execute so that buffers processing can start */
  eError =
    OMX_SendCommand (pAppData->pDisHandle, OMX_CommandStateSet,
                     OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error from SendCommand-Executing State set :%s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display state execute \n ");

 /* If noise filter is in chain, move state to execute */
 
 if (2 == pAppData->displayId ) {
    /* change state to execute so that buffers processing can start */
    eError =
      OMX_SendCommand (pAppData->pNfHandle, OMX_CommandStateSet,
                       OMX_StateExecuting, NULL);
    if (eError != OMX_ErrorNone)
    {
      printf ("Error from SendCommand-Executing State set :%s \n",
              IL_ClientErrorToStr (eError));
      goto EXIT;
    }
    semp_pend (pAppData->nfILComp->done_sem);
  }

  /* change state to execute so that buffers processing can start */
  eError =
    OMX_SendCommand (pAppData->pScHandle, OMX_CommandStateSet,
                     OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error from SendCommand-Executing State set :%s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  semp_pend (pAppData->scILComp->done_sem);

  printf (" scalar state execute \n ");
  /* change state to execute so that buffers processing can start */
  eError =
    OMX_SendCommand (pAppData->pCapHandle, OMX_CommandStateSet,
                     OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
    printf ("Error from SendCommand-Executing State set :%s \n",
            IL_ClientErrorToStr (eError));
    goto EXIT;
  }

  semp_pend (pAppData->capILComp->done_sem);

  printf (" capture state execute \n ");
  /* Create Graphics Thread */
  if (args->gfx)
  {
    /* Set the Graphics (FB) node to be the same as for the video display 
       chosen*/
    pAppData->gfx.gfxId = pAppData->displayId;
    
    pthread_attr_init (&pAppData->gfx.ThreadAttr);
    if (0 !=
        pthread_create (&pAppData->gfx.ThrdId,
                        &pAppData->gfx.ThreadAttr,
                        (ILC_StartFcnPtr) IL_ClientFbDevAppTask, &(pAppData->gfx)))
    {
      printf ("Create_Task failed !: %s", "IL_ClientFbDevAppTask");
      goto EXIT;
    }
    printf (" Graphics thread Created \n ");
  }
#if 0
  /* Create thread for reading bitstream and passing the buffers to decoder
     component */
  /* This thread would take the h264 elementary stream , parses it, give a
     frame to decoder at the input port. decoder */
  pthread_attr_init (&pAppData->decILComp->ThreadAttr);

  if (0 !=
      pthread_create (&pAppData->decILComp->inDataStrmThrdId,
                      &pAppData->decILComp->ThreadAttr,
                      (ILC_StartFcnPtr) IL_ClientInputBitStreamReadTask, pAppData))
  {
    printf ("Create_Task failed !");
    goto EXIT;
  }

  printf (" file read thread created \n ");
#endif


  pthread_attr_init (&pAppData->capILComp->ThreadAttr);

  if (0 !=
      pthread_create (&pAppData->capILComp->connDataStrmThrdId,
                      &pAppData->capILComp->ThreadAttr,
                      (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pAppData->capILComp))
  {
    printf ("Create_Task failed !");
    goto EXIT;
  }
  printf (" capture connect thread created \n ");

#if 0
  pthread_attr_init (&pAppData->decILComp->ThreadAttr);

  /* These are thread create for each component to pass the buffers to each
     other. this thread function reads the buffers from pipe and feeds it to
     componenet or for processed buffers, passes the buffers to connceted
     component */
  if (0 !=
      pthread_create (&pAppData->decILComp->connDataStrmThrdId,
                      &pAppData->decILComp->ThreadAttr,
                      (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pAppData->decILComp))
  {
    printf ("Create_Task failed !");
    goto EXIT;
  }

  printf (" decoder connect thread created \n ");
#endif

  pthread_attr_init (&pAppData->scILComp->ThreadAttr);

  if (0 !=
      pthread_create (&pAppData->scILComp->connDataStrmThrdId,
                      &pAppData->scILComp->ThreadAttr,
                      (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pAppData->scILComp))
  {
    printf ("Create_Task failed !");
    goto EXIT;
  }

  printf (" scalar connect thread created \n ");
  
  if(2 == pAppData->displayId ) {
  
    pthread_attr_init (&pAppData->nfILComp->ThreadAttr);

    if (0 !=
        pthread_create (&pAppData->nfILComp->connDataStrmThrdId,
                        &pAppData->nfILComp->ThreadAttr,
                        (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pAppData->nfILComp))
    {
      printf ("Create_Task failed !");
      goto EXIT;
    }

    printf (" Noise Filter connect thread created \n ");
  }

  pthread_attr_init (&pAppData->disILComp->ThreadAttr);

  if (0 !=
      pthread_create (&pAppData->disILComp->connDataStrmThrdId,
                      &pAppData->disILComp->ThreadAttr,
                      (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pAppData->disILComp))
  {
    printf ("Create_Task failed !");
    goto EXIT;
  }
  printf (" display connect thread created \n ");

  printf (" executing the appliaction now!!!\n ");

  /* Waiting for this semaphore to be posted by the bitstream read thread */
  /*if(2 == pAppData->displayId ) { 
   semp_pend (pAppData->nfILComp->eos);
  }
  else {
   semp_pend (pAppData->disILComp->eos);
   }*/

	return 1;

EXIT:
  return (0);
}

//----------------------/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Int Decode_Display_Stop ()
{

  OMX_U32 i;
  OMX_S32 ret_value;
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  IL_CLIENT_PIPE_MSG pipeMsg;
  OMX_VIDEO_CODINGTYPE coding;
  gILClientExit = OMX_FALSE;

  printf (" tearing down the capture-display example\n ");

  /* change the state to idle */
  /* before changing state to idle, buffer communication to component should be
	 stoped , writing an exit messages to threads */

  pipeMsg.cmd = IL_CLIENT_PIPE_CMD_EXIT;

  write (pAppData->capILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  write (pAppData->scILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  if(pAppData->displayId  == 2) {
	write (pAppData->nfILComp->localPipe[1],
		   &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));
  }

  write (pAppData->disILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

	/* change state to idle so that buffers processing would stop */
  eError =
	OMX_SendCommand (pAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pAppData->capILComp->done_sem);
  printf (" capture state idle \n ");

  /* change state to idle so that buffers processing can stop */
  eError =
	OMX_SendCommand (pAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pAppData->scILComp->done_sem);

  printf (" Scalar state idle \n ");

  if(2 == pAppData->displayId ) {
	/* change state to idle so that buffers processing can stop */
	eError =
	  OMX_SendCommand (pAppData->pNfHandle, OMX_CommandStateSet,
					   OMX_StateIdle, NULL);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error from SendCommand-Idle State set :%s \n",
			  IL_ClientErrorToStr (eError));
	  goto EXIT;
	}

	semp_pend (pAppData->nfILComp->done_sem);

	printf (" Noise Filter state idle \n ");
  }
  /* change state to execute so that buffers processing can stop */
  eError =
	OMX_SendCommand (pAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display state idle \n ");

  eError =
	OMX_SendCommand (pAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display control state idle \n ");

  /* change the state to loded */

   eError =
	OMX_SendCommand (pAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pCapHandle,
					  OMX_VFCC_OUTPUT_PORT_START_INDEX,
					  pAppData->capILComp->outPortParams->pOutBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pAppData->capILComp->done_sem);

  printf (" capture state loaded \n ");

  eError =
	OMX_SendCommand (pAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pAppData->scILComp->inPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pScHandle, OMX_VFPC_INPUT_PORT_START_INDEX,
					  pAppData->scILComp->inPortParams->pInBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  for (i = 0; i < pAppData->scILComp->outPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pScHandle, OMX_VFPC_OUTPUT_PORT_START_INDEX,
					  pAppData->scILComp->outPortParams->pOutBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pAppData->scILComp->done_sem);

  printf (" Scalar state loaded \n ");

  if(2 == pAppData->displayId ) {
  eError =
	OMX_SendCommand (pAppData->pNfHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pAppData->nfILComp->inPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pNfHandle, OMX_VFPC_INPUT_PORT_START_INDEX,
					  pAppData->nfILComp->inPortParams->pInBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  for (i = 0; i < pAppData->nfILComp->outPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pNfHandle, OMX_VFPC_OUTPUT_PORT_START_INDEX,
					  pAppData->nfILComp->outPortParams->pOutBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pAppData->nfILComp->done_sem);

  printf (" Noise Filter state loaded \n ");

  }

  eError =
	OMX_SendCommand (pAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pAppData->disILComp->inPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pAppData->pDisHandle, OMX_VFDC_INPUT_PORT_START_INDEX,
					  pAppData->disILComp->inPortParams->pInBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" display state loaded \n ");

  /* control component does not alloc/free any data buffers, It's interface is
	 though as it is omx componenet */
  eError =
	OMX_SendCommand (pAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateLoaded State set : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pAppData->disILComp->done_sem);

  printf (" ctrl-dc state loaded \n ");

  /* free handle for all component */

  eError = OMX_FreeHandle (pAppData->pCapHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }
  printf (" capture free handle \n");

  eError = OMX_FreeHandle (pAppData->pScHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" scalar free handle \n");

  if (2 == pAppData->displayId ) {
	eError = OMX_FreeHandle (pAppData->pNfHandle);
	if ((eError != OMX_ErrorNone))
	{
	  printf ("Error in Free Handle function : %s \n",
			  IL_ClientErrorToStr (eError));
	  goto EXIT;
	}
	printf (" Noise Filter free handle \n");
  }

  eError = OMX_FreeHandle (pAppData->pDisHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" display free handle \n");

  eError = OMX_FreeHandle (pAppData->pctrlHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			IL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" ctrl-dc free handle \n");

  if (pAppData->fIn != NULL)
  {
	fclose (pAppData->fIn);
	pAppData->fIn = NULL;
  }

  /* terminate the threads */
/*  if (args->gfx)
  {
	pAppData->gfx.terminateGfx = 1;
	while (!pAppData->gfx.terminateDone);
	pthread_join (pAppData->gfx.ThrdId, (void **) &ret_value);
  }*/
  pthread_join (pAppData->capILComp->connDataStrmThrdId, (void **) &ret_value);

  pthread_join (pAppData->scILComp->connDataStrmThrdId, (void **) &ret_value);

  if ( 2 == pAppData->displayId ) {
	pthread_join (pAppData->nfILComp->connDataStrmThrdId, (void **) &ret_value);
  }
  pthread_join (pAppData->disILComp->connDataStrmThrdId, (void **) &ret_value);

  IL_ClientDeInit (pAppData);

  printf ("IL Client deinitialized \n");

  printf (" example exit \n");

EXIT:
  return (0);
}

/* Nothing beyond this point */
