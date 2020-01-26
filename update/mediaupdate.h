/****************************************************************************
 *  Copyright (C) 2019-2020 Kron Technologies Inc <http://www.krontech.ca>. *
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
#ifndef MEDIAUPDATE_H
#define MEDIAUPDATE_H

#include "updateprogress.h"

enum UpdateStates {
	UPDATE_CHECKSUMS,
	UPDATE_PREINST,
	UPDATE_DEBPKG,
	UPDATE_POSTINST,
	UPDATE_REBOOT,
};

class MediaUpdate : public UpdateProgress
{
	Q_OBJECT

public:
	MediaUpdate(QString manifestPath, QWidget *parent = NULL);
	~MediaUpdate();

private:
	QDir      location;
	QFileInfo manifest;
	enum UpdateStates state;

	void parseManifest();
	virtual void handleStdout(QString msg);

private slots:
	virtual void started();
	virtual void finished(int code, QProcess::ExitStatus status);
};

#endif // MEDIAUPDATE_H
