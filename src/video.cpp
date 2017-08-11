
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




#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <xdc/std.h>
#include <memory.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

/*-------------------------program files -------------------------------------*/

#include "ti/omx/interfaces/openMaxv11/OMX_Core.h"
#include "ti/omx/interfaces/openMaxv11/OMX_Component.h"
#include "OMX_TI_Common.h"
//#include "vpss/ilclient.h"
//#include "vpss/ilclient_utils.h"
extern "C"{
#include "vpss/dm814x/platform_utils.h"
}
#include <omx_vfcc.h>
#include <omx_vdec.h>
#include <omx_vfpc.h>
#include <omx_vfdc.h>
#include <omx_ctrl.h>
#include <OMX_TI_Index.h>
#include "OMX_TI_Video.h"

#include <fcntl.h>
#include <poll.h>
extern "C" {
#include "vpss/semp.h"
}

#include "video.h"
#include "camera.h"

/*--------------------------- defines ----------------------------------------*/
/* Align address "a" at "b" boundary */
#define UTIL_ALIGN(a,b)  ((((uint32_t)(a)) + (b)-1) & (~((uint32_t)((b)-1))))

#define HD_WIDTH       (1920)
#define HD_HEIGHT      (1080)

/* DM8168 PG1.1 SD display takes only 720 as width */

#define SD_WIDTH       (720)


#define CROP_START_X    0
#define CROP_START_Y    0


OMX_U8 PADX;
OMX_U8 PADY;

OMX_BOOL vgILClientExit = OMX_FALSE;
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
OMX_ERRORTYPE VIL_ClientCbEventHandler (OMX_HANDLETYPE hComponent,
											  OMX_PTR ptrAppData,
											  OMX_EVENTTYPE eEvent,
											  OMX_U32 nData1, OMX_U32 nData2,
											  OMX_PTR pEventData)
{
  IL_CLIENT_COMP_PRIVATE *comp;

  comp = (IL_CLIENT_COMP_PRIVATE *)ptrAppData;

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
			VIL_ClientErrorToStr ((OMX_ERRORTYPE)nData1), nData1);
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

