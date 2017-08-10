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

#ifndef __OMX_H264_VDEC_PARSER_H__
#define __OMX_H264_VDEC_PARSER_H__

/******************************************************************************\
 *      Includes
\******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus              /* required for headers that might */
extern "C"
{                               /* be compiled by a C++ compiler */
#endif

/******************************************************************************\
 *      Defines
\******************************************************************************/
#define AVCLOG(X,Y,Z) {}        // { printf Z; printf ("\n"); }

/* In lieu of avc.h */

//typedef unsigned int uint32_t;
//typedef unsigned char uint8_t;
typedef unsigned int AVC_Err;

/* Common errorcodes */
#define AVC_SUCCESS                 (0)
#define AVC_ERR_NOT_SUPPORTED       (1)
#define AVC_ERR_MALLOC_FAILED       (2)
#define AVC_ERR_INTERNAL_ERROR      (3)
#define AVC_ERR_INVALID_ARGS        (4)
#define AVC_ERR_NO_OUTPUT_CHANNEL   (5)
#define AVC_ERR_WRONG_STATE         (6)
#define AVC_ERR_INSUFFICIENT_INPUT  (7)

#define CHECKPOINT

/* Various start codes for H264 */
/* Last byte only */
#define H264_NAL_ACCESS_UNIT_CODEDSLICE_CODE_FOR_NONIDR		0x01
#define H264_NAL_ACCESS_UNIT_CODEDSLICE_CODE_FOR_IDR 		0x05
#define H264_PPS_START_CODE                                 0x08
#define H264_SPS_START_CODE                                 0x07
#define H264_WORKING_WORD_INIT  							0xFFFFFFFF

/* Read size is data, which is read by parser in single read, this will be 
  chuked by parser untill more data is required to find a frame */

#define READSIZE 	(1*1024*1024)
#define CHUNKSIZE (1*1024*1024)
#define CHUNK_TO_READ (64*16)

/******************************************************************************\
*      Data type definitions
\******************************************************************************/

/*!
*******************************************************************************
*  \struct AVChunk_Buf
*  \brief  Chunk Buffer
*******************************************************************************
*/
typedef struct
{
  unsigned char *ptr;
  int bufsize;
  int bufused;
} AVChunk_Buf;

/*!
*******************************************************************************
*  \enum
*  \brief  Parser State
*******************************************************************************
*/
enum
{
  H264_ST_LOOKING_FOR_SPS,     /**< Initial state at start, look for SPS */
  H264_ST_LOOKING_FOR_ZERO_SLICE,    /**< Looking for slice header with zero MB num */
  H264_ST_INSIDE_PICTURE,         /**< Inside a picture, looking for next picure start */
  H264_ST_STREAM_ERR,             /**< When some discontinuity was detected in the stream */
  H264_ST_HOLDING_SC              /**< Intermediate state, when a new frame is detected */
};

/*!
*******************************************************************************
*  \struct H.264_ChunkingCtx
*  \brief  H.264 Chunking Context
*******************************************************************************
*/
/* H264 video chunking */
typedef struct
{
  unsigned int workingWord;
  unsigned char fifthByte;
  unsigned char state;
} H264_ChunkingCtx;

typedef struct
{
  FILE *fp;
  unsigned int frameNo;
  unsigned int frameSize;
  unsigned char *readBuf;
  unsigned char *chunkBuf;
  H264_ChunkingCtx ctx;
  AVChunk_Buf inBuf;
  AVChunk_Buf outBuf;
  unsigned int bytes;
  unsigned int tmp;
  unsigned char firstParse;
  unsigned int chunkCnt;
} H264_ParsingCtx;

typedef struct
{
  FILE *fp;
  unsigned int idx;
  unsigned int flagSEQ;
  unsigned char *working_frame;
  unsigned char *savedbuff;
  unsigned char *buff_in;
  unsigned int count;
} MPEG4_ParsingCtx;

typedef struct
{
  FILE *fp;
  unsigned int idx;
  unsigned int flagSEQ;
  unsigned char *working_frame;
  unsigned char *savedbuff;
  unsigned char *buff_in;
} MPEG2_ParsingCtx;

typedef MPEG4_ParsingCtx H263_ParsingCtx;
typedef MPEG4_ParsingCtx VC1_ParsingCtx;


/******************************************************************************\
*      Macros
\******************************************************************************/

/******************************************************************************\
*      Globals
\******************************************************************************/

/******************************************************************************\
*      External Object Definitions
\******************************************************************************/

/******************************************************************************\
*      Public Function Prototypes
\******************************************************************************/

/******************************************************************************\
*      Decode_ChunkingCtxInit Function Declaration
\******************************************************************************/
/**
*
*  @brief    H.264 Chunking Context initialization
*
*  @param in:
*            c: Pointer to H264_ChunkingCtx structure
*
*  @param Out:
*            None
*
*  @return   void
*
*/

void Decode_ChunkingCtxInit (H264_ChunkingCtx * c);

/******************************************************************************\
*      Decode_DoChunking Function Declaration
\******************************************************************************/
/**
*
*  @brief    Does H.264 Frame Chunking.
*
*  @param in:
*            c: Pointer to H264_ChunkingCtx structure
*            opBufs: Pointer to AVChunk_Buf Structure
*            numOutBufs: Number of output buffers
*            inBuf: Pointer to AVChunk_Buf Structure
*            attrs: Additional attributes
*
*  @param Out:
*            None
*
*  @return   AVC_Err - AVC Error Code
*
*/

AVC_Err Decode_DoChunking (H264_ChunkingCtx * c,
                             AVChunk_Buf * opBufs, unsigned int numOutBufs,
                             AVChunk_Buf * inBuf, void *attrs);

/******************************************************************************\
*      Decode_GetNextFrameSize Function Declaration
\******************************************************************************/
/**
*
*  @brief    Gets the size of the next frame.
*
*  @param in:
*            pc: Pointer to H264_ParsingCtx structure
*
*  @param Out:
*            None
*
*  @return   uint32_t - Frame Size
*
*/
unsigned int Decode_GetNextFrameSize (H264_ParsingCtx * pc);
unsigned int Decode_GetNextH263FrameSize (H263_ParsingCtx * pc);
unsigned int Decode_GetNextMpeg4FrameSize (MPEG4_ParsingCtx * pc);
unsigned int Decode_GetNextVC1FrameSize (VC1_ParsingCtx * pc);
unsigned int Decode_GetNextMpeg2FrameSize (MPEG2_ParsingCtx *pc);

void Decode_VDEC_Reset_Parser (void *parserPtr);
void Decode_ParserInit (H264_ParsingCtx * pc, void *fp);
void Decode_Mpeg2ParserInit (MPEG2_ParsingCtx *pc, void *fp);
void Decode_Mpeg4ParserInit (MPEG4_ParsingCtx *pc, void *fp);
void Decode_H263ParserInit (H263_ParsingCtx *pc, void *fp);
void Decode_VC1ParserInit(VC1_ParsingCtx *ctx, void *fin);

#ifdef __cplusplus              /* matches __cplusplus construct above */
}
#endif

#endif
/*--------------------------------- end of file -----------------------------*/
