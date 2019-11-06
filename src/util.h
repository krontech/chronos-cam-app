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
#ifndef UTIL_H
#define UTIL_H

#include <QString>


void delayms(int ms);

/* delayms_events:
 *   ms - miliseconds to delay for
 *
 * Note this runs the event loop in 10ms blocks (max) so assume
 * the timing will be within 10ms only.
 */
void delayms_events(int ms);

bool checkAndCreateDir(const char * dir);

int path_is_mounted(const char *path);

QString runCommand(QString command, int *status = NULL);
QString buildSambaString();
QString buildNfsString();
bool isReachable(QString address);

#endif // UTIL_H
