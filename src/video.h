#ifndef VIDEO_H
#define VIDEO_H

#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "types.h"

/******************************************************************************/

void* frameThread(void *arg);

class Video {
public:
	Video();
	~Video();
	Int32 init();
	bool isRunning(void) {return running;}
	bool setRunning(bool run);
	CameraErrortype setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY);
	void (*frameCallback)(void *);
	void * frameCallbackArg;
private:
	int pid;
	bool running;

	UInt32 displayWindowXSize;
	UInt32 displayWindowYSize;
	UInt32 displayWindowXOff;
	UInt32 displayWindowYOff;

	Int32 gpioFD;
	struct pollfd gpioPoll;
	pthread_t frameThreadID;
	friend void* frameThread(void *arg);
	volatile bool terminateGPIOThread;
	void frameCB(void);
};

#endif // VIDEO_H
