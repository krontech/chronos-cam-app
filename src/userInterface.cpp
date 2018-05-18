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

	//set up initial values
	read(encAgpioFD, buf, sizeof(buf));
	encAVal = encALast = ('1' == buf[0]) ? true : false;

	read(encBgpioFD, buf, sizeof(buf));
	encBVal = encBLast = ('1' == buf[0]) ? true : false;

	printf("Starting encoder threads\n");
	terminateEncThreads = false;

	err = pthread_create(&encThreadID, NULL, &encoderThread, this);
	if(err)
		return UI_THREAD_ERROR;

	printf("UI init done\n");
	return SUCCESS;
}

UserInterface::UserInterface()
{
	encValue = 0;
	encValueLowRes = 0;
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
		if (delta == 0) return;
		Qt::Key key = (delta > 0) ? Qt::Key_Up : Qt::Key_Down;
		QWidget *w = QApplication::focusWidget();
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyPress, key, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(w, ev);
	}
}

void *encoderThread(void *arg)
{
	pthread_t this_thread = pthread_self();
	UserInterface * uiInst = (UserInterface *)arg;
	struct pollfd fds[] = {
		{ .fd = uiInst->encAgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = uiInst->encBgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = uiInst->encSwgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
	};
	char buf[2];
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

	//Dummy read so poll blocks properly
	read(uiInst->encAgpioFD, buf, sizeof(buf));
	read(uiInst->encBgpioFD, buf, sizeof(buf));
	read(uiInst->encSwgpioFD, buf, sizeof(buf));

	while(!uiInst->terminateEncThreads)
	{
		if((ret = poll(fds, sizeof(fds)/sizeof(struct pollfd), -1)) > 0)	//If we returned due to a GPIO event rather than a timeout
		{
			if (fds[0].revents & POLLPRI)
			{
				/* IRQ happened for encoder pin A */
				uiInst->encAVal = uiInst->getGpioValue(uiInst->encAgpioFD);
			}
			if (fds[1].revents & POLLPRI)
			{
				/* IRQ happened for encoder pin B */
				uiInst->encBVal = uiInst->getGpioValue(uiInst->encBgpioFD);
			}
			if (fds[2].revents & POLLPRI)
			{
				/* IRQ happened for encoder switch */
				QEvent::Type type = uiInst->getEncoderSwitch() ? QKeyEvent::KeyPress : QKeyEvent::KeyRelease;
				QWidget *w = QApplication::focusWidget();
				QKeyEvent *ev = new QKeyEvent(type, Qt::Key_Return, Qt::NoModifier, "encoder", false, 0);
				QApplication::postEvent(w, ev);
			}
			uiInst->encoderCB();
		}
	}

	pthread_exit(NULL);
}
