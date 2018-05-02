#ifndef VIDEO_H
#define VIDEO_H


#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosVideoInterface.h"
#include "types.h"

#include <QObject>

/******************************************************************************/

void* frameThread(void *arg);

struct VideoRecordArgs {
	const char *filename;
	unsigned long startFrame;
	unsigned long numFrames;
	unsigned long bitrate;	/* Target bitrate for compressed formats. */
	unsigned int bpp;		/* Bits-per-pixel for raw formats. */
};

typedef enum {
	SAVE_MODE_H264 = 0,
	SAVE_MODE_RAW16,
	SAVE_MODE_RAW16RJ,
	SAVE_MODE_RAW12,
	SAVE_MODE_RAW16_PNG
} save_mode_type;

typedef enum {
	VIDEO_STATE_LIVEDISPLAY = 0,
	VIDEO_STATE_PLAYBACK = 1,
	VIDEO_STATE_RECORDING = 2,
} VideoState;

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

struct VideoStatus {
	VideoState	state;
	UInt32 totalFrames;
	UInt32 position;
	double framerate;
};

class Video : public QObject {
	Q_OBJECT

public:
	Video();
	~Video(); /* Class needs at least one virtual function for slots to work */
	Int32 init();

	UInt32 getPosition(void);
	void setPosition(unsigned int position, int rate);
	void setPlayback(int rate);
	void setDisplayOptions(bool zebra, bool peaking);
	void liveDisplay(void);
	VideoState getStatus(VideoStatus *st);
	double getFramerate();

	CameraErrortype startRecording(UInt32 sizeX, UInt32 sizeY, UInt32 start, UInt32 length, save_mode_type save_mode);
	CameraErrortype stopRecording(void);

	void addRegion(UInt32 base, UInt32 size, UInt32 offset);
	bool isRunning(void) {return running;}
	bool setRunning(bool run);
	void reload(void);
	CameraErrortype setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY);

	/* Settings moved over from the VideoRecord class */
	double bitsPerPixel;
	double maxBitrate;
	UInt32 framerate;
	UInt32 profile;
	UInt32 level;
	char filename[1000];
	char fileDirectory[1000];

	void (*errorCallback)(void *, char *);
	void * errorCallbackArg;
	void (*eosCallback)(void *);
	void * eosCallbackArg;

private:
	int pid;
	bool running;
	pthread_mutex_t mutex;

	int mkfilename(char *path, save_mode_type save_mode);

	ComKrontechChronosVideoInterface iface;

	UInt32 displayWindowXSize;
	UInt32 displayWindowYSize;
	UInt32 displayWindowXOff;
	UInt32 displayWindowYOff;

private slots:
	void sof(const QVariantMap &args)
	{
		qDebug() << "video sof received: playback = " << args["playback"].toBool();
	}

	void eof(const QVariantMap &args)
	{
		qDebug() << "video eof received: playback = " << args["playback"].toBool();
	}
};

#endif // VIDEO_H
