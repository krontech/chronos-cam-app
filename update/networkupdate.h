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
#ifndef NETWORKUPDATE_H
#define NETWORKUPDATE_H

#include "updateprogress.h"

enum NetworkUpdateStates {
        NETWORK_UPDATE_KEYS,
	NETWORK_COUNT_REPOS,
	NETWORK_UPDATE_LISTS,
	NETWORK_PREPARE_METAPACKAGE,
	NETWORK_INSTALL_METAPACKAGE,
	NETWORK_PREPARE_UPGRADE,
	NETWORK_INSTALL_UPGRADE,
	NETWORK_REBOOT,
};

class NetworkUpdate : public UpdateProgress
{
	Q_OBJECT

public:
	NetworkUpdate(QWidget *parent = NULL);
	~NetworkUpdate();

private:
	QString dist;
	unsigned int userReply;
	unsigned int repoCount;
	unsigned int packageCount;
	unsigned int downloadCount;
	unsigned int downloadFail;
	enum NetworkUpdateStates state;

	virtual void handleStdout(QString msg);

private slots:
	virtual void started();
	virtual void finished(int code, QProcess::ExitStatus status);
};

#endif // NETWORKUPDATE_H
