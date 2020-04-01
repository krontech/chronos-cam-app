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
#include <QComboBox>


void delayms(int ms);

/* delayms_events:
 *   ms - miliseconds to delay for
 *
 * Note this runs the event loop in 10ms blocks (max) so assume
 * the timing will be within 10ms only.
 */
void delayms_events(int ms);

bool checkAndCreateDir(const char * dir);

bool pathIsMounted(const char *path);

QString runCommand(QString command, int *status = NULL);
void runBackground(const char *command);
QString parseSambaServer(QString share);
QString buildSambaString();
QString buildNfsString();
bool isReachable(QString address);
bool isExportingNfs(QString camIpAddress);

/* Scan the mounted devices for media devices which we can save
 * to and update the QComboBox with our findings. We can pass in
 * a selected drive to be the default. This will return true if
 * the selected drive changes, and false otherwise.
 */
bool refreshDriveList(QComboBox *combo, QString selection = QString());

#endif // UTIL_H
