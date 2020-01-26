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
#include "mediaupdate.h"
#include "ui_updateprogress.h"

#include <QTextStream>
#include <QTimer>
#include <QDebug>

MediaUpdate::MediaUpdate(QString manifestPath, QWidget *parent) : UpdateProgress(parent)
{
	manifest = QFileInfo(manifestPath);
	location = manifest.dir();
	process->setWorkingDirectory(location.absolutePath());

	/* Start by validating the manifest hashes */
	QStringList checksumArgs = {
		"-c", manifest.fileName()
	};
	process->start("md5sum", checksumArgs, QProcess::ReadOnly);
	state = UPDATE_CHECKSUMS;
}

MediaUpdate::~MediaUpdate()
{

}

void MediaUpdate::started()
{
	switch (state) {
		case UPDATE_CHECKSUMS:
			stepProgress("Validating Checksums");
			break;
		case UPDATE_PREINST:
			stepProgress("Running Preinstall Hooks");
			break;
		case UPDATE_DEBPKG:
			stepProgress("Installing Packages");
			break;
		case UPDATE_POSTINST:
			stepProgress("Running Postinstall Hooks");
			break;
	}
}

void MediaUpdate::parseManifest()
{
	const QRegExp whitespace("\\s+");

	QFile mFile(manifest.filePath());
	if (!mFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return;
	}

	QTextStream mStream(&mFile);
	while (!mStream.atEnd()) {
		QString line = mStream.readLine();
		QString filename = line.section(whitespace, 1, 1);
		if (filename == "preinst.sh") preinst = location.absoluteFilePath(filename);
		if (filename == "postinst.sh") postinst = location.absoluteFilePath(filename);
		if (filename.endsWith(".deb")) packages.append(filename);
	}
	mFile.close();
}

void MediaUpdate::finished(int code, QProcess::ExitStatus status)
{
	ui->log->appendPlainText("");

	/* Handle failed exit conditions. */
	if (code != 0) {
		ui->title->setStyleSheet("QLabel {font-weight: bold; color: red; }");
		ui->title->setText("Update Failed");
		startReboot(30);
		return;
	}

	switch (state) {
		case UPDATE_CHECKSUMS: {
			parseManifest();

			/* Estimate the number of steps on the progress bar */
			int steps = 2*packages.count() + 2;
			if (packages.count()) steps++;
			if (!preinst.isEmpty()) steps++;
			if (!postinst.isEmpty()) steps++;
			ui->progress->setMaximum(steps);

			/* If a pre-install hook exists, run it. */
			if (!preinst.isEmpty()) {
				state = UPDATE_PREINST;
				process->start(preinst, QProcess::ReadOnly);
				return;
			}
		}
		/* Fall-Through */

		case UPDATE_PREINST: {
			/* If debian packages exist, install them. */
			if (packages.count()) {
				state = UPDATE_DEBPKG;
				QStringList dpkgArgs = {"--status-fd=1", "-i"};
				process->start("dpkg", dpkgArgs + packages, QProcess::ReadOnly);
				return;
			}
		}
		/* Fall-Through */

		case UPDATE_DEBPKG: {
			/* If a post-install hook exists, run it. */
			if (!postinst.isEmpty()) {
				state = UPDATE_POSTINST;
				process->start(preinst, QProcess::ReadOnly);
				return;
			}
		}

		case UPDATE_POSTINST:
			stepProgress("Update Successful");
			startReboot(10);
			return;
	}
}

void MediaUpdate::handleStdout(QString msg)
{
	/* Special case when handling package installation. */
	if (state == UPDATE_DEBPKG) {
		/* Special case: parse the dpkg status format and use it to increment the progress bar. */
		if (msg.startsWith("status: ")) return;
		if (msg.startsWith("processing: ")) {
			QString value = msg.section(QRegExp("\\s+"), 1, 1);
			if (value == "upgrade:") stepProgress();
			else if (value == "install:") stepProgress();
			else if (value == "configure:") stepProgress();
			return;
		}
	}

	/* Otherwise, do the normal thing and log it. */
	log(msg);
}
