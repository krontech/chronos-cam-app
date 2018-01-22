
#include <QObject>
#include <QTimer>

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <poll.h>
#include "video.h"
#include "camera.h"

void catch_sigchild(int sig) { /* nop */ }

Int32 Video::init(void)
{
	int err;

	printf("Opening frame GPIO\n");
	gpioFD = open("/sys/class/gpio/gpio51/value", O_RDONLY|O_NONBLOCK);

	if (-1 == gpioFD)
		return VIDEO_FILE_ERROR;

	gpioPoll.fd = gpioFD;
	gpioPoll.events = POLLPRI | POLLERR;

	printf("Starting frame thread\n");
	terminateGPIOThread = false;

	err = pthread_create(&frameThreadID, NULL, &frameThread, this);
	if(err)
		return VIDEO_THREAD_ERROR;

	signal(SIGCHLD, catch_sigchild);

	printf("Video init done\n");
	return SUCCESS;
}

bool Video::setRunning(bool run)
{
	if(!running && run) {
		int child = fork();
		if (child < 0) {
			/* Could not start the video pipeline. */
			return false;
		}
		else if (child == 0) {
			/* child process - start the pipeline */
			const char *path = "/opt/camera/cam-pipeline";
			char display[64];
			char offset[64];
			snprintf(display, sizeof(display), "%ux%u", displayWindowXSize, displayWindowYSize);
			snprintf(offset, sizeof(offset), "%ux%u", displayWindowXOff, displayWindowYOff);
			execl(path, path, display, "--offset", offset, NULL);
			exit(EXIT_FAILURE);
		}
		pid = child;
		running = true;
	}
	else if(running && !run) {
		int status;
		kill(pid, SIGINT);
		waitpid(pid, &status, 0);
		running = false;
	}
	return true;
}

CameraErrortype Video::setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY)
{
	/* TODO: Replace with a SIGUSR to change focus aid scaling. */
	return SUCCESS;
}

void Video::frameCB(void)
{


}

Video::Video()
{
	pid = -1;
	running = false;
	frameCallback = NULL;

	/* Use the full width with QWS because widgets can autohide. */
#ifdef Q_WS_QWS
	displayWindowXSize = 800;
#else
	displayWindowXSize = 600;
#endif
	displayWindowYSize = 480;
	displayWindowXOff = 0;
	displayWindowYOff = 0;
}

Video::~Video()
{
	terminateGPIOThread = true;
	pthread_join(frameThreadID, NULL);
	setRunning(false);

	if(-1 != gpioFD)
		close(gpioFD);
}


void* frameThread(void *arg)
{
	Video * vInst = (Video *)arg;
	char buf[2];
	int ret;

	pthread_t this_thread = pthread_self();

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);

	printf("Trying to set frameThread realtime prio = %d", params.sched_priority);

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
	read(vInst->gpioPoll.fd, buf, sizeof(buf));

	while(!vInst->terminateGPIOThread)
	{
		if((ret = poll(&vInst->gpioPoll, 1, 200)) > 0)	//If we returned due to a GPIO event rather than a timeout
		{
			if (vInst->gpioPoll.revents & POLLPRI)
			{
				/* IRQ happened */
				lseek(vInst->gpioPoll.fd, 0, SEEK_SET);
				read(vInst->gpioPoll.fd, buf, sizeof(buf));
			}
			vInst->frameCB();	//The GPIO is high starting at the line before the first active video line in the frame.
								//This line goes high just after the frame address is registered by the video display hardware, so you have almost the full frame period to update it.
			if(vInst->frameCallback)
				(*vInst->frameCallback)(vInst->frameCallbackArg);	//Call user callback

		}
	}

	pthread_exit(NULL);
}
