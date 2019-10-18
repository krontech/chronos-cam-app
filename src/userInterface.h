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
#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "types.h"

void *encoderThread(void *arg);

class UserInterface {
public:
	Int32 init(void);
	~UserInterface();

	bool getShutterButton();
private:
	bool encAVal, encALast;
	bool encBVal, encBLast;
	bool encSwVal, encSwLast;
	Int32 encAgpioFD, encBgpioFD, encSwgpioFD, shSwgpioFD;
	pthread_t encThreadID;
	bool getGpioValue(int fd);
	bool getEncoderSwitch();
	friend void* encoderThread(void *arg);
	volatile bool terminateEncThreads;
	void encoderCB(void);
	bool switchCB(void);
};

#endif // USERINTERFACE_H
