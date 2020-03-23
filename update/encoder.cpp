/****************************************************************************
 *  Copyright (C) 2013-2020 Kron Technologies Inc <http://www.krontech.ca>. *
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
#include <sys/poll.h>
#include <QDebug>
#include <QApplication>
#include <QWidget>
#include <QKeyEvent>

#include "utils.h"

#define ENC_DEBOUNCE_MSEC	10

int getGpioValue(int fd)
{
	char buf[2];
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, sizeof(buf));
	return ('1' == buf[0]) ? 1 : 0;
}

//encoder in detent with EncA = 1, EncB = 1. Clockwise rotation results in first change being EncA <- 0;
static void
encoderCB(int a, int b, int sw)
{
	static int first = 0;
	static int lastA = 0;
	static int lastB = 0;

	int delta = 0;
	int lowres = 0;

	if (first == 0) {
		lastA = a;
		lastB = b;
		first = 1;
		return;
	}

	/* Compute the total encoder position change */
	delta = (b) ? (lastA - a) : (a - lastA);
	if (a) {
		delta += (b - lastB);
	} else {
		delta -= (b - lastB);
		lowres -= (b - lastB);
	}
	lastA = a;
	lastB = b;

	if (sw) {
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

// handle changes in the encoder switch position.
static void
switchCB(int swval)
{
	if (swval) {
		/* Encoder switch rising edge */
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyPress, Qt::Key_Select, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(QApplication::focusWidget(), ev);
	}
	else {
		/* Encoder switch falling edge */
		QKeyEvent *ev = new QKeyEvent(QKeyEvent::KeyRelease, Qt::Key_Select, Qt::NoModifier, "", false, 0);
		QApplication::postEvent(QApplication::focusWidget(), ev);
	}
}

static int terminateEncThreads = 0;

void *encoderThread(void *arg)
{
	struct timespec debounce = {0, 0};
	int encAgpioFD = open("/sys/class/gpio/gpio20/value", O_RDONLY|O_NONBLOCK);
	int encBgpioFD = open("/sys/class/gpio/gpio26/value", O_RDONLY|O_NONBLOCK);
	int encSWgpioFD = open("/sys/class/gpio/gpio27/value", O_RDONLY);
	int encSWval;
	struct pollfd fds[] = {
		{ .fd = encAgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = encBgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
		{ .fd = encSWgpioFD, .events = POLLPRI | POLLERR, .revents = 0 },
	};
	int ret;

	/* Make an attempt to increase scheduling priority */
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &params);
	if (ret != 0) {
		// Print the error
		printf("Unsuccessful in setting thread realtime prio");
		return (void *)0;
	}

	// Print thread scheduling priority
	printf("Thread priority is %d", params.sched_priority);

	// Setup the initial values
	encoderCB(getGpioValue(encAgpioFD), getGpioValue(encBgpioFD), getGpioValue(encSWgpioFD));

	while(!terminateEncThreads) {
		ret = poll(fds, sizeof(fds)/sizeof(struct pollfd), ENC_DEBOUNCE_MSEC);
		if(ret > 0) {
			encoderCB(getGpioValue(encAgpioFD), getGpioValue(encBgpioFD), getGpioValue(encSWgpioFD));
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
		else {
			int val = getGpioValue(encSWgpioFD);

			/* Handle the change in encoder switch */
			if (val == encSWval) continue;
			switchCB(val);
			encSWval = val;

			/* Set a new debounce timeout. */
			clock_gettime(CLOCK_MONOTONIC, &debounce);
			debounce.tv_nsec += ENC_DEBOUNCE_MSEC * 1000000;
			if (debounce.tv_nsec > 1000000000) {
				debounce.tv_nsec -= 1000000000;
				debounce.tv_sec++;
			}
		}
	}

	pthread_exit(NULL);
}

void
startEncoder(void)
{
	pthread_t encoderThreadID;
	pthread_create(&encoderThreadID, NULL, &encoderThread, NULL);
}

void
stopEncoder(void)
{
	terminateEncThreads = 1;
}
