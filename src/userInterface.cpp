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
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include <QApplication>
#include <QWidget>
#include <QKeyEvent>
#include "userInterface.h"
#include "types.h"

#define ENC_SW_DEBOUNCE_MSEC	10

Int32 UserInterface::init(void)
{
	int err;

	printf("Opening frame GPIO\n");
	encAgpioFD = open("/sys/class/gpio/gpio20/value", O_RDONLY|O_NONBLOCK);
	encBgpioFD = open("/sys/class/gpio/gpio26/value", O_RDONLY|O_NONBLOCK);
	encSwgpioFD = open("/sys/class/gpio/gpio27/value", O_RDONLY);
	shSwgpioFD = open("/sys/class/gpio/gpio66/value", O_RDONLY);

	if (-1 == encAgpioFD || -1 == encBgpioFD || -1 == encSwgpioFD || -1 == shSwgpioFD)
		return UI_FILE_ERROR;

	printf("Starting encoder threads\n");
	terminateEncThreads = false;

	err = pthread_create(&encThreadID, NULL, &encoderThread, this);
	if(err)
		return UI_THREAD_ERROR;

	printf("UI init done\n");
	return SUCCESS;
}

UserInterface::~UserInterface()
{
	terminateEncThreads = true;

	pthread_join(encThreadID, NULL);

	if(-1 != encAgpioFD)
		close(encAgpioFD);

	if(-1 != encBgpioFD)
		close(encBgpioFD);

	if(-1 != encSwgpioFD)
		close(encSwgpioFD);

	if(-1 != shSwgpioFD)
		close(shSwgpioFD);
}

bool UserInterface::getGpioValue(int fd)
{
	char buf[2];
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, sizeof(buf));
	return ('1' == buf[0]) ? true : false;
}

bool UserInterface::getShutterButton()
{
	char buf[2];

	lseek(shSwgpioFD, 0, SEEK_SET);
	read(shSwgpioFD, buf, sizeof(buf));

	return ('1' == buf[0]) ? false : true;
}

bool UserInterface::getEncoderSwitch()
{
	return getGpioValue(encSwgpioFD);
}

//encoder in detent with EncA = 1, EncB = 1. Clockwise rotation results in first change being EncA <- 0;
void UserInterface::encoderCB(void)
{
	int delta = 0;
	int lowres = 0;

	if(encAVal && !encALast)	//rising edge
	{
		if(encBVal)
			delta--;
		else
			delta++;
	}
	else if(!encAVal && encALast)	//falling edge
	{
		if(encBVal)
			delta++;
		else
			delta--;
	}
	if(encBVal && !encBLast)	//rising edge
	{
		if(encAVal) {
			delta++;
		} else {
			delta--;
			lowres--;
		}
	}
	else if(!encBVal && encBLast)	//falling edge
	{
		if(encAVal) {
			delta--;
		} else {
			delta++;
			lowres++;
		}
	}
	encALast = encAVal;
	encBLast = encBVal;

	if (getEncoderSwitch()) {
		if (delta == 0) return;
		Qt::Key key = (delta > 0) ? Qt::Key_PageUp : Qt::Key_PageDown;
		QWidget *w = QApplication::focusWidget();
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyPress, key, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(w, ev);
	}
	else {
		if (lowres == 0) return;
		Qt::Key key = (lowres > 0) ? Qt::Key_Up : Qt::Key_Down;
		QWidget *w = QApplication::focusWidget();
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyPress, key, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(w, ev);
	}
}

// check for changes in the encoder switch position.
bool UserInterface::switchCB(void)
{
	bool change = false;

	encSwVal = getGpioValue(encSwgpioFD);
	if (encSwVal && !encSwLast) {
		/* Encoder switch rising edge */
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyPress, Qt::Key_Select, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(QApplication::focusWidget(), ev);
		change = true;
	}
	if (!encSwVal && encSwLast) {
		/* Encoder switch falling edge */
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyRelease, Qt::Key_Select, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(QApplication::focusWidget(), ev);
		change = true;
	}
	encSwLast = encSwVal;
	return change;
}

void *encoderThread(void *arg)
{
	pthread_t this_thread = pthread_self();
	UserInterface * uiInst = (UserInterface *)arg;
	struct timespec debounce = {0, 0};
	struct pollfd fds[] = {
		{ .fd = uiInst->encAgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = uiInst->encBgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = uiInst->encSwgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
	};
	int ret;

	// struct sched_param is used to store the scheduling priority
	struct sched_param params;
	// We'll set the priority to the maximum.
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;

	printf("Trying to set encoderThread realtime prio = %d", params.sched_priority);

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

	// Setup the initial values
	uiInst->encAVal = uiInst->encALast = uiInst->getGpioValue(uiInst->encAgpioFD);
	uiInst->encBVal = uiInst->encBLast = uiInst->getGpioValue(uiInst->encBgpioFD);
	uiInst->encSwVal = uiInst->encSwLast = uiInst->getGpioValue(uiInst->encSwgpioFD);

	while(!uiInst->terminateEncThreads) {
		ret = poll(fds, sizeof(fds)/sizeof(struct pollfd), ENC_SW_DEBOUNCE_MSEC);
		if(ret > 0) {
			/* Handle IRQs for the encoder rotation pins. */
			if (fds[0].revents & POLLPRI) {
				uiInst->encAVal = uiInst->getGpioValue(uiInst->encAgpioFD);
			}
			if (fds[1].revents & POLLPRI) {
				uiInst->encBVal = uiInst->getGpioValue(uiInst->encBgpioFD);
			}
			uiInst->encoderCB();
		}

		/* Debounce and check for changes in the encoder switch. */
		if (debounce.tv_sec || debounce.tv_nsec) {
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec < debounce.tv_sec) continue;
			if ((now.tv_sec == debounce.tv_sec) && (now.tv_nsec < debounce.tv_nsec)) continue;

			/* Debouncing cleared */
			memset(&debounce, 0, sizeof(debounce));
		}
		if (uiInst->switchCB()) {
			/* Set a new debounce timeout. */
			clock_gettime(CLOCK_MONOTONIC, &debounce);
			debounce.tv_nsec += ENC_SW_DEBOUNCE_MSEC * 1000000;
			if (debounce.tv_nsec > 1000000000) {
				debounce.tv_nsec -= 1000000000;
				debounce.tv_sec++;
			}
		}
	}

	pthread_exit(NULL);
}
