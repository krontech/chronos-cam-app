#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "types.h"
/*
typedef enum
{
	UI_ERROR_NONE = 0,
	UI_FILE_ERROR,
	UI_THREAD_ERROR
} UIErrortype;
*/
void* encAThread(void *arg);
void* encBThread(void *arg);

class UserInterface {
public:
	Int32 init(void);
	UserInterface();
	~UserInterface();

	Int32 getEncoderValue(Int32 * encValLowResPtr = 0);
	bool getEncoderSwitch();
	bool getShutterButton();
	void setRecLEDBack(bool on);
	void setRecLEDFront(bool on);
private:
	Int32 encValue;
	Int32 encValueLowRes;
	bool encAVal, encBVal, encALast, encBLast;
	Int32 encAgpioFD, encBgpioFD, encSwgpioFD, shSwgpioFD, recLedFrontFD, recLedBackFD;
	struct pollfd encAgpioPoll, encBgpioPoll;
	pthread_t encAThreadID, encBThreadID;
	friend void* encAThread(void *arg);
	friend void* encBThread(void *arg);
	volatile bool terminateEncThreads;
	void encoderCB(void);

};

#endif // USERINTERFACE_H
