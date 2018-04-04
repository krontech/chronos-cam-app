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

	void addRegion(UInt32 base, UInt32 size, UInt32 offset);
	bool isRunning(void) {return running;}
	bool setRunning(bool run);
	CameraErrortype setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY);

private:
	int pid;
	bool running;
	pthread_mutex_t mutex;

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
