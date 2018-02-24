#ifndef VIDEO_H
#define VIDEO_H

#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "types.h"

/*
typedef enum VideoErrortype
{
	VIDEO_ERROR_NONE = 0,
	VIDEO_NOT_RUNNING,
	VIDEO_RESOLUTION_ERROR,
	VIDEO_OMX_ERROR,
	VIDEO_FILE_ERROR,
	VIDEO_THREAD_ERROR
} VideoErrortype;
*/
//#define IL_CLIENT_DECODER_INPUT_BUFFER_COUNT   (4)
#define IL_CLIENT_CAPTURE_OUTPUT_BUFFER_COUNT  (6)//(12)
#define IL_CLIENT_SCALAR_INPUT_BUFFER_COUNT    IL_CLIENT_CAPTURE_OUTPUT_BUFFER_COUNT
#define IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT   (6)//(6)
#define IL_CLIENT_NF_INPUT_BUFFER_COUNT        IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT
#define IL_CLIENT_NF_OUTPUT_BUFFER_COUNT       IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT
#define IL_CLIENT_DISPLAY_INPUT_BUFFER_COUNT   IL_CLIENT_SCALAR_OUTPUT_BUFFER_COUNT

/* SD display is through NF in this example, and NF has limitation of multiple
   of 32 ; Netra PG1.1 SD width has to be 720 ; follwing is done for Scalar and NF*/

#define SD_DISPLAY_WIDTH    ((720 + 31) & 0xffffffe0)
#define SD_DISPLAY_HEIGHT   ((480 + 31) & 0xffffffe0)



/******************************************************************************\
 *      Includes
\******************************************************************************/
#include <stdint.h>
#include "vpss/semp.h"
//#include <vpss/es_parser.h>
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
  OMX_COLOR_FORMATTYPE ColorFormat;
  OMX_U32 nWidth;
  OMX_U32 nHeight;
  OMX_U32 nDecStride;
  OMX_U32 nFrameRate;
  OMX_U32 nEncodedFrms;
  OMX_U32 displayId;
  OMX_VIDEO_CODINGTYPE  codingType;
  void *fieldBuf;
  IL_CLIENT_COMP_PRIVATE *capILComp;
  IL_CLIENT_COMP_PRIVATE *scILComp;
  IL_CLIENT_COMP_PRIVATE *disILComp;
  IL_CLIENT_COMP_PRIVATE *nfILComp;
  IL_CLIENT_GFX_PRIVATE gfx;
} IL_Client;

OMX_STRING VIL_ClientErrorToStr (OMX_ERRORTYPE error);

OMX_ERRORTYPE
  VIL_ClientUseInitialOutputResources (IL_CLIENT_COMP_PRIVATE *thisComp);

OMX_ERRORTYPE VIL_ClientUtilGetSelfBufHeader (IL_CLIENT_COMP_PRIVATE *thisComp,
											 OMX_U8 *pBuffer,
											 ILCLIENT_PORT_TYPE type,
											 OMX_U32 portIndex,
											 OMX_BUFFERHEADERTYPE **pBufferOut);

OMX_ERRORTYPE VIL_ClientProcessPipeCmdETB (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE VIL_ClientProcessPipeCmdFTB (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE VIL_ClientProcessPipeCmdEBD (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE VIL_ClientProcessPipeCmdFBD (IL_CLIENT_COMP_PRIVATE *thisComp,
										  IL_CLIENT_PIPE_MSG *pipeMsg);
OMX_ERRORTYPE VIL_ClientCbFillBufferDone (OMX_HANDLETYPE hComponent,
										 OMX_PTR ptrAppData,
										 OMX_BUFFERHEADERTYPE *pBuffer);
OMX_ERRORTYPE VIL_ClientCbEmptyBufferDone (OMX_HANDLETYPE hComponent,
										  OMX_PTR ptrAppData,
										  OMX_BUFFERHEADERTYPE *pBuffer);
OMX_ERRORTYPE VIL_ClientCbEventHandler (OMX_HANDLETYPE hComponent,
											  OMX_PTR ptrAppData,
											  OMX_EVENTTYPE eEvent,
											  OMX_U32 nData1, OMX_U32 nData2,
											  OMX_PTR pEventData);
void VIL_ClientSIGINTHandler(int sig);

#ifdef __cplusplus              /* matches __cplusplus construct above */
}
#endif



void* frameThread(void *arg);

class Video {
public:
	Int32 init(void);
	Video();
	~Video();
	bool isRunning(void) {return running;}
	bool setRunning(bool run);
	void waitGPIO();
	CameraErrortype setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY);
	CameraErrortype setImagerResolution(UInt32 x, UInt32 y);
	void (*frameCallback)(void *);
	void * frameCallbackArg;
private:
	bool running;

	UInt32 imgXSize;	//Input resolution coming from imager
	UInt32 imgYSize;
	UInt32 displayXRes;	//Display resolution
	UInt32 displayYRes;

	//Window cropped from input and scaled
	UInt32 imgStartX;	//Top left pixel of cropped window taken from input
	UInt32 imgStartY;
	UInt32 imgCropX;	//Size of cropped window
	UInt32 imgCropY;

	//Scaler output
	UInt32 displayWindowStartX;		//Top left pixel location to place image within display resolution
	UInt32 displayWindowStartY;
	UInt32 displayWindowXSize;		//Resolution image is scaled to
	UInt32 displayWindowYSize;

	//maximum sizes for image and display resolution. Used for sizing buffers
	UInt32 maxImgXSize;	//Input resolution coming from imager
	UInt32 maxImgYSize;
	UInt32 maxDisplayXRes;	//Display resolution
	UInt32 maxDisplayYRes;

	Int32 gpioFD;
	struct pollfd gpioPoll;
	pthread_t frameThreadID;
	friend void* frameThread(void *arg);
	volatile bool terminateGPIOThread;
	void frameCB(void);

	UInt32 startVideo ();
	UInt32 stopVideo ();

	void VIL_ClientInit (IL_Client **pAppData);
	void VIL_ClientDeInit (IL_Client *pAppData);

	OMX_ERRORTYPE VIL_ClientConnectComponents (IL_CLIENT_COMP_PRIVATE
												*handleCompPrivA,
											  unsigned int compAPortOut,
											  IL_CLIENT_COMP_PRIVATE
												*handleCompPrivB,
											  unsigned int compBPortIn);

	OMX_ERRORTYPE VIL_ClientSetCaptureParams (IL_Client *pAppData);
	OMX_ERRORTYPE VIL_ClientSetScalarParams (IL_Client *pAppData);
	OMX_ERRORTYPE VIL_ClientSetNfParams (IL_Client *pAppData);
	OMX_ERRORTYPE VIL_ClientSetDisplayParams (IL_Client *pAppData);

};

#endif // VIDEO_H
