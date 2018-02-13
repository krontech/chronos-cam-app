#ifndef VIDEO_H
#define VIDEO_H

#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosVideoInterface.h"
#include "types.h"

/******************************************************************************/

void* frameThread(void *arg);

class Video {
public:
	Video();
	~Video();
	Int32 init();

	UInt32 getPosition(void);
	void setPosition(unsigned int position, int rate);
	void setPlayback(int rate);

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
};

#endif // VIDEO_H
