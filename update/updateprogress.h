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
#ifndef UPDATEPROGRESS_H
#define UPDATEPROGRESS_H

#include <QWidget>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <pthread.h>

namespace Ui {
class UpdateProgress;
}

class UpdateProgress : public QWidget
{
	Q_OBJECT
	
public:
	explicit UpdateProgress(QWidget *parent = 0);
	~UpdateProgress();
	
	/* What to do with the console output */
	void log(QString msg);
	virtual void handleStdout(QString msg) { log(msg); }
	virtual void handleStderr(QString msg) { log(msg); }

protected:
	Ui::UpdateProgress *ui;
	QProcess *process;
	QTimer   *timer;
	QStringList packages;
	QString   preinst;
	QString   postinst;
	int       countdown;

	void startReboot(int secs);
	void stepProgress(const QString & title = QString());

public slots:
	virtual void started() = 0;
	virtual void finished(int code, QProcess::ExitStatus status) = 0;
	void error(QProcess::ProcessError err);
	void readyReadStandardOutput();
	void readyReadStandardError();

	void timeout();
};

#endif // UPDATEPROGRESS_H