OMX_ERRORTYPE VIL_ClientCbEmptyBufferDone (OMX_HANDLETYPE hComponent,
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

OMX_ERRORTYPE VIL_ClientCbFillBufferDone (OMX_HANDLETYPE hComponent,
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
  VIL_ClientUseInitialOutputResources (thisComp);

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
		err = VIL_ClientProcessPipeCmdETB (thisComp, &pipeMsg);
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
		err = VIL_ClientProcessPipeCmdFTB (thisComp, &pipeMsg);

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
		VIL_ClientProcessPipeCmdEBD (thisComp, &pipeMsg);

		break;

	  case IL_CLIENT_PIPE_CMD_FBD:
		VIL_ClientProcessPipeCmdFBD (thisComp, &pipeMsg);
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

OMX_ERRORTYPE VIL_ClientProcessPipeCmdETB (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferIn;

  /* search its own buffer header based on submitted by connected comp */
  VIL_ClientUtilGetSelfBufHeader (thisComp, pipeMsg->bufHeader.pBuffer,
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

OMX_ERRORTYPE VIL_ClientProcessPipeCmdFTB (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE *pBufferOut;

  /* search its own buffer header based on submitted by connected comp */
  VIL_ClientUtilGetSelfBufHeader (thisComp, pipeMsg->bufHeader.pBuffer,
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

OMX_ERRORTYPE VIL_ClientProcessPipeCmdEBD (IL_CLIENT_COMP_PRIVATE *thisComp,
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

OMX_ERRORTYPE VIL_ClientProcessPipeCmdFBD (IL_CLIENT_COMP_PRIVATE *thisComp,
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
void VIL_ClientSIGINTHandler(int sig)
{
 vgILClientExit = OMX_TRUE;
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

IL_Client *pVAppData = NULL;

UInt32 Video::startVideo ()
{

  OMX_U32 i;

  OMX_ERRORTYPE eError = OMX_ErrorNone;
  vgILClientExit = OMX_FALSE;

  pVAppData = NULL;

  printf (" Client Init \n");

  /* Initialize application specific data structures and buffer management
	 data structure */
	VIL_ClientInit (&pVAppData) ;

  printf (" Callback init \n");

  /* Initialize application / IL Client callback functions */
  /* Callbacks are passed during getHandle call to component, Component uses
	 these callaback to communicate with IL Client */
  /* event handler is to handle the state changes , omx commands and any
	 message for IL client */
  pVAppData->pCb.EventHandler = VIL_ClientCbEventHandler;

  /* Empty buffer done is data callback at the input port, where component lets
	 the application know that buffer has been consumned, this is not
	 applicable if there is no input port in the component */
  pVAppData->pCb.EmptyBufferDone = VIL_ClientCbEmptyBufferDone;

  /* fill buffer done is callback at the output port, where component lets the
	 application know that an output buffer is available with the processed data
   */
  pVAppData->pCb.FillBufferDone = VIL_ClientCbFillBufferDone;

  printf (" Get capture handle \n");

/******************************************************************************/
  /* Create the capture Component, component handle would be returned component
	 name is unique and fixed for a componnet, callback are passed to
	 component in this function. component would be in loaded state post this
	 call */

  eError =
	OMX_GetHandle (&pVAppData->pCapHandle,
				   (OMX_STRING) "OMX.TI.VPSSM3.VFCC", pVAppData->capILComp,
				   &pVAppData->pCb);

  printf (" capture compoenent is created \n");

  if ((eError != OMX_ErrorNone) || (pVAppData->pCapHandle == NULL))
  {
	printf ("Error in Get Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  pVAppData->capILComp->handle = pVAppData->pCapHandle;

  /* Configute the capture componet */
  /* calling OMX_Setparam in this function */
  VIL_ClientSetCaptureParams (pVAppData);

  printf ("enable capture output port \n");

  OMX_SendCommand (pVAppData->pCapHandle, OMX_CommandPortEnable,
				   OMX_VFCC_OUTPUT_PORT_START_INDEX, NULL);

  semp_pend (pVAppData->capILComp->port_sem);

/******************************************************************************/
  /* Create Scalar component, it creatd OMX compponnet for scalar writeback ,
	 Int this client we are passing the same callbacks to all the component */

  eError =
	OMX_GetHandle (&pVAppData->pScHandle,
				   (OMX_STRING) "OMX.TI.VPSSM3.VFPC.INDTXSCWB",
				   pVAppData->scILComp, &pVAppData->pCb);

  if ((eError != OMX_ErrorNone) || (pVAppData->pScHandle == NULL))
  {
	printf ("Error in Get Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  pVAppData->scILComp->handle = pVAppData->pScHandle;

  printf (" scalar compoenent is created \n");

  /* omx calls are made in this function for setting the parameters for scalar
	 component, For clarity purpose it is written as separate function */

  eError = VIL_ClientSetScalarParams (pVAppData);
  if ((eError != OMX_ErrorNone))
  {
	goto EXIT;
  }

  /* enable input and output port */
  /* as per openmax specs all the ports should be enabled by default but EZSDK
	 OMX component does not enable it hence we manually need to enable it. */

  printf ("enable scalar input port \n");
  OMX_SendCommand (pVAppData->pScHandle, OMX_CommandPortEnable,
				   OMX_VFPC_INPUT_PORT_START_INDEX, NULL);

  /* wait for both ports to get enabled, event handler would be notified from
	 the component after enabling the port, which inturn would post this
	 semaphore */
  semp_pend (pVAppData->scILComp->port_sem);

  printf ("enable scalar output port \n");
  OMX_SendCommand (pVAppData->pScHandle, OMX_CommandPortEnable,
				   OMX_VFPC_OUTPUT_PORT_START_INDEX, NULL);
  semp_pend (pVAppData->scILComp->port_sem);

/*****************************************************************************/

/******************************************************************************/
/* Create and Configure the display component. It will use VFDC component on  */
/* media controller.                                                          */
/******************************************************************************/

  /* Create the display component */
  /* getting display component handle */
  eError =
	OMX_GetHandle (&pVAppData->pDisHandle, "OMX.TI.VPSSM3.VFDC",
				   pVAppData->disILComp, &pVAppData->pCb);
  if (eError != OMX_ErrorNone)
	ERROR ("failed to get handle\n");

  printf ("found handle %p for component %s \n", pVAppData->pDisHandle,
		  "OMX.TI.VPSSM3.VFDC");

  pVAppData->disILComp->handle = pVAppData->pDisHandle;

  printf (" got display handle \n");
  /* getting display controller component handle, Display contrller is
	 implemented as an OMX component, however it does not have any input or
	 output ports. It is used only for controling display hw */
  eError =
	OMX_GetHandle (&pVAppData->pctrlHandle, "OMX.TI.VPSSM3.CTRL.DC",
				   pVAppData->disILComp, &pVAppData->pCb);
  if (eError != OMX_ErrorNone)
	ERROR ("failed to get handle\n");

  printf ("found handle %p for component %s\n", pVAppData->pctrlHandle,
		  "OMX.TI.VPSSM3.CTRL.DC");

  /* omx calls are made in this function for setting the parameters for display
	 component, For clarity purpose it is written as separate function */

  eError = VIL_ClientSetDisplayParams(pVAppData);
  if ((eError != OMX_ErrorNone))
  {
	goto EXIT;
  }

  /* as per openmax specs all the ports should be enabled by default but EZSDK
	 OMX component does not enable it hence we manually need to enable it */
  printf ("enable input port \n");

  OMX_SendCommand (pVAppData->pDisHandle, OMX_CommandPortEnable,
				   OMX_VFDC_INPUT_PORT_START_INDEX, NULL);

  /* wait for port to get enabled, event handler would be notified from the
	 component after enabling the port, which inturn would post this semaphore */
  semp_pend (pVAppData->disILComp->port_sem);

/******************************************************************************/

  /* Connect the decoder to scalar, This application uses 'pipe' to pass the
	 buffers between different components. each compponent has a local pipe,
	 which It reads for taking buffers. By connecting this functions informs
	 about local pipe to other component, so that other component can pass
	 buffers to this 'remote' pipe */

  printf (" connect call for capture-scalar \n ");

  VIL_ClientConnectComponents (pVAppData->capILComp, OMX_VFCC_OUTPUT_PORT_START_INDEX,
							  pVAppData->scILComp,
							  OMX_VFPC_INPUT_PORT_START_INDEX);

  printf (" connect call for scalar-display \n ");
  /* Connect the scalar to display */
  VIL_ClientConnectComponents (pVAppData->scILComp,
							  OMX_VFPC_OUTPUT_PORT_START_INDEX,
							  pVAppData->disILComp,
							  OMX_VFDC_INPUT_PORT_START_INDEX);

/******************************************************************************/

  /* OMX_SendCommand expecting OMX_StateIdle, after this command component
	 would create codec, and will wait for all buffers to be allocated as per
	 omx buffers are created during loaded to Idle transition ( if ports are
	 enabled ) */
  eError =
	OMX_SendCommand (pVAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

 /* Allocate I/O Buffers; componnet would allocated buffers and would return
	 the buffer header containing the pointer to buffer */
  for (i = 0; i < pVAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {
	eError = OMX_AllocateBuffer (pVAppData->pCapHandle,
								 &pVAppData->capILComp->outPortParams->
								 pOutBuff[i],
								 OMX_VFCC_OUTPUT_PORT_START_INDEX, pVAppData,
								 pVAppData->capILComp->outPortParams->
								 nBufferSize);

	if (eError != OMX_ErrorNone)
	{
	  printf
		("Capture: Error in OMX_AllocateBuffer() : %s \n",
		 VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }
  printf (" Capture outport buffers allocated \n ");

  semp_pend (pVAppData->capILComp->done_sem);

  printf (" Capture is in IDLE state \n");

/******************************************************************************/
  eError =
	OMX_SendCommand (pVAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  /* since capture is connected to scalar, buffers are supplied by capture to
	 scalar, so scalar does not allocate the buffers. Howvere it is informed to
	 use the buffers created by capture. scalar component would create only
	 buffer headers coreesponding to these buffers */

  for (i = 0; i < pVAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {

	eError = OMX_UseBuffer (pVAppData->pScHandle,
							&pVAppData->scILComp->inPortParams->pInBuff[i],
							OMX_VFPC_INPUT_PORT_START_INDEX,
							pVAppData->scILComp,
							pVAppData->capILComp->outPortParams->nBufferSize,
							pVAppData->capILComp->outPortParams->pOutBuff[i]->
							pBuffer);

	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_UseBuffer()-input Port State set : %s \n",
			  VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }
  printf (" Scalar input port use buffer done \n ");

  /* in SDK conventionally output port allocates the buffers, scalar would
	 create the buffers which would be consumed by display component */
  /* buffer alloaction for output port */
  for (i = 0; i < pVAppData->scILComp->outPortParams->nBufferCountActual; i++)
  {
	eError = OMX_AllocateBuffer (pVAppData->pScHandle,
								 &pVAppData->scILComp->
								 outPortParams->pOutBuff[i],
								 OMX_VFPC_OUTPUT_PORT_START_INDEX, pVAppData,
								 pVAppData->scILComp->
								 outPortParams->nBufferSize);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_AllocateBuffer()-Output Port State set : %s \n",
			  VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  printf (" scalar outport buffers allocated \n ");

  semp_pend (pVAppData->scILComp->done_sem);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error %s:    WaitForState has timed out \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  printf (" scalar state IDLE \n ");

/******************************************************************************/


  /* control component does not allocate any data buffers, It's interface is
	 though as it is omx componenet */
  eError =
	OMX_SendCommand (pVAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" ctrl-dc state IDLE \n ");

  eError =
	OMX_SendCommand (pVAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* Since display has only input port and buffers are already created by
	 scalar/nf components, only use_buffer call is used at input port. there is
	 no output port in the display component */
  /* Display input will have same number of buffers as scalar output port */



	for (i = 0; i < pVAppData->scILComp->outPortParams->nBufferCountActual; i++)
	{
	  eError = OMX_UseBuffer (pVAppData->pDisHandle,
							  &pVAppData->disILComp->inPortParams->pInBuff[i],
							  OMX_VFDC_INPUT_PORT_START_INDEX,
							  pVAppData->disILComp,
							  pVAppData->scILComp->outPortParams->nBufferSize,
							  pVAppData->scILComp->outPortParams->pOutBuff[i]->
							  pBuffer);
	}
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in Display OMX_UseBuffer()- %s \n",
			  VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}


  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display state IDLE \n ");

  /* change state tho execute, so that component can accept buffers from IL
	 client. Please note the ordering of components is from consumer to
	 producer component i.e. display-scalar-(nf)-decoder */
  eError =
	OMX_SendCommand (pVAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display control state execute \n ");

  /* change state to execute so that buffers processing can start */
  eError =
	OMX_SendCommand (pVAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Executing State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display state execute \n ");

  /* change state to execute so that buffers processing can start */
  eError =
	OMX_SendCommand (pVAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Executing State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->scILComp->done_sem);

  printf (" scalar state execute \n ");
  /* change state to execute so that buffers processing can start */
  eError =
	OMX_SendCommand (pVAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateExecuting, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Executing State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->capILComp->done_sem);

  printf (" capture state execute \n ");

  pthread_attr_init (&pVAppData->capILComp->ThreadAttr);

  if (0 !=
	  pthread_create (&pVAppData->capILComp->connDataStrmThrdId,
					  &pVAppData->capILComp->ThreadAttr,
					  (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pVAppData->capILComp))
  {
	printf ("Create_Task failed !");
	goto EXIT;
  }
  printf (" capture connect thread created \n ");

  pthread_attr_init (&pVAppData->scILComp->ThreadAttr);

  if (0 !=
	  pthread_create (&pVAppData->scILComp->connDataStrmThrdId,
					  &pVAppData->scILComp->ThreadAttr,
					  (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pVAppData->scILComp))
  {
	printf ("Create_Task failed !");
	goto EXIT;
  }

  printf (" scalar connect thread created \n ");


  pthread_attr_init (&pVAppData->disILComp->ThreadAttr);

  if (0 !=
	  pthread_create (&pVAppData->disILComp->connDataStrmThrdId,
					  &pVAppData->disILComp->ThreadAttr,
					  (ILC_StartFcnPtr) IL_ClientConnInConnOutTask, pVAppData->disILComp))
  {
	printf ("Create_Task failed !");
	goto EXIT;
  }
  printf (" display connect thread created \n ");

  printf (" executing the appliaction now!!!\n ");


  return 1;

EXIT:
  return (0);
}

//----------------------/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UInt32 Video::stopVideo()
{

  OMX_U32 i;
  OMX_S32 ret_value;
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  IL_CLIENT_PIPE_MSG pipeMsg;
  vgILClientExit = OMX_FALSE;

  printf (" tearing down the capture-display example\n ");

  /* change the state to idle */
  /* before changing state to idle, buffer communication to component should be
	 stoped , writing an exit messages to threads */

  pipeMsg.cmd = IL_CLIENT_PIPE_CMD_EXIT;

  write (pVAppData->capILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  write (pVAppData->scILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

  write (pVAppData->disILComp->localPipe[1],
		 &pipeMsg, sizeof (IL_CLIENT_PIPE_MSG));

	/* change state to idle so that buffers processing would stop */
  eError =
	OMX_SendCommand (pVAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->capILComp->done_sem);
  printf (" capture state idle \n ");

  /* change state to idle so that buffers processing can stop */
  eError =
	OMX_SendCommand (pVAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->scILComp->done_sem);

  printf (" Scalar state idle \n ");

  /* change state to execute so that buffers processing can stop */
  eError =
	OMX_SendCommand (pVAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display state idle \n ");

  eError =
	OMX_SendCommand (pVAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateIdle, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateIdle State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display control state idle \n ");

  /* change the state to loded */

   eError =
	OMX_SendCommand (pVAppData->pCapHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pVAppData->capILComp->outPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pVAppData->pCapHandle,
					  OMX_VFCC_OUTPUT_PORT_START_INDEX,
					  pVAppData->capILComp->outPortParams->pOutBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pVAppData->capILComp->done_sem);

  printf (" capture state loaded \n ");

  eError =
	OMX_SendCommand (pVAppData->pScHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pVAppData->scILComp->inPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pVAppData->pScHandle, OMX_VFPC_INPUT_PORT_START_INDEX,
					  pVAppData->scILComp->inPortParams->pInBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  for (i = 0; i < pVAppData->scILComp->outPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pVAppData->pScHandle, OMX_VFPC_OUTPUT_PORT_START_INDEX,
					  pVAppData->scILComp->outPortParams->pOutBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pVAppData->scILComp->done_sem);

  printf (" Scalar state loaded \n ");

  eError =
	OMX_SendCommand (pVAppData->pDisHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error from SendCommand-Idle State set :%s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  /* During idle-> loaded state transition buffers need to be freed up */
  for (i = 0; i < pVAppData->disILComp->inPortParams->nBufferCountActual; i++)
  {
	eError =
	  OMX_FreeBuffer (pVAppData->pDisHandle, OMX_VFDC_INPUT_PORT_START_INDEX,
					  pVAppData->disILComp->inPortParams->pInBuff[i]);
	if (eError != OMX_ErrorNone)
	{
	  printf ("Error in OMX_FreeBuffer : %s \n", VIL_ClientErrorToStr (eError));
	  goto EXIT;
	}
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" display state loaded \n ");

  /* control component does not alloc/free any data buffers, It's interface is
	 though as it is omx componenet */
  eError =
	OMX_SendCommand (pVAppData->pctrlHandle, OMX_CommandStateSet,
					 OMX_StateLoaded, NULL);
  if (eError != OMX_ErrorNone)
  {
	printf ("Error in SendCommand()-OMX_StateLoaded State set : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  semp_pend (pVAppData->disILComp->done_sem);

  printf (" ctrl-dc state loaded \n ");

  /* free handle for all component */

  eError = OMX_FreeHandle (pVAppData->pCapHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }
  printf (" capture free handle \n");

  eError = OMX_FreeHandle (pVAppData->pScHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" scalar free handle \n");

  eError = OMX_FreeHandle (pVAppData->pDisHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" display free handle \n");

  eError = OMX_FreeHandle (pVAppData->pctrlHandle);
  if ((eError != OMX_ErrorNone))
  {
	printf ("Error in Free Handle function : %s \n",
			VIL_ClientErrorToStr (eError));
	goto EXIT;
  }

  printf (" ctrl-dc free handle \n");

  /* terminate the threads */
  pthread_join (pVAppData->capILComp->connDataStrmThrdId, (void **) &ret_value);

  pthread_join (pVAppData->scILComp->connDataStrmThrdId, (void **) &ret_value);

  pthread_join (pVAppData->disILComp->connDataStrmThrdId, (void **) &ret_value);

  VIL_ClientDeInit (pVAppData);

  printf ("IL Client deinitialized \n");

  printf (" example exit \n");

EXIT:
  return (0);
}

/* Nothing beyond this point */


/* ========================================================================== */
/**
* IL_ClientInit() : This function is to allocate and initialize the application
*                   data structure. It is just to maintain application control.
*
* @param pAppData          : appliaction / client data Handle
*
*  @return
*
*
*/
/* ========================================================================== */

void Video::VIL_ClientInit (IL_Client **pAppData)
{
  int i;

  IL_Client *pAppDataPtr;
  IL_CLIENT_INPORT_PARAMS *inPortParamsPtr;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamsPtr;

  /* Allocating data structure for IL client structure / buffer management */

  pAppDataPtr = (IL_Client *) malloc (sizeof (IL_Client));
  memset (pAppDataPtr, 0x0, sizeof (IL_Client));

  /* update the user provided parameters */
/*  pAppDataPtr->nHeight = height;
  pAppDataPtr->nWidth = width;
  pAppDataPtr->nFrameRate = frameRate;
  pAppDataPtr->codingType = coding;
  pAppDataPtr->displayId = displayId;*/

 /* alloacte data structure for each component used in this IL Client */
  pAppDataPtr->capILComp =
	(IL_CLIENT_COMP_PRIVATE *) malloc (sizeof (IL_CLIENT_COMP_PRIVATE));
  memset (pAppDataPtr->capILComp, 0x0, sizeof (IL_CLIENT_COMP_PRIVATE));

  /* these semaphores are used for tracking the callbacks received from
	 component */
  pAppDataPtr->capILComp->eos = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->capILComp->eos, 0);

  pAppDataPtr->capILComp->done_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->capILComp->done_sem, 0);

  pAppDataPtr->capILComp->port_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->capILComp->port_sem, 0);

  /* alloacte data structure for each component used in this IL Cleint */
  pAppDataPtr->scILComp =
	(IL_CLIENT_COMP_PRIVATE *) malloc (sizeof (IL_CLIENT_COMP_PRIVATE));
  memset (pAppDataPtr->scILComp, 0x0, sizeof (IL_CLIENT_COMP_PRIVATE));

  /* these semaphores are used for tracking the callbacks received from
	 component */
  pAppDataPtr->scILComp->eos = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->scILComp->eos, 0);

  pAppDataPtr->scILComp->done_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->scILComp->done_sem, 0);

  pAppDataPtr->scILComp->port_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->scILComp->port_sem, 0);

  /* alloacte data structure for each component used in this IL Cleint */
  pAppDataPtr->nfILComp =
	(IL_CLIENT_COMP_PRIVATE *) malloc (sizeof (IL_CLIENT_COMP_PRIVATE));
  memset (pAppDataPtr->nfILComp, 0x0, sizeof (IL_CLIENT_COMP_PRIVATE));

  /* these semaphores are used for tracking the callbacks received from
	 component */
  pAppDataPtr->nfILComp->eos = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->nfILComp->eos, 0);

  pAppDataPtr->nfILComp->done_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->nfILComp->done_sem, 0);

  pAppDataPtr->nfILComp->port_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->nfILComp->port_sem, 0);

  /* alloacte data structure for each component used in this IL Cleint */
  pAppDataPtr->disILComp =
	(IL_CLIENT_COMP_PRIVATE *) malloc (sizeof (IL_CLIENT_COMP_PRIVATE));
  memset (pAppDataPtr->disILComp, 0x0, sizeof (IL_CLIENT_COMP_PRIVATE));

  /* these semaphores are used for tracking the callbacks received from
	 component */
  pAppDataPtr->disILComp->eos = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->disILComp->eos, 0);

  pAppDataPtr->disILComp->done_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->disILComp->done_sem, 0);

  pAppDataPtr->disILComp->port_sem = (semp_t*)malloc (sizeof (semp_t));
  semp_init (pAppDataPtr->disILComp->port_sem, 0);

  /* number of ports for each component, which this IL cleint will handle, this
	 will be equal to number of ports supported by component or less */
  pAppDataPtr->capILComp->numInport = 0;
  /* capture does not has i/p ports */
  pAppDataPtr->capILComp->numOutport = 1;
  pAppDataPtr->capILComp->startOutportIndex = 0;

  pAppDataPtr->scILComp->numInport = 1;
  pAppDataPtr->scILComp->numOutport = 1;

  /* VFPC OMX component support max 16 input / output ports, so o/p port index
	 starts at 16 */
  pAppDataPtr->scILComp->startOutportIndex = OMX_VFPC_NUM_INPUT_PORTS;

  pAppDataPtr->nfILComp->numInport = 1;
  pAppDataPtr->nfILComp->numOutport = 1;

  /* VFPC OMX component support max 16 input / output ports, so o/p port index
	 starts at 16 */
  pAppDataPtr->nfILComp->startOutportIndex = OMX_VFPC_NUM_INPUT_PORTS;

  pAppDataPtr->disILComp->numInport = 1;

  /* display does not has o/pports */
  pAppDataPtr->disILComp->numOutport = 0;
  pAppDataPtr->disILComp->startOutportIndex = 0;

  /* allocate data structure for input and output port params of IL client
	 component, It is for mainitaining data structure in IL Cleint only.
	 Components will have its own data structure inside omx components */
  pAppDataPtr->capILComp->outPortParams =
	(IL_CLIENT_OUTPORT_PARAMS*)malloc (sizeof (IL_CLIENT_OUTPORT_PARAMS) *
			pAppDataPtr->capILComp->numOutport);
  memset (pAppDataPtr->capILComp->outPortParams, 0x0,
		  sizeof (IL_CLIENT_OUTPORT_PARAMS) *
		  pAppDataPtr->capILComp->numOutport);

  pAppDataPtr->scILComp->inPortParams =
	(IL_CLIENT_INPORT_PARAMS*)malloc (sizeof (IL_CLIENT_INPORT_PARAMS) *
			pAppDataPtr->scILComp->numInport);

  memset (pAppDataPtr->scILComp->inPortParams, 0x0,
		  sizeof (IL_CLIENT_INPORT_PARAMS));

  pAppDataPtr->scILComp->outPortParams =
	(IL_CLIENT_OUTPORT_PARAMS*)malloc (sizeof (IL_CLIENT_OUTPORT_PARAMS) *
			pAppDataPtr->scILComp->numOutport);
  memset (pAppDataPtr->scILComp->outPortParams, 0x0,
		  sizeof (IL_CLIENT_OUTPORT_PARAMS));

  pAppDataPtr->nfILComp->inPortParams =
	(IL_CLIENT_INPORT_PARAMS*)malloc (sizeof (IL_CLIENT_INPORT_PARAMS) *
			pAppDataPtr->nfILComp->numInport);

  memset (pAppDataPtr->nfILComp->inPortParams, 0x0,
		  sizeof (IL_CLIENT_INPORT_PARAMS));

  pAppDataPtr->nfILComp->outPortParams =
	(IL_CLIENT_OUTPORT_PARAMS*)malloc (sizeof (IL_CLIENT_OUTPORT_PARAMS) *
			pAppDataPtr->nfILComp->numOutport);
  memset (pAppDataPtr->nfILComp->outPortParams, 0x0,
		  sizeof (IL_CLIENT_OUTPORT_PARAMS));

  pAppDataPtr->disILComp->inPortParams =
	(IL_CLIENT_INPORT_PARAMS*)malloc (sizeof (IL_CLIENT_INPORT_PARAMS) *
			pAppDataPtr->disILComp->numInport);

  memset (pAppDataPtr->disILComp->inPortParams, 0x0,
		  sizeof (IL_CLIENT_INPORT_PARAMS));

  /* specify some of the parameters, that will be used for initializing OMX
	 component parameters */

  for (i = 0; i < pAppDataPtr->capILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppDataPtr->capILComp->outPortParams + i;
	outPortParamsPtr->nBufferCountActual =
	  IL_CLIENT_CAPTURE_OUTPUT_BUFFER_COUNT;

	outPortParamsPtr->nBufferSize =
	  (maxImgXSize * maxImgYSize * 3) >> 1;

	/* this pipe will not be used in this application, as capture does not read
	   / write into file */
	pipe ((int *) outPortParamsPtr->opBufPipe);
  }

  /* each componet will have local pipe to take buffers from other component or
	 its own consumed buffer, so that it can be passed to other connected
	 components */
  pipe ((int *) pAppDataPtr->capILComp->localPipe);

  for (i = 0; i < pAppDataPtr->scILComp->numInport; i++)
  {
	inPortParamsPtr = pAppDataPtr->scILComp->inPortParams + i;
	inPortParamsPtr->nBufferCountActual = IL_CLIENT_SCALAR_INPUT_BUFFER_COUNT;
	/* since input of scalar is connected to output of capture, size is same as
	   capture o/p buffers */
	inPortParamsPtr->nBufferSize =
	  (maxImgXSize * maxImgYSize * 3) >> 1;

	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	pipe ((int *) inPortParamsPtr->ipBufPipe);
  }
  for (i = 0; i < pAppDataPtr->scILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppDataPtr->scILComp->outPortParams + i;
	outPortParamsPtr->nBufferCountActual = IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT;

	/* configure the buffer size to that of the display size, for custom
	   display this can be used to change width and height */
	outPortParamsPtr->nBufferSize = maxDisplayXRes * maxDisplayYRes * 2;

	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	pipe ((int *) outPortParamsPtr->opBufPipe);
  }

  /* each componet will have local pipe to take bufffes from other component or
	 its own consumed buffer, so that it can be passed to other conected
	 components */
  pipe ((int *) pAppDataPtr->scILComp->localPipe);

  for (i = 0; i < pAppDataPtr->nfILComp->numInport; i++)
  {
	inPortParamsPtr = pAppDataPtr->nfILComp->inPortParams + i;
	inPortParamsPtr->nBufferCountActual = IL_CLIENT_NF_INPUT_BUFFER_COUNT;
	/* since input of scalar is connected to output of decoder, size is same as
	   decoder o/p buffers */
	inPortParamsPtr->nBufferSize = SD_DISPLAY_HEIGHT * SD_DISPLAY_WIDTH  * 2;

	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	pipe ((int *) inPortParamsPtr->ipBufPipe);
  }
  for (i = 0; i < pAppDataPtr->nfILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppDataPtr->nfILComp->outPortParams + i;
	outPortParamsPtr->nBufferCountActual = IL_CLIENT_NF_OUTPUT_BUFFER_COUNT;
	/* scalar is configured for YUV 422 output, so buffer size is calculates as
	   follows */
	outPortParamsPtr->nBufferSize =
	(SD_DISPLAY_HEIGHT * SD_DISPLAY_WIDTH  * 3) >> 1;
	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	pipe ((int *) outPortParamsPtr->opBufPipe);
  }

  /* each componet will have local pipe to take bufffes from other component or
	 its own consumed buffer, so that it can be passed to other conected
	 components */
  pipe ((int *) pAppDataPtr->nfILComp->localPipe);

  for (i = 0; i < pAppDataPtr->disILComp->numInport; i++)
  {
	inPortParamsPtr = pAppDataPtr->disILComp->inPortParams + i;
	inPortParamsPtr->nBufferCountActual = IL_CLIENT_DISPLAY_INPUT_BUFFER_COUNT;
	inPortParamsPtr->nBufferSize =
	  maxDisplayXRes * maxDisplayYRes * 2;

	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	pipe ((int *) inPortParamsPtr->ipBufPipe);
  }

  /* each componet will have local pipe to take bufffes from other component or
	 its own consumed buffer, so that it can be passed to other conected
	 components */
  pipe ((int *) pAppDataPtr->disILComp->localPipe);

  /* populate the pointer for allocated data structure */
  *pAppData = pAppDataPtr;
}

/* ========================================================================== */
/**
* IL_ClientDeInit() : This function is to deinitialize the application
*                   data structure.
*
* @param pAppData          : appliaction / client data Handle
*  @return
*
*
*/
/* ========================================================================== */

void Video::VIL_ClientDeInit (IL_Client *pAppData)
{
  int i;
  IL_CLIENT_INPORT_PARAMS *inPortParamsPtr;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamsPtr;

  close ((int) pAppData->disILComp->localPipe);

  for (i = 0; i < pAppData->disILComp->numInport; i++)
  {
	inPortParamsPtr = pAppData->disILComp->inPortParams + i;
	/* this pipe will not be used in this application, as scalar does not read
	   / write into file */
	close ((int) inPortParamsPtr->ipBufPipe);
  }

  close ((int) pAppData->scILComp->localPipe);

  for (i = 0; i < pAppData->scILComp->numInport; i++)
  {
	inPortParamsPtr = pAppData->scILComp->inPortParams + i;
	/* this pipe is not used in this application, as scalar does not read /
	   write into file */
	close ((int) inPortParamsPtr->ipBufPipe);
  }
  for (i = 0; i < pAppData->scILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppData->scILComp->outPortParams + i;
	/* this pipe is not used in this application, as scalar does not read /
	   write into file */
	close ((int) outPortParamsPtr->opBufPipe);
  }

  close ((int) pAppData->nfILComp->localPipe);

  for (i = 0; i < pAppData->nfILComp->numInport; i++)
  {
	inPortParamsPtr = pAppData->nfILComp->inPortParams + i;
	/* this pipe is not used in this application, as scalar does not read /
	   write into file */
	close ((int) inPortParamsPtr->ipBufPipe);
  }
  for (i = 0; i < pAppData->nfILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppData->nfILComp->outPortParams + i;
	/* this pipe is not used in this application, as scalar does not read /
	   write into file */
	close ((int) outPortParamsPtr->opBufPipe);
  }

  close ((int) pAppData->capILComp->localPipe);

  for (i = 0; i < pAppData->capILComp->numOutport; i++)
  {
	outPortParamsPtr = pAppData->capILComp->outPortParams + i;
	/* this pipe will not be used in this application, as capture does not read
	   / write into file */
	close ((int) outPortParamsPtr->opBufPipe);
  }


  free (pAppData->disILComp->inPortParams);

  free (pAppData->scILComp->inPortParams);

  free (pAppData->scILComp->outPortParams);

  free (pAppData->nfILComp->inPortParams);

  free (pAppData->nfILComp->outPortParams);

 free(pAppData->capILComp->outPortParams);

  /* these semaphores are used for tracking the callbacks received from
	 component */
  semp_deinit (pAppData->scILComp->eos);
  free (pAppData->scILComp->eos);

  semp_deinit (pAppData->scILComp->done_sem);
  free (pAppData->scILComp->done_sem);

  semp_deinit (pAppData->scILComp->port_sem);
  free (pAppData->scILComp->port_sem);

  semp_deinit (pAppData->nfILComp->eos);
  free (pAppData->nfILComp->eos);

  semp_deinit (pAppData->nfILComp->done_sem);
  free (pAppData->nfILComp->done_sem);

  semp_deinit (pAppData->nfILComp->port_sem);
  free (pAppData->nfILComp->port_sem);

  semp_deinit (pAppData->disILComp->eos);
  free (pAppData->disILComp->eos);

  semp_deinit (pAppData->disILComp->done_sem);
  free (pAppData->disILComp->done_sem);

  semp_deinit (pAppData->disILComp->port_sem);
  free (pAppData->disILComp->port_sem);

  semp_deinit (pAppData->capILComp->eos);
  free(pAppData->capILComp->eos);

  semp_deinit (pAppData->capILComp->done_sem);
  free(pAppData->capILComp->done_sem);

  semp_deinit (pAppData->capILComp->port_sem);
  free(pAppData->capILComp->port_sem);

  free (pAppData->capILComp);

  free (pAppData->scILComp);

  free (pAppData->nfILComp);

  free (pAppData->disILComp);

  free (pAppData);

}

/* ========================================================================== */
/**
* IL_ClientErrorToStr() : Function to map the OMX error enum to string
*
* @param error   : OMX Error type
*
*  @return
*  String conversion of the OMX_ERRORTYPE
*
*/
/* ========================================================================== */

OMX_STRING VIL_ClientErrorToStr (OMX_ERRORTYPE error)
{
  OMX_STRING errorString;

  /* used for printing purpose */
  switch (error)
  {
	case OMX_ErrorNone:
	  errorString = "OMX_ErrorNone";
	  break;
	case OMX_ErrorInsufficientResources:
	  errorString = "OMX_ErrorInsufficientResources";
	  break;
	case OMX_ErrorUndefined:
	  errorString = "OMX_ErrorUndefined";
	  break;
	case OMX_ErrorInvalidComponentName:
	  errorString = "OMX_ErrorInvalidComponentName";
	  break;
	case OMX_ErrorComponentNotFound:
	  errorString = "OMX_ErrorComponentNotFound";
	  break;
	case OMX_ErrorInvalidComponent:
	  errorString = "OMX_ErrorInvalidComponent";
	  break;
	case OMX_ErrorBadParameter:
	  errorString = "OMX_ErrorBadParameter";
	  break;
	case OMX_ErrorNotImplemented:
	  errorString = "OMX_ErrorNotImplemented";
	  break;
	case OMX_ErrorUnderflow:
	  errorString = "OMX_ErrorUnderflow";
	  break;
	case OMX_ErrorOverflow:
	  errorString = "OMX_ErrorOverflow";
	  break;
	case OMX_ErrorHardware:
	  errorString = "OMX_ErrorHardware";
	  break;
	case OMX_ErrorInvalidState:
	  errorString = "OMX_ErrorInvalidState";
	  break;
	case OMX_ErrorStreamCorrupt:
	  errorString = "OMX_ErrorStreamCorrupt";
	  break;
	case OMX_ErrorPortsNotCompatible:
	  errorString = "OMX_ErrorPortsNotCompatible";
	  break;
	case OMX_ErrorResourcesLost:
	  errorString = "OMX_ErrorResourcesLost";
	  break;
	case OMX_ErrorNoMore:
	  errorString = "OMX_ErrorNoMore";
	  break;
	case OMX_ErrorVersionMismatch:
	  errorString = "OMX_ErrorVersionMismatch";
	  break;
	case OMX_ErrorNotReady:
	  errorString = "OMX_ErrorNotReady";
	  break;
	case OMX_ErrorTimeout:
	  errorString = "OMX_ErrorTimeout";
	  break;
	default:
	  errorString = "<unknown>";
  }

  return errorString;
}

/* ========================================================================== */
/**
* IL_ClientUtilGetSelfBufHeader() : This util function is to get buffer header
*                                   specific to one component, from the buffer
*                                   received from other component  .
*
* @param thisComp   : application component data structure
* @param pBuffer    : OMX buffer pointer
* @param type       : it is to identfy teh port type
* @param portIndex  : port number of the component
* @param pBufferOut : components buffer header correponding to pBuffer
*
*  @return
*  String conversion of the OMX_ERRORTYPE
*
*/
/* ========================================================================== */

OMX_ERRORTYPE VIL_ClientUtilGetSelfBufHeader (IL_CLIENT_COMP_PRIVATE *thisComp,
											 OMX_U8 *pBuffer,
											 ILCLIENT_PORT_TYPE type,
											 OMX_U32 portIndex,
											 OMX_BUFFERHEADERTYPE **pBufferOut)
{
  int i;
  IL_CLIENT_INPORT_PARAMS *inPortParamsPtr;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamsPtr;
  OMX_ERRORTYPE eError = OMX_ErrorNone;

  /* Check for input port buffer header queue */
  if (type == ILCLIENT_INPUT_PORT)
  {
	inPortParamsPtr = thisComp->inPortParams + portIndex;
	for (i = 0; i < inPortParamsPtr->nBufferCountActual; i++)
	{
	  if (pBuffer == inPortParamsPtr->pInBuff[i]->pBuffer)
	  {
		*pBufferOut = inPortParamsPtr->pInBuff[i];
	  }
	}
  }
  /* Check for output port buffer header queue */
  else
  {
	outPortParamsPtr =
	  thisComp->outPortParams + portIndex - thisComp->startOutportIndex;
	for (i = 0; i < outPortParamsPtr->nBufferCountActual; i++)
	{
	  if (pBuffer == outPortParamsPtr->pOutBuff[i]->pBuffer)
	  {
		*pBufferOut = outPortParamsPtr->pOutBuff[i];
	  }
	}
  }

  return (eError);
}

/* ========================================================================== */
/**
* IL_ClientConnectComponents() : This util function is to update the pipe
*                                information of other connected comonnet, so that
*                                buffers can be passed to connected component.
*
* @param handleCompPrivA   : application component data structure for producer
* @param compAPortOut      : port of producer comp
* @param handleCompPrivB   : application component data structure for consumer
* @param compBPortIn       : port number of the consumer component
*
*  @return
*  String conversion of the OMX_ERRORTYPE
*
*/
/* ========================================================================== */

OMX_ERRORTYPE Video::VIL_ClientConnectComponents (IL_CLIENT_COMP_PRIVATE
											*handleCompPrivA,
										  unsigned int compAPortOut,
										  IL_CLIENT_COMP_PRIVATE
											*handleCompPrivB,
										  unsigned int compBPortIn)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamPtr = NULL;
  IL_CLIENT_INPORT_PARAMS *inPortParamPtr = NULL;

  /* update the input port connect structure */
  outPortParamPtr =
	handleCompPrivA->outPortParams + compAPortOut -
	handleCompPrivA->startOutportIndex;

  inPortParamPtr = handleCompPrivB->inPortParams + compBPortIn;

  /* update input port component pipe info with connected port */
  inPortParamPtr->connInfo.remoteClient = handleCompPrivA;
  inPortParamPtr->connInfo.remotePort = compAPortOut;
  inPortParamPtr->connInfo.remotePipe[0] = handleCompPrivA->localPipe[0];
  inPortParamPtr->connInfo.remotePipe[1] = handleCompPrivA->localPipe[1];

  /* update output port component pipe info with connected port */
  outPortParamPtr->connInfo.remoteClient = handleCompPrivB;
  outPortParamPtr->connInfo.remotePort = compBPortIn;
  outPortParamPtr->connInfo.remotePipe[0] = handleCompPrivB->localPipe[0];
  outPortParamPtr->connInfo.remotePipe[1] = handleCompPrivB->localPipe[1];

  return eError;
}

/* ========================================================================== */
/**
* IL_ClientUseInitialOutputResources() :  This function gives initially all
*                                         output buffers to a component.
*                                         after consuming component would keep
*                                         in local pipe for connect thread use.
*
* @param pAppdata   : application data structure
*
*  @return
*  String conversion of the OMX_ERRORTYPE
*
*/
/* ========================================================================== */

OMX_ERRORTYPE
  VIL_ClientUseInitialOutputResources (IL_CLIENT_COMP_PRIVATE *thisComp)

{
  OMX_ERRORTYPE err = OMX_ErrorNone;
  unsigned int i = 0, j;
  IL_CLIENT_OUTPORT_PARAMS *outPortParamPtr = NULL;
  OMX_PARAM_PORTDEFINITIONTYPE param;

  memset (&param, 0, sizeof (param));

  OMX_INIT_PARAM (&param);

  /* Give output buffers to component which is limited by no of output buffers
	 available. Rest of the data will be written on the callback from output
	 data write thread */
  for (j = 0; j < thisComp->numOutport; j++)
  {
	param.nPortIndex = j + thisComp->startOutportIndex;

	OMX_GetParameter (thisComp->handle, OMX_IndexParamPortDefinition, &param);

	outPortParamPtr = thisComp->outPortParams + j;

	if (OMX_TRUE == param.bEnabled)
	{
	  if (outPortParamPtr->connInfo.remotePipe != NULL)
	  {

		for (i = 0; i < thisComp->outPortParams->nBufferCountActual; i++)
		{
		  /* Pass the output buffer to the component */
		  err =
			OMX_FillThisBuffer (thisComp->handle, outPortParamPtr->pOutBuff[i]);

		} /* for (i) */
	  } /* if (outPortParamPtr->...) */
	} /* if (OMX_TRUE...) */
  } /* for (j) */

  return err;
}


/* ========================================================================== */
/**
* IL_ClientSetCaptureParams() : Function to fill the port definition
* structures and call the Set_Parameter function on to the Capture
* Component
*
* @param pAppData   : Pointer to the application data
*
*  @return
*  OMX_ErrorNone = Successful
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE Video::VIL_ClientSetCaptureParams (IL_Client *pAppData)
{

  OMX_PARAM_VFCC_HWPORT_PROPERTIES sHwPortParam;

  OMX_PARAM_VFCC_HWPORT_ID sHwPortId;

  OMX_PARAM_BUFFER_MEMORYTYPE memTypeCfg;

  OMX_PARAM_PORTDEFINITIONTYPE paramPort;

  OMX_ERRORTYPE eError = OMX_ErrorNone;

  OMX_INIT_PARAM (&paramPort);

  /* set input height/width and color format */
  paramPort.nPortIndex = OMX_VFCC_OUTPUT_PORT_START_INDEX;
  OMX_GetParameter (pAppData->pCapHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  paramPort.nPortIndex = OMX_VFCC_OUTPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth = imgXSize;
  paramPort.format.video.nFrameHeight = imgYSize;
  paramPort.format.video.nStride = ((imgXSize+15) & 0xFFFFFFF0);	//Scaler stride must be a multiple of 16
  paramPort.nBufferCountActual = IL_CLIENT_CAPTURE_OUTPUT_BUFFER_COUNT;
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  /* Capture output in 420 format */
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
  paramPort.nBufferSize =
	(paramPort.format.video.nStride * imgYSize * 3) >> 1;

  printf ("Buffer Size computed: %d\n", (int) paramPort.nBufferSize);
  printf ("set input port params (width = %d, height = %d)",
		  (int) imgXSize, (int) imgYSize);
  OMX_SetParameter (pAppData->pCapHandle, OMX_IndexParamPortDefinition,
					&paramPort);

  /* Setting Memory type at output port to Raw Memory */
  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFCC_OUTPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pCapHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
	ERROR ("failed to set memory Type at output port\n");

  OMX_INIT_PARAM (&sHwPortId);
  /* capture on EIO card is component input at VIP1 port */
  sHwPortId.eHwPortId = OMX_VIDEO_CaptureHWPortVIP1_PORTA;
  eError = OMX_SetParameter (pAppData->pCapHandle,
							 (OMX_INDEXTYPE) OMX_TI_IndexParamVFCCHwPortID,
							 (OMX_PTR) & sHwPortId);

  OMX_INIT_PARAM (&sHwPortParam);

  sHwPortParam.eCaptMode = OMX_VIDEO_CaptureModeSC_DISCRETESYNC_ACTVID_VSYNC;
  sHwPortParam.eVifMode = OMX_VIDEO_CaptureVifMode_24BIT;
  sHwPortParam.eInColorFormat = OMX_COLOR_Format24bitRGB888;
  sHwPortParam.eScanType = OMX_VIDEO_CaptureScanTypeProgressive;
  sHwPortParam.nMaxHeight = imgYSize;
  sHwPortParam.bFieldMerged   = OMX_FALSE;

  sHwPortParam.nMaxWidth = imgXSize;
  sHwPortParam.nMaxChnlsPerHwPort = 1;

  eError = OMX_SetParameter (pAppData->pCapHandle,
							 (OMX_INDEXTYPE)
							 OMX_TI_IndexParamVFCCHwPortProperties,
							 (OMX_PTR) & sHwPortParam);

  if (eError != OMX_ErrorNone)
	ERROR ("failed to set video capture HW params \n");
/*
  if (pAppData->nFrameRate == 30)
  {
	OMX_CONFIG_VFCC_FRAMESKIP_INFO sCapSkipFrames;
	OMX_INIT_PARAM (&sCapSkipFrames);
	printf (" applying skip mask \n");

	sCapSkipFrames.frameSkipMask = 0x2AAAAAAA;
	eError = OMX_SetConfig (pAppData->pCapHandle,
							(OMX_INDEXTYPE) OMX_TI_IndexConfigVFCCFrameSkip,
							(OMX_PTR) & sCapSkipFrames);
  }
*/
  return (eError);
}

/* ========================================================================== */
/**
* IL_ClientSetScalarParams() : Function to fill the port definition
* structures and call the Set_Parameter function on to the scalar
* Component
*
* @param pAppData   : Pointer to the application data
*
*  @return
*  OMX_ErrorNone = Successful
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE Video::VIL_ClientSetScalarParams (IL_Client *pAppData)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_PARAM_BUFFER_MEMORYTYPE memTypeCfg;
  OMX_PARAM_PORTDEFINITIONTYPE paramPort;
  OMX_PARAM_VFPC_NUMCHANNELPERHANDLE sNumChPerHandle;
  OMX_CONFIG_ALG_ENABLE algEnable;
  OMX_CONFIG_VIDCHANNEL_RESOLUTION chResolution;

  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pScHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set memory Type at input port\n");
  }

  /* Setting Memory type at output port to Raw Memory */
  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pScHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set memory Type at output port\n");
  }

  /* set input height/width and color format */
  OMX_INIT_PARAM (&paramPort);
  paramPort.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;

  OMX_GetParameter (pAppData->pScHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  paramPort.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth = imgXSize;
  paramPort.format.video.nFrameHeight = imgYSize;

  /* Scalar is connceted to H264 decoder, whose stride is different than width*/
  paramPort.format.video.nStride = ((imgXSize+15) & 0xFFFFFFF0); //Scaler stride must be a multiple of 16
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
  paramPort.nBufferSize = (paramPort.format.video.nStride *
						   paramPort.format.video.nFrameHeight * 3) >> 1;

  paramPort.nBufferAlignment = 0;
  paramPort.bBuffersContiguous = OMX_FALSE;
  paramPort.nBufferCountActual = IL_CLIENT_SCALAR_INPUT_BUFFER_COUNT;
  printf ("set scaler input port params (width = %u, height = %u) \n",
		  (unsigned int) imgXSize, (unsigned int) imgYSize);
  eError = OMX_SetParameter (pAppData->pScHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  if(eError != OMX_ErrorNone)
  {
	ERROR(" Invalid INPUT color formats for Scalar \n");
	OMX_FreeHandle (pAppData->pDisHandle);
	return eError;
  }

  /* set output height/width and color format */
  OMX_INIT_PARAM (&paramPort);
  paramPort.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  OMX_GetParameter (pAppData->pScHandle, OMX_IndexParamPortDefinition,
					&paramPort);

  paramPort.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth = displayXRes;
  paramPort.format.video.nFrameHeight = displayYRes;
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;
  paramPort.nBufferAlignment = 0;
  paramPort.bBuffersContiguous = OMX_FALSE;
  paramPort.nBufferCountActual = IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT;
  /* scalar buffer pitch should be multiple of 16 */
  paramPort.format.video.nStride = ((displayXRes + 15) & 0xfffffff0) * 2;

  paramPort.nBufferSize =
	paramPort.format.video.nStride * paramPort.format.video.nFrameHeight;

  printf ("set scaler output port params (width = %d, height = %d) \n",
		  (int) paramPort.format.video.nFrameWidth,
		  (int) paramPort.format.video.nFrameHeight);

  eError = OMX_SetParameter (pAppData->pScHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  if(eError != OMX_ErrorNone)
  {
	ERROR(" Invalid OUTPUT color formats for Scalar \n");
	OMX_FreeHandle (pAppData->pDisHandle);
	return eError;
  }

  /* set number of channles */
  printf ("set number of channels \n");

  OMX_INIT_PARAM (&sNumChPerHandle);
  sNumChPerHandle.nNumChannelsPerHandle = 1;
  eError =
	OMX_SetParameter (pAppData->pScHandle,
					  (OMX_INDEXTYPE) OMX_TI_IndexParamVFPCNumChPerHandle,
					  &sNumChPerHandle);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set num of channels\n");
  }

  /* set VFPC input and output resolution information */
  printf ("set input resolution \n");

  OMX_INIT_PARAM (&chResolution);
  chResolution.Frm0Width = imgXSize;
  chResolution.Frm0Height = imgYSize;
  chResolution.Frm0Pitch = imgXSize;
  chResolution.Frm1Width = 0;
  chResolution.Frm1Height = 0;
  chResolution.Frm1Pitch = 0;
  chResolution.FrmStartX = imgStartX;
  chResolution.FrmStartY = imgStartY;
  chResolution.FrmCropWidth = imgCropX;
  chResolution.FrmCropHeight = imgCropY;
  chResolution.eDir = OMX_DirInput;
  chResolution.nChId = 0;

  eError =
	OMX_SetConfig (pAppData->pScHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigVidChResolution,
				   &chResolution);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set input channel resolution\n");
  }

  printf ("set output resolution \n");
  OMX_INIT_PARAM (&chResolution);
  chResolution.Frm0Width = displayWindowXSize;
  chResolution.Frm0Height = displayWindowYSize;
  chResolution.Frm0Pitch = ((displayWindowXSize + 15) & 0xfffffff0) * 2;
  chResolution.Frm1Width = 0;
  chResolution.Frm1Height = 0;
  chResolution.Frm1Pitch = 0;
  chResolution.FrmStartX = 0;
  chResolution.FrmStartY = 0;
  chResolution.FrmCropWidth = 0;
  chResolution.FrmCropHeight = 0;
  chResolution.eDir = OMX_DirOutput;
  chResolution.nChId = 0;

  eError =
	OMX_SetConfig (pAppData->pScHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigVidChResolution,
				   &chResolution);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set output channel resolution\n");
  }

  /* disable algo bypass mode */
  OMX_INIT_PARAM (&algEnable);
  algEnable.nPortIndex = 0;
  algEnable.nChId = 0;
  algEnable.bAlgBypass = OMX_FALSE;

  eError =
	OMX_SetConfig (pAppData->pScHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigAlgEnable, &algEnable);
  if (eError != OMX_ErrorNone)
	ERROR ("failed to disable algo by pass mode\n");

  return (eError);
}

/* ========================================================================== */
/**
* IL_ClientSetNfParams() : Function to fill the port definition
* structures and call the Set_Parameter function on to the scalar
* Component
*
* @param pAppData   : Pointer to the application data
*
*  @return
*  OMX_ErrorNone = Successful
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE Video::VIL_ClientSetNfParams (IL_Client *pAppData)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_PARAM_BUFFER_MEMORYTYPE memTypeCfg;
  OMX_PARAM_PORTDEFINITIONTYPE paramPort;
  OMX_PARAM_VFPC_NUMCHANNELPERHANDLE sNumChPerHandle;
  OMX_CONFIG_ALG_ENABLE algEnable;
  OMX_CONFIG_VIDCHANNEL_RESOLUTION chResolution;

  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pNfHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set memory Type at input port\n");
  }

  /* Setting Memory type at output port to Raw Memory */
  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pNfHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set memory Type at output port\n");
  }

  /* set input height/width and color format */
  OMX_INIT_PARAM (&paramPort);
  paramPort.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;

  OMX_GetParameter (pAppData->pNfHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  paramPort.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth  = SD_DISPLAY_WIDTH;
  paramPort.format.video.nFrameHeight = SD_DISPLAY_HEIGHT;

  /* Scalar is connceted to H264 decoder, whose stride is different than width*/
  paramPort.format.video.nStride = SD_DISPLAY_WIDTH * 2;
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;
  paramPort.nBufferSize = (paramPort.format.video.nStride *
						   paramPort.format.video.nFrameHeight * 2);

  paramPort.nBufferAlignment = 0;
  paramPort.bBuffersContiguous = OMX_FALSE;
  paramPort.nBufferCountActual = IL_CLIENT_NF_INPUT_BUFFER_COUNT;
  printf ("set input port params (width = %u, height = %u) \n",
		  (unsigned int) pAppData->nWidth, (unsigned int) pAppData->nHeight);
  OMX_SetParameter (pAppData->pNfHandle, OMX_IndexParamPortDefinition,
					&paramPort);

  /* set output height/width and color format */
  OMX_INIT_PARAM (&paramPort);
  paramPort.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  OMX_GetParameter (pAppData->pNfHandle, OMX_IndexParamPortDefinition,
					&paramPort);

  paramPort.nPortIndex = OMX_VFPC_OUTPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth  = SD_DISPLAY_WIDTH;
  paramPort.format.video.nFrameHeight = SD_DISPLAY_HEIGHT;
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
  paramPort.nBufferAlignment = 0;
  paramPort.bBuffersContiguous = OMX_FALSE;
  paramPort.nBufferCountActual = IL_CLIENT_NF_OUTPUT_BUFFER_COUNT;
  /* scalar buffer pitch should be multiple of 16 */
  paramPort.format.video.nStride = SD_DISPLAY_WIDTH;

  paramPort.nBufferSize =
   (paramPort.format.video.nStride * paramPort.format.video.nFrameHeight * 3 )>> 1;

  printf ("set output port params (width = %d, height = %d) \n",
		  (int) paramPort.format.video.nFrameWidth,
		  (int) paramPort.format.video.nFrameHeight);

  OMX_SetParameter (pAppData->pNfHandle, OMX_IndexParamPortDefinition,
					&paramPort);

  /* set number of channles */
  printf ("set number of channels \n");

  OMX_INIT_PARAM (&sNumChPerHandle);
  sNumChPerHandle.nNumChannelsPerHandle = 1;
  eError =
	OMX_SetParameter (pAppData->pNfHandle,
					  (OMX_INDEXTYPE) OMX_TI_IndexParamVFPCNumChPerHandle,
					  &sNumChPerHandle);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set num of channels\n");
  }

  /* set VFPC input and output resolution information */
  printf ("set input resolution \n");

  OMX_INIT_PARAM (&chResolution);
  chResolution.Frm0Width =  SD_DISPLAY_WIDTH;
  chResolution.Frm0Height = SD_DISPLAY_HEIGHT;
  chResolution.Frm0Pitch =  SD_DISPLAY_WIDTH * 2;
  chResolution.Frm1Width = 0;
  chResolution.Frm1Height = 0;
  chResolution.Frm1Pitch = 0;
  chResolution.FrmStartX = 0;
  chResolution.FrmStartY = 0;
  chResolution.FrmCropWidth = 0;
  chResolution.FrmCropHeight = 0;
  chResolution.eDir = OMX_DirInput;
  chResolution.nChId = 0;

  eError =
	OMX_SetConfig (pAppData->pNfHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigVidChResolution,
				   &chResolution);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set input channel resolution\n");
  }

  printf ("set output resolution \n");
  OMX_INIT_PARAM (&chResolution);
  chResolution.Frm0Width  = SD_DISPLAY_WIDTH;
  chResolution.Frm0Height = SD_DISPLAY_HEIGHT;
  chResolution.Frm0Pitch  = SD_DISPLAY_WIDTH;
  chResolution.Frm1Width = 0;
  chResolution.Frm1Height = 0;
  chResolution.Frm1Pitch = 0;
  chResolution.FrmStartX = 0;
  chResolution.FrmStartY = 0;
  chResolution.FrmCropWidth = 0;
  chResolution.FrmCropHeight = 0;
  chResolution.eDir = OMX_DirOutput;
  chResolution.nChId = 0;

  eError =
	OMX_SetConfig (pAppData->pNfHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigVidChResolution,
				   &chResolution);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set output channel resolution\n");
  }

  /* disable algo bypass mode */
  OMX_INIT_PARAM (&algEnable);
  algEnable.nPortIndex = 0;
  algEnable.nChId = 0;
  algEnable.bAlgBypass = OMX_FALSE;

  eError =
	OMX_SetConfig (pAppData->pNfHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigAlgEnable, &algEnable);
  if (eError != OMX_ErrorNone)
	ERROR ("failed to disable algo by pass mode\n");

  return (eError);
}


/* ========================================================================== */
/**
* IL_ClientSetDisplayParams() : Function to fill the port definition
* structures and call the Set_Parameter function on to the display
* Component
*
* @param pAppData   : Pointer to the application data
*
*  @return
*  OMX_ErrorNone = Successful
*
*  Other_value = Failed (Error code is returned)
*
*/
/* ========================================================================== */

OMX_ERRORTYPE Video::VIL_ClientSetDisplayParams (IL_Client *pAppData)
{
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OMX_PARAM_BUFFER_MEMORYTYPE memTypeCfg;
  OMX_PARAM_PORTDEFINITIONTYPE paramPort;
  //OMX_PARAM_VFPC_NUMCHANNELPERHANDLE sNumChPerHandle;
  OMX_PARAM_VFDC_DRIVERINSTID driverId;
  OMX_PARAM_VFDC_CREATEMOSAICLAYOUT mosaicLayout;
  OMX_CONFIG_VFDC_MOSAICLAYOUT_PORT2WINMAP port2Winmap;

  OMX_INIT_PARAM (&paramPort);

  /* set input height/width and color format */
  paramPort.nPortIndex = OMX_VFDC_INPUT_PORT_START_INDEX;
  OMX_GetParameter (pAppData->pDisHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  paramPort.nPortIndex = OMX_VFDC_INPUT_PORT_START_INDEX;
  paramPort.format.video.nFrameWidth = displayWindowXSize;
  paramPort.format.video.nFrameHeight = displayWindowYSize;
  paramPort.format.video.nStride = ((displayWindowXSize + 15) & 0xfffffff0) * 2;
  paramPort.nBufferCountActual = IL_CLIENT_DISPLAY_INPUT_BUFFER_COUNT;
  paramPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  paramPort.format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;

  paramPort.nBufferSize = paramPort.format.video.nStride * paramPort.format.video.nFrameHeight;

  printf ("Buffer Size computed: %d\n", (int) paramPort.nBufferSize);
  printf ("set input port params (width = %d, height = %d) \n",
		  (int) displayWindowXSize, (int) displayWindowYSize);

  eError =
	OMX_SetParameter (pAppData->pDisHandle, OMX_IndexParamPortDefinition,
					&paramPort);
  if(eError != OMX_ErrorNone)
  {
	ERROR("failed to set display params\n");
	OMX_FreeHandle (pAppData->pDisHandle);
	return eError;
  }
  /* --------------------------------------------------------------------------*
	 Supported display IDs by VFDC and DC are below The names will be renamed in
	 future releases as some of the driver names & interfaces will be changed in
	 future @ param OMX_VIDEO_DISPLAY_ID_HD0: 422P On-chip HDMI @ param
	 OMX_VIDEO_DISPLAY_ID_HD1: 422P HDDAC component output @ param
	 OMX_VIDEO_DISPLAY_ID_SD0: 420T/422T SD display (NTSC): Not supported yet.
	 ------------------------------------------------------------------------ */

  /* set the parameter to the disaply component to 1080P @60 mode */
  OMX_INIT_PARAM (&driverId);
  //Configure to user video output 1
   driverId.nDrvInstID = OMX_VIDEO_DISPLAY_ID_HD1;
   driverId.eDispVencMode = DISPLAY_VENC_MODE;

  eError =
	OMX_SetParameter (pAppData->pDisHandle,
					  (OMX_INDEXTYPE) OMX_TI_IndexParamVFDCDriverInstId,
					  &driverId);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set driver mode\n");
  }

  /* set the parameter to the disaply controller component to 1080P @60 mode */
  OMX_INIT_PARAM (&driverId);
  //Configure to user video output 1
   driverId.nDrvInstID = OMX_VIDEO_DISPLAY_ID_HD1;
   driverId.eDispVencMode = DISPLAY_VENC_MODE;

  eError =
	OMX_SetParameter (pAppData->pctrlHandle,
					  (OMX_INDEXTYPE) OMX_TI_IndexParamVFDCDriverInstId,
					  &driverId);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set controller driver mode\n");
  }

  IL_ClientSetSecondaryDisplayParams(pAppData);

  OMX_INIT_PARAM (&mosaicLayout);
  /* Configuring the first (and only) window */
  /* position of window can be changed by following cordinates, keeping in
	 center by default */

  mosaicLayout.sMosaicWinFmt[0].winStartX = displayWindowStartX;
  mosaicLayout.sMosaicWinFmt[0].winStartY = displayWindowStartY;
  mosaicLayout.sMosaicWinFmt[0].winWidth = displayWindowXSize;
  mosaicLayout.sMosaicWinFmt[0].winHeight = displayWindowYSize;
  mosaicLayout.sMosaicWinFmt[0].pitch[VFDC_YUV_INT_ADDR_IDX] =
	((displayWindowXSize + 15) & 0xfffffff0) * 2;
  mosaicLayout.sMosaicWinFmt[0].dataFormat = VFDC_DF_YUV422I_YVYU;
  mosaicLayout.sMosaicWinFmt[0].bpp = VFDC_BPP_BITS16;
  mosaicLayout.sMosaicWinFmt[0].priority = 0;
  mosaicLayout.nDisChannelNum = 0;
  /* Only one window in this layout, hence setting it to 1 */
  mosaicLayout.nNumWindows = 1;

  eError = OMX_SetParameter (pAppData->pDisHandle, (OMX_INDEXTYPE)
							 OMX_TI_IndexParamVFDCCreateMosaicLayout,
							 &mosaicLayout);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set mosaic window parameter\n");
  }

  /* map OMX port to window */
  OMX_INIT_PARAM (&port2Winmap);
  /* signifies the layout id this port2win mapping refers to */
  port2Winmap.nLayoutId = 0;
  /* Just one window in this layout, hence setting the value to 1 */
  port2Winmap.numWindows = 1;
  /* Only 1st input port used here */
  port2Winmap.omxPortList[0] = OMX_VFDC_INPUT_PORT_START_INDEX + 0;
  eError =
	OMX_SetConfig (pAppData->pDisHandle,
				   (OMX_INDEXTYPE) OMX_TI_IndexConfigVFDCMosaicPort2WinMap,
				   &port2Winmap);
  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to map port to windows\n");
  }

  /* Setting Memory type at input port to Raw Memory */
  printf ("setting input and output memory type to default\n");
  OMX_INIT_PARAM (&memTypeCfg);
  memTypeCfg.nPortIndex = OMX_VFPC_INPUT_PORT_START_INDEX;
  memTypeCfg.eBufMemoryType = OMX_BUFFER_MEMORY_DEFAULT;
  eError =
	OMX_SetParameter (pAppData->pDisHandle, (OMX_INDEXTYPE)OMX_TI_IndexParamBuffMemType,
					  &memTypeCfg);

  if (eError != OMX_ErrorNone)
  {
	ERROR ("failed to set memory Type at input port\n");
  }

  return (eError);
}

/* Nothing beyond this point */






Int32 Video::init(void)
{
	int err;

	printf("Opening frame GPIO\n");
	gpioFD = open("/sys/class/gpio/gpio51/value", O_RDONLY|O_NONBLOCK);

	if (-1 == gpioFD)
		return VIDEO_FILE_ERROR;

	gpioPoll.fd = gpioFD;
	gpioPoll.events = POLLPRI | POLLERR;

	printf("Starting frame thread\n");
	terminateGPIOThread = false;

	err = pthread_create(&frameThreadID, NULL, &frameThread, this);
	if(err)
		return VIDEO_THREAD_ERROR;


	printf("Video init done\n");
	return CAMERA_SUCCESS;
}

bool Video::setRunning(bool run)
{
	if(!running && run)
	{
		OMX_Init ();
		startVideo ();
		running = true;

	}
	else if(running && !run)
	{

		stopVideo ();
		OMX_Deinit();
		running = false;
	}
}

CameraErrortype Video::setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY)
{
	OMX_CONFIG_VIDCHANNEL_RESOLUTION chResolution;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	if(!running)
		return VIDEO_NOT_RUNNING;

	if(	startX + cropX > imgXSize ||
		startY + cropY > imgYSize)
		return VIDEO_RESOLUTION_ERROR;

	imgStartX = startX;	//These must be a multiple of 8
	imgStartY = startY;
	imgCropX = cropX;
	imgCropY = cropY;

	/* set VFPC input and output resolution information */
	printf ("set input resolution \n");

	OMX_INIT_PARAM (&chResolution);
	chResolution.Frm0Width = imgXSize;
	chResolution.Frm0Height = imgYSize;
	chResolution.Frm0Pitch = imgXSize;
	chResolution.Frm1Width = 0;
	chResolution.Frm1Height = 0;
	chResolution.Frm1Pitch = 0;
	chResolution.FrmStartX = imgStartX;
	chResolution.FrmStartY = imgStartY;
	chResolution.FrmCropWidth = imgCropX;
	chResolution.FrmCropHeight = imgCropY;
	chResolution.eDir = OMX_DirInput;
	chResolution.nChId = 0;

	eError =
	  OMX_SetConfig (pVAppData->pScHandle,
					 (OMX_INDEXTYPE) OMX_TI_IndexConfigVidChResolution,
					 &chResolution);
	if (eError != OMX_ErrorNone)
	{
	  printf ("failed to set input channel resolution\n");
	  return VIDEO_OMX_ERROR;
	}
	return CAMERA_SUCCESS;
}

CameraErrortype Video::setImagerResolution(UInt32 x, UInt32 y)
{
	imgXSize = x;
	imgYSize = y;
	imgStartX = 0;
	imgStartY = 0;
	imgCropX = x;
	imgCropY = y;

	//Depending on aspect ratio, set the display window appropriately
	if((y * MAX_FRAME_SIZE_H) > (x * MAX_FRAME_SIZE_V))	//If it's taller than the display aspect
	{
		displayWindowYSize = 480;
		displayWindowXSize = displayWindowYSize * x / y;
		if(displayWindowXSize & 1)	//If it's odd, round it up to be even
			displayWindowXSize++;
		displayWindowStartY = 0;
		displayWindowStartX = ((600 - displayWindowXSize) / 2) & 0xFFFFFFFE;	//Must be even

	}
	else
	{
		displayWindowXSize = 600;
		displayWindowYSize = displayWindowXSize * y / x;
		if(displayWindowYSize & 1)	//If it's odd, round it up to be even
			displayWindowYSize++;
		displayWindowStartX = 0;
		displayWindowStartY = (480 - displayWindowYSize) / 2;

	}
	return CAMERA_SUCCESS;
}

void Video::frameCB(void)
{


}



Video::Video()
{
	running = false;

	imgXSize = 1280;	//Input resolution coming from imager
	imgYSize = 1024;
	displayXRes = 800;	//Display resolution
	displayYRes = 480;

	//Window cropped from input and scaled
	imgStartX = 0;	//Top left pixel of cropped window taken from input
	imgStartY = 0;
	imgCropX = 1280;	//Size of cropped window
	imgCropY = 1024;

	//Scaler output
	displayWindowStartX = 0;		//Top left pixel location to place image within display resolution
	displayWindowStartY = 0;
	displayWindowXSize = 600;		//Resolution image is scaled to
	displayWindowYSize = 480;

	maxImgXSize = 1280;	//Input resolution coming from imager
	maxImgYSize = 1024;
	maxDisplayXRes = 800;	//Display resolution
	maxDisplayYRes = 600;

	frameCallback = NULL;


}

Video::~Video()
{
	terminateGPIOThread = true;
	pthread_join(frameThreadID, NULL);

	if(-1 != gpioFD)
		close(gpioFD);


}


void* frameThread(void *arg)
{
	Video * vInst = (Video *)arg;
	char buf[2];
	int ret;

	pthread_t this_thread = pthread_self();

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);

	printf("Trying to set frameThread realtime prio = %d", params.sched_priority);

	// Attempt to set thread real-time priority to the SCHED_FIFO policy
	ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
	if (ret != 0) {
		// Print the error
		printf("Unsuccessful in setting thread realtime prio");
		return (void *)0;
	}

	// Now verify the change in thread priority
	int policy = 0;
	ret = pthread_getschedparam(this_thread, &policy, &params);
	if (ret != 0) {
		printf("Couldn't retrieve real-time scheduling paramers");
		return (void *)0;
	}

	// Check the correct policy was applied
	if(policy != SCHED_FIFO) {
		printf("Scheduling is NOT SCHED_FIFO!");
	} else {
		printf("SCHED_FIFO OK");
	}

	// Print thread scheduling priority
	printf("Thread priority is %d", params.sched_priority);



	//Dummy read so poll blocks properly
	read(vInst->gpioPoll.fd, buf, sizeof(buf));

	while(!vInst->terminateGPIOThread)
	{
		if((ret = poll(&vInst->gpioPoll, 1, 200)) > 0)	//If we returned due to a GPIO event rather than a timeout
		{
			if (vInst->gpioPoll.revents & POLLPRI)
			{
				/* IRQ happened */
				lseek(vInst->gpioPoll.fd, 0, SEEK_SET);
				read(vInst->gpioPoll.fd, buf, sizeof(buf));
			}
			vInst->frameCB();	//The GPIO is high starting at the line before the first active video line in the frame.
								//This line goes high just after the frame address is registered by the video display hardware, so you have almost the full frame period to update it.
			if(vInst->frameCallback)
				(*vInst->frameCallback)(vInst->frameCallbackArg);	//Call user callback

		}
	}

	pthread_exit(NULL);
}
