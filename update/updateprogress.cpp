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
#include "updateprogress.h"
#include "ui_updateprogress.h"

#include <QTimer>
#include <QDebug>

#include <errno.h>
#include <string.h>

#include "utils.h"

UpdateProgress::UpdateProgress(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::UpdateProgress)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_DeleteOnClose);
	recenterWidget(this);

	process = new QProcess(this);
	connect(process, SIGNAL(started()), this, SLOT(started()));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
	connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));

	/* Prepare a timer to auto reboot */
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));

	/* Progress bar should start at 0% */
	ui->progress->setMaximum(100);
	ui->progress->setValue(0);
	ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");
}

UpdateProgress::~UpdateProgress()
{
	delete timer;
	delete process;
	delete ui;
}

void UpdateProgress::stepProgress(const QString & title)
{
	int count = ui->progress->value();
	if (count < ui->progress->maximum()) ui->progress->setValue(count+1);
	if (!title.isEmpty()) {
		/* If a title was set, update it. */
		ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");
		ui->title->setText(title);
		log(title);
	}
}

void UpdateProgress::startReboot(int secs)
{
	QString msg = QString("Rebooting in %1").arg(secs);

	ui->progress->setValue(0);
	ui->progress->setMaximum(secs);
	ui->progress->setFormat(msg);
	countdown = secs;
	timer->start(1000);

	log(msg);
}

void UpdateProgress::error(QProcess::ProcessError err)
{
	qDebug("Process error %d", (int)err);
}

void UpdateProgress::log(QString msg)
{
	qDebug("%s", msg.toUtf8().constData());
	ui->log->appendPlainText(msg);
}

void UpdateProgress::readyReadStandardOutput()
{
	QString msg;
	char buf[1024];
	qint64 len;

	process->setReadChannel(QProcess::StandardOutput);
	len = process->readLine(buf, sizeof(buf));
	if (len > 0) {
		handleStdout(QString(buf).trimmed());
	}
}

void UpdateProgress::readyReadStandardError()
{
	char buf[1024];
	qint64 len;

	process->setReadChannel(QProcess::StandardError);
	len = process->readLine(buf, sizeof(buf));
	if (len > 0) {
		handleStderr(QString(buf).trimmed());
	}
}

void UpdateProgress::timeout()
{
	int count = ui->progress->value();
	if (count < ui->progress->maximum()) ui->progress->setValue(count+1);
	ui->progress->setFormat(QString("Rebooting in %1").arg(countdown));
	if (!countdown--) {
		system("shutdown -hr now");
		QApplication::quit();
	}
}
