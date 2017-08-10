#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include "userInterface.h"
#include "types.h"

Int32 UserInterface::init(void)
{
	int err;
	char buf[2];

	printf("Opening frame GPIO\n");
	encAgpioFD = open("/sys/class/gpio/gpio20/value", O_RDONLY|O_NONBLOCK);
	encBgpioFD = open("/sys/class/gpio/gpio26/value", O_RDONLY|O_NONBLOCK);
	encSwgpioFD = open("/sys/class/gpio/gpio27/value", O_RDONLY);
	shSwgpioFD = open("/sys/class/gpio/gpio66/value", O_RDONLY);
	recLedFrontFD = open("/sys/class/gpio/gpio41/value", O_WRONLY);
	recLedBackFD = open("/sys/class/gpio/gpio25/value", O_WRONLY);

	if (-1 == encAgpioFD || -1 == encBgpioFD || -1 == encSwgpioFD || -1 == shSwgpioFD || -1 == recLedFrontFD || -1 == recLedBackFD)
		return UI_FILE_ERROR;

	encAgpioPoll.fd = encAgpioFD;
	encBgpioPoll.fd = encBgpioFD;

	encAgpioPoll.events = POLLPRI | POLLERR;
	encBgpioPoll.events = POLLPRI | POLLERR;

	//set up initial values
	read(encAgpioPoll.fd, buf, sizeof(buf));
	encAVal = encALast = ('1' == buf[0]) ? true : false;

	read(encBgpioPoll.fd, buf, sizeof(buf));
	encBVal = encBLast = ('1' == buf[0]) ? true : false;

	printf("Starting encoder threads\n");
	terminateEncThreads = false;

	err = pthread_create(&encAThreadID, NULL, &encAThread, this);
	if(err)
		return UI_THREAD_ERROR;

	err = pthread_create(&encBThreadID, NULL, &encBThread, this);
	if(err)
		return UI_THREAD_ERROR;

	printf("UI init done\n");
	return CAMERA_SUCCESS;
}

UserInterface::UserInterface()
{
	encValue = 0;
	encValueLowRes = 0;
}

UserInterface::~UserInterface()
{
	terminateEncThreads = true;

	pthread_join(encAThreadID, NULL);
	pthread_join(encBThreadID, NULL);

	if(-1 != encAgpioFD)
		close(encAgpioFD);

	if(-1 != encBgpioFD)
		close(encBgpioFD);

	if(-1 != encSwgpioFD)
		close(encSwgpioFD);

	if(-1 != shSwgpioFD)
		close(shSwgpioFD);
}

Int32 UserInterface::getEncoderValue(Int32 * encValLowResPtr)
 {
	Int32 val = encValue;
	if(encValLowResPtr)
		*encValLowResPtr = encValueLowRes;

	encValue = 0;
	encValueLowRes = 0;

	return val;
 }

bool UserInterface::getShutterButton()
{
	char buf[2];

	lseek(shSwgpioFD, 0, SEEK_SET);
	read(shSwgpioFD, buf, sizeof(buf));

	return ('1' == buf[0]) ? false : true;
}

void UserInterface::setRecLEDBack(bool on)
{
	lseek(recLedBackFD, 0, SEEK_SET);
	write(recLedBackFD, on ? "1" : "0", 1);
}

void UserInterface::setRecLEDFront(bool on)
{
	lseek(recLedFrontFD, 0, SEEK_SET);
	write(recLedFrontFD, on ? "1" : "0", 1);
}

bool UserInterface::getEncoderSwitch()
{
	char buf[2];

	lseek(encSwgpioFD, 0, SEEK_SET);
	read(encSwgpioFD, buf, sizeof(buf));

	return ('1' == buf[0]) ? true : false;
}

//encoder in detent with EncA = 1, EncB = 1. Clockwise rotation results in first change being EncA <- 0;
void UserInterface::encoderCB(void)
{
	if(encAVal && !encALast)	//rising edge
	{
		if(encBVal)
			encValue--;

		else
			encValue++;
	}
	else if(!encAVal && encALast)	//falling edge
	{
		if(encBVal)
			encValue++;

		else
			encValue--;
	}
	if(encBVal && !encBLast)	//rising edge
	{
		if(encAVal)
			encValue++;
		else
		{
			encValue--;
			encValueLowRes--;
		}
	}
	else if(!encBVal && encBLast)	//falling edge
	{
		if(encAVal)
			encValue--;
		else
		{
			encValue++;
			encValueLowRes++;
		}

	}

	encALast = encAVal;
	encBLast = encBVal;
}


void* encAThread(void *arg)
{
	UserInterface * uiInst = (UserInterface *)arg;
	char buf[2];
	int ret;

	pthread_t this_thread = pthread_self();

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;

	printf("Trying to set encAThread realtime prio = %d", params.sched_priority);

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
	read(uiInst->encAgpioPoll.fd, buf, sizeof(buf));

	while(!uiInst->terminateEncThreads)
	{
		if(ret = poll(&uiInst->encAgpioPoll, 1, -1) > 0)	//If we returned due to a GPIO event rather than a timeout
		{
			if (uiInst->encAgpioPoll.revents & POLLPRI)
			{
				/* IRQ happened */
				lseek(uiInst->encAgpioPoll.fd, 0, SEEK_SET);
				read(uiInst->encAgpioPoll.fd, buf, sizeof(buf));
				uiInst->encAVal = ('1' == buf[0]) ? true : false;
			}
			uiInst->encoderCB();
		}
	}

	pthread_exit(NULL);
}

void* encBThread(void *arg)
{
	UserInterface * uiInst = (UserInterface *)arg;
	char buf[2];
	int ret;

	pthread_t this_thread = pthread_self();

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;

	printf("Trying to set encBThread realtime prio = %d", params.sched_priority);

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
	read(uiInst->encBgpioPoll.fd, buf, sizeof(buf));

	while(!uiInst->terminateEncThreads)
	{
		if(ret = poll(&uiInst->encBgpioPoll, 1, -1) > 0)	//If we returned due to a GPIO event rather than a timeout
		{
			if (uiInst->encBgpioPoll.revents & POLLPRI)
			{
				/* IRQ happened */
				lseek(uiInst->encBgpioPoll.fd, 0, SEEK_SET);
				read(uiInst->encBgpioPoll.fd, buf, sizeof(buf));
				uiInst->encBVal = ('1' == buf[0]) ? true : false;
			}
			uiInst->encoderCB();
		}
	}

	pthread_exit(NULL);
}
