/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#ifndef VIDEORECORD_H
#define VIDEORECORD_H

#include <pthread.h>
#include <poll.h>
#include <glib.h>
#include <gst/gst.h>
#include "errorCodes.h"
#include "types.h"

typedef enum {
	SAVE_MODE_H264 = 0,
	SAVE_MODE_RAW16,
	SAVE_MODE_RAW16RJ,
	SAVE_MODE_RAW12,
	SAVE_MODE_RAW16_PNG
} save_mode_type;


/*
  bitrate             : Encoding bit-rate
						flags: readable, writable
						Unsigned Integer. Range: 0 - 4294967295 Default: 500000 Current: 500000
  profile             : H.264 Profile
						flags: readable, writable
						Enum "GstOmxVideoAVCProfile" Default: 1, "base" Current: 1, "base"
						   (1): base             - Base Profile
						   (2): main             - Main Profile
						   (4): extended         - Extended Profile
						   (8): high             - High Profile
						   (16): high-10          - High 10 Profile
						   (32): high-422         - High 4:2:2 Profile
						   (64): high-444         - High 4:4:4 Profile
  level               : H.264 Level
						flags: readable, writable
						Enum "GstOmxVideoAVCLevel" Default: 8192, "level-42" Current: 8192, "level-42"
						   (1): level-1          - Level 1
						   (2): level-1b         - Level 1b
						   (4): level-11         - Level 11
						   (8): level-12         - Level 12
						   (16): level-13         - Level 13
						   (32): level-2          - Level 2
						   (64): level-21         - Level 21
						   (128): level-22         - Level 22
						   (256): level-3          - Level 3
						   (512): level-31         - Level 31
						   (1024): level-32         - Level 32
						   (2048): level-4          - Level 4
						   (4096): level-41         - Level 41
						   (8192): level-42         - Level 42
						   (16384): level-5          - Level 5
						   (32768): level-51         - Level 51
  i-period            : Specifies periodicity of I frames (0:Disable)
						flags: readable, writable
						Unsigned Integer. Range: 0 - 2147483647 Default: 90 Current: 90
  force-idr-period    : Specifies periodicity of IDR frames (0:Disable)
						flags: readable, writable
						Unsigned Integer. Range: 0 - 2147483647 Default: 0 Current: 0
  force-idr           : force next frame to be IDR
						flags: writable
						Boolean. Default: false  Write only
  encodingPreset      : Specifies which encoding preset to use
						flags: readable, writable
						Enum "GstOmxVideoEncoderPreset" Default: 3, "hsmq" Current: 3, "hsmq"
						   (1): hq               - High Quality
						   (2): user             - User Defined
						   (3): hsmq             - High Speed Med Qual
						   (4): msmq             - Med Speed Med Qaul
						   (5): mshq             - Med Speed High Qaul
						   (6): hs               - High Speed
  rateControlPreset   : Specifies what rate control preset to use
						flags: readable, writable
						Enum "GstOmxVideoRateControlPreset" Default: 0, "low-delay" Current: 0, "low-delay"
						   (0): low-delay        - Low Delay
						   (1): storage          - Storage
						   (2): two-pass         - Two Pass
						   (3): none             - none
root@dm814x-evm:~#

 **/

#define ENCODER_MIN_H_RES		((UInt32) 96)	//Minimum resoluiton the TI encoder can run at
#define ENCODER_MIN_V_RES		((UInt32) 80)
#define ENCODER_H_RES_INC		((UInt32) 16)	//Horizontal resolution must be a multiple of 16
/*
typedef enum VideoRecordErrortype
{
	RECORD_ERROR_NONE = 0,
	RECORD_THREAD_ERROR,
	RECORD_NOT_RUNNING,
	RECORD_ALREADY_RUNNING,
	RECORD_NO_DIRECTORY_SET,
	RECORD_FILE_EXISTS
} VideoRecordErrortype;
*/
enum
{
	OMX_H264ENC_PROFILE_BASE =		1,
	OMX_H264ENC_PROFILE_MAIN =		2,
	OMX_H264ENC_PROFILE_EXTENDED =	4,
	OMX_H264ENC_PROFILE_HIGH =		8,
	OMX_H264ENC_PROFILE_HIGH_10 =	16,
	OMX_H264ENC_PROFILE_HIGH_422 =	32,
	OMX_H264ENC_PROFILE_HIGH_444 =	64
};

enum
{
	OMX_H264ENC_LVL_1 =		1,
	OMX_H264ENC_LVL_1B =	2,
	OMX_H264ENC_LVL_11 =	4,
	OMX_H264ENC_LVL_12 =	8,
	OMX_H264ENC_LVL_13 =	16,
	OMX_H264ENC_LVL_2 =		32,
	OMX_H264ENC_LVL_21 =	64,
	OMX_H264ENC_LVL_22 =	128,
	OMX_H264ENC_LVL_3 =		256,
	OMX_H264ENC_LVL_31 =	512,
	OMX_H264ENC_LVL_32 =	1024,
	OMX_H264ENC_LVL_4 =		2048,
	OMX_H264ENC_LVL_41 =	4096,
	OMX_H264ENC_LVL_42 =	8192,
	OMX_H264ENC_LVL_5 =		16384,
	OMX_H264ENC_LVL_51 =	32768
};

enum
{
	OMX_H264ENC_ENC_PRE_HQ =	1,
	OMX_H264ENC_ENC_PRE_USER =	2,
	OMX_H264ENC_ENC_PRE_HSMQ =	3,
	OMX_H264ENC_ENC_PRE_MSMQ =	4,
	OMX_H264ENC_ENC_PRE_MSHQ =	5,
	OMX_H264ENC_ENC_PRE_HS =	6
};

void* frameThread(void *arg);

class VideoRecord {
public:
	Int32 init(void);
	VideoRecord();
	~VideoRecord();
	int start(unsigned int hSize, unsigned int vSize, unsigned int frames, save_mode_type save_mode = SAVE_MODE_H264);
	unsigned int stop();
	unsigned int stop2();
    bool flowReady();
    void debug();
	UInt32 getSafeHRes(UInt32 hRes);
	UInt32 getSafeVRes(UInt32 vRes);
	double getFramerate();
	bool endOfStream();
	bool getRunning(void) {return running;}

	//H264 encoder parameters
	UInt32 bitrate;
	UInt32 profile;
	UInt32 level;
	UInt32 i_period;
	UInt32 force_idr_period;
	UInt32 encodingPreset;
	void (*eosCallback)(void *);
	void * eosCallbackArg;
	void (*errorCallback)(void *, char *);
	void * errorCallbackArg;


	volatile bool recordRunning;
	double bitsPerPixel;
	double maxBitrate;
	UInt32 framerate;
	char filename[1000];
	char fileDirectory[1000];

private:
	bool running;
    int frameCount;
    int fd;
	struct timespec lastFrame;
	unsigned long long frameInterval;

	UInt32 imgXSize;	//Input resolution coming from imager
	UInt32 imgYSize;
	UInt32 numFrames;

	GstElement *pipeline;
    GstElement *source;
	guint bus_watch_id;
	bool eosFlag;
	bool error;
	pthread_t recordThreadID;
	friend gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
	friend gboolean buffer_probe(GstPad *pad, GstBuffer *buffer, gpointer data);
};


#endif // VIDEORECORD_H

