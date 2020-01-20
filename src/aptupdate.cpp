

/****************************************************************************
 *  Copyright (C) 2013-2019 Kron Technologies Inc <http://www.krontech.ca>. *
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
#include <QLabel>
#include <QMessageBox>

#include "util.h"
#include "aptupdate.h"

AptUpdate::AptUpdate(QWidget * parent)
{
	this->parent = parent;

	/* Create the progress dialog for the repository update process. */
	progUpdate = new QProgressDialog(parent);
	progUpdate->setWindowTitle(QString("apt-get update"));
	progUpdate->setMinimumSize(500, 200);
	progUpdate->setMaximum(4);
	progUpdate->setMinimumDuration(0);
	progUpdate->setWindowModality(Qt::WindowModal);

	/* Create the progress dialog for the package upgrade process. */
	progUpgrade = new QProgressDialog(parent);
	progUpdate->setWindowTitle(QString("apt-get ugrade"));
	progUpdate->setMinimumSize(500, 200);
	progUpdate->setMaximum(4);
	progUpdate->setMinimumDuration(0);
	progUpdate->setWindowModality(Qt::WindowModal);
}

AptUpdate::~AptUpdate()
{
	delete progUpdate;
	delete progUpgrade;
}

/* Run the apt-get upgrade progress. */
int AptUpdate::exec()
{
	QMessageBox::StandardButton reply;

	char line[256];
	unsigned int uri_count = 0;
	unsigned int uri_progress = 0;
	unsigned int pkg_count = 0;
	FILE *fp;
	pid_t child;

	progUpdate->setValue(0);
	progUpdate->show();

	/* Get the list of URIs required for an apt-get update. */
	fp = popen("apt-get update --print-uris", "r");
	if (!fp) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::critical(NULL, "apt update failed", "Failed to start apt update with error?", QMessageBox::Ok);
		return -1;
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		uri_count++;
		qDebug("apt-get uri%d -> %s", uri_count, line);
	}
	pclose(fp);
	/*
	 * Additional steps:
	 *   'print-uris' is step zero.
	 *   'Fetched' output from apt-get
	 *   'Reading package list' from apt-get
	 *   'Listing...' from apt list
	 */
	uri_progress = 1;
	progUpdate->setMaximum(uri_count+4);
	progUpdate->setValue(uri_progress);

	stIndex = 0;
	memset(stLabel, 0, sizeof(stLabel));
	fp = popen("apt update", "r");
	if (!fp) {
		reply = QMessageBox::critical(parent, "apt update failed", "Failed to start apt update with error?", QMessageBox::Ok);
		return -1;
	}
	while (fgets(stLabel[stIndex], sizeof(stLabel[stIndex]), fp) != NULL) {
		/* Advance the progress bar on Get or Hit */
		if ((strncasecmp(stLabel[stIndex], "Get", 3) == 0) || (strncasecmp(stLabel[stIndex], "Hit", 3) == 0)) {
			uri_progress++;
		}
		else if (strncasecmp(stLabel[stIndex], "Fetched", 7) == 0) {
			uri_progress++;
		}
		else if (strncasecmp(stLabel[stIndex], "Reading", 7) == 0) {
			uri_progress++;
		}
		/* Update the progress bar status text. */
		QString stString = "";
		stIndex = (stIndex+1) % 4;
		for (int i = 0; i < 4; i++) {
			stString.append(stLabel[(stIndex + i) % 4]);
		}
		progUpdate->setLabelText(stString);
		progUpdate->setValue(uri_progress);
	}
	pclose(fp);
	progUpdate->hide();

	/* The last line should have listed the number of packages to be upgraded. */
	pkg_count = strtoul(stLabel[(stIndex + 3) % 4], NULL, 10);
	if (pkg_count == 0) {
		reply = QMessageBox::information(parent, "apt update", "No updates were found.", QMessageBox::Ok);
		return 0;
	}
	reply = QMessageBox::question(parent, "apt update", QString("Found %1 packages eligible for upgrade, download and install them?").arg(pkg_count), QMessageBox::Yes|QMessageBox::No);
	if (reply != QMessageBox::Yes) {
		return 0;
	}

	runBackground("apt-get -y upgrade; sleep 10; shutdown -hr now");
	reply = QMessageBox::information(parent, "apt upgrade", "Package update in progress. The camera will reboot when complete.", QMessageBox::NoButton);
	return 0;
}
