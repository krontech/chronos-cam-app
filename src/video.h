#ifndef VIDEO_H
#define VIDEO_H


#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosVideoInterface.h"
#include "types.h"

#include <QObject>
#include <QWidget>
#include <QLabel>

/******************************************************************************/

typedef enum {
	SAVE_MODE_H264 = 0,
	SAVE_MODE_RAW16,
	SAVE_MODE_RAW12,
	SAVE_MODE_DNG,
	SAVE_MODE_TIFF,
	SAVE_MODE_TIFF_RAW,
} save_mode_type;

typedef enum {
	VIDEO_STATE_LIVEDISPLAY = 0,
	VIDEO_STATE_PLAYBACK = 1,
	VIDEO_STATE_FILESAVE = 2,
} VideoState;

typedef enum {
	FOCUS_PEAK_DISABLE = 0,
	FOCUS_PEAK_BLUE = 1,
	FOCUS_PEAK_GREEN = 2,
	FOCUS_PEAK_CYAN = 3,
	FOCUS_PEAK_RED = 4,
	FOCUS_PEAK_MAGENTA = 5,
	FOCUS_PEAK_YELLOW = 6,
	FOCUS_PEAK_WHITE = 7,
} FocusPeakColors;

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
	Int32  position;
	UInt32 totalSegments;
	UInt32 segment;
	double framerate;
};

class Video : public QWidget {
	Q_OBJECT

public:
	explicit Video();
	~Video();

	UInt32 getPosition(void);
	void seekFrame(int delta);
	void setPosition(unsigned int position);
	void setPlayback(int rate);
	void loopPlayback(unsigned int start, unsigned int length, int rate);
	void setDisplayOptions(bool zebra, FocusPeakColors fpColor);
	void setZebra(bool zebra);
	void setZoom(double zoom);
	void setDisplayPosition(bool videoOnRight);
	void liveDisplay(bool flip);
	void liveRecord(void);
	void pauseDisplay(void);
	void reset(void);
	VideoState getStatus(VideoStatus *st);

	CameraErrortype startRecording(UInt32 sizeX, UInt32 sizeY, UInt32 start, UInt32 length, save_mode_type save_mode);
	CameraErrortype stopRecording(void);

	void flushRegions(void);
	CameraErrortype setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY);

	/* Settings moved over from the VideoRecord class */

	double bitsPerPixel;
	double maxBitrate;
	UInt32 framerate;
	UInt32 profile;
	UInt32 level;
    char liveRecFileDirectory[1000];

signals:
	void started(VideoState state);
	void ended(VideoState state, QString error);
	void newSegment(VideoStatus *status);

private:
	int pid;
	bool running;
	pthread_mutex_t mutex;

	void checkpid(void);
	int mkfilename(char *path, save_mode_type save_mode);

    CaKrontechChronosVideoInterface iface;

	UInt32 displayWindowXSize;
	UInt32 displayWindowYSize;
	UInt32 displayWindowXOff;
	UInt32 displayWindowYOff;
	bool displayVideoZoom;

	/* Doubleclick events don't really work for touch, so let's DIY */
	void mousePressEvent(QMouseEvent *);
	QTime clickTimer;
	int clickX;
	int clickY;

	/* Status Text */
	void setStatusText(const QString &value);
	QLabel *text;

	/* D-Bus signal handlers. */
private slots:
	void sof(const QVariantMap &args);
	void eof(const QVariantMap &args);
	void segment(const QVariantMap &args);

    void noNewSegment();
};

#endif // VIDEO_H
