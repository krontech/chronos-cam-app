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
#include "networkupdate.h"
#include "ui_updateprogress.h"

#include <QTextStream>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>

NetworkUpdate::NetworkUpdate(QWidget *parent) : UpdateProgress(parent)
{
	/* Debian packaging must be prepared to run non-interractively */
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("DEBIAN_FRONTEND", "noninteractive");
	process->setProcessEnvironment(env);

	/* Figure out how many repositories need to be queried. */
	QStringList checksumArgs = { "update", "--print-uris" };
	process->start("apt-get", checksumArgs, QProcess::ReadOnly);
	userReply = 0;
	repoCount = 0;
	packageCount = 0;
	downloadFail = 0;
	state = NETWORK_COUNT_REPOS;
}

NetworkUpdate::~NetworkUpdate()
{

}

void NetworkUpdate::started()
{
	switch (state) {
		case NETWORK_COUNT_REPOS:
			stepProgress("Preparing Update");
			break;
		case NETWORK_UPDATE_LISTS:
			stepProgress("Updating Package Lists");
			break;
		case NETWORK_PREPARE_METAPACKAGE:
			stepProgress("Installing Metapackage");
			break;
		case NETWORK_INSTALL_METAPACKAGE:
			break;
		case NETWORK_PREPARE_UPGRADE:
			stepProgress("Upgrading Packages");
			break;
		case NETWORK_INSTALL_UPGRADE:
			break;
	}
}

void NetworkUpdate::finished(int code, QProcess::ExitStatus status)
{
	/* Handle failed exit conditions. */
	if (code != 0) {
		ui->title->setStyleSheet("QLabel {font-weight: bold; color: red; }");
		ui->title->setText("Update Failed");
		startReboot(30);
		return;
	}

	switch (state) {
		case NETWORK_COUNT_REPOS:{
			/*
			 * Additional steps:
			 *   'print-uris' is step zero.
			 *   'Updating Package Lists' is step 1.
			 *   'Fetched' output from apt-get
			 *   'Reading package list' from apt-get
			 *   'Listing...' from apt list
			 *   'Reading state information'
			 */
			ui->progress->setMaximum(repoCount + 6);

			QStringList aptUpdateArgs = { "update" };
			process->start("apt-get", aptUpdateArgs, QProcess::ReadOnly);
			state = NETWORK_UPDATE_LISTS;
			break;
		}

		case NETWORK_UPDATE_LISTS:{
			/* Ensure that the chronos-essential metapackage is installed. */
			const char *checkpkg = "dpkg-query --show --showformat=\'${db:Status-Status}\n\' chronos-essential | grep ^installed";
			if (downloadFail) {
				QString prompt = QString("Failed to donwload package lists, please check internet connectivity");
				userReply = QMessageBox::warning(this, "apt update", prompt, QMessageBox::Ok);
				close();
				return;
			}
			else if (system(checkpkg) == 0) {
				/* Perform a dry-run to calculate the dist-upgrade. */
				ui->progress->setValue(0);
				ui->progress->setMaximum(100); /* Just a random guess for now... */

				QStringList aptUgradeArgs = { "--assume-yes", "--print-uris", "dist-upgrade" };
				process->start("apt-get", aptUgradeArgs, QProcess::ReadOnly);
				downloadCount = 0;
				state = NETWORK_PREPARE_UPGRADE;
			}
			else {
				/* The user needs to install the chronos-essential metapackage */
				QString prompt = QString("Found packages eligible for upgrade, download and install them?");
				userReply = QMessageBox::question(this, "apt update", prompt, QMessageBox::Yes|QMessageBox::No);
				if (userReply != QMessageBox::Yes) {
					close();
					return;
				}

				/* Install the chronos-essentials metapackage */
				ui->progress->setValue(0);
				ui->progress->setMaximum(100); /* Just a random guess for now... */
				QStringList aptInstallArgs = { "--assume-yes", "--print-uris", "install", "chronos-essential"};
				process->start("apt-get", aptInstallArgs, QProcess::ReadOnly);
				downloadCount = 0;
				state = NETWORK_PREPARE_METAPACKAGE;
			}
			break;
		}

		case NETWORK_PREPARE_METAPACKAGE: {
			/* Start installation of the chronos-essentials package */
			ui->progress->setValue(1);
			ui->progress->setMaximum((packageCount * 2) + downloadCount + 4);

			QStringList aptInstallArgs = { "-y", "install", "chronos-essential" };
			process->start("apt-get", aptInstallArgs, QProcess::ReadOnly);
			state = NETWORK_INSTALL_METAPACKAGE;
			break;
		}

		case NETWORK_INSTALL_METAPACKAGE:{
			/* Perform a dry-run to calculate the dist-upgrade. */
			ui->progress->setValue(0);
			ui->progress->setMaximum(100); /* Just a random guess for now... */

			QStringList aptUgradeArgs = { "--assume-yes", "--print-uris", "dist-upgrade" };
			process->start("apt-get", aptUgradeArgs, QProcess::ReadOnly);
			state = NETWORK_PREPARE_UPGRADE;
			break;
		}

		case NETWORK_PREPARE_UPGRADE:
			if (userReply == 0) {
				/* Prompt the user about our findings, and ask to continue. */
				if (packageCount == 0) {
					userReply = QMessageBox::information(this, "apt update", "No updates were found.", QMessageBox::Ok);
					close();
					return;
				}
				userReply = QMessageBox::question(this, "apt update", QString("Found %1 packages eligible for upgrade, download and install them?").arg(packageCount), QMessageBox::Yes|QMessageBox::No);
				if (userReply != QMessageBox::Yes) {
					close();
					return;
				}
			}

			if (packageCount != 0) {
				/*
				 * Steps during an upgrade:
				 *   'Reading package lists...'
				 *   'Building dependency tree...'
				 *   'Reading state information...'
				 *
				 * For each package there is also:
				 *   Get:<n> <url> or Hit:<n> <url>
				 *   'Unpacking <package> ...'
				 */
				ui->progress->setValue(1);
				ui->progress->setMaximum((packageCount * 2) + downloadCount + 4);

				/* Continue with the upgrade. */
				QStringList aptUpgradeArgs = {"-y", "dist-upgrade"};
				process->start("apt-get", aptUpgradeArgs, QProcess::ReadOnly);
				state = NETWORK_INSTALL_UPGRADE;
				break;
			}
			/* Otherwise, fall through for a successful update */
		case NETWORK_INSTALL_UPGRADE: {
			state = NETWORK_REBOOT;
			stepProgress("Update Successful");
			startReboot(10);
			break;
		}
	}
}

void NetworkUpdate::handleStdout(QString msg)
{
	if (state == NETWORK_COUNT_REPOS) {
		repoCount++;
		return;
	}
	else if (state == NETWORK_UPDATE_LISTS) {
		QString first = msg.section(QRegExp("[:\\s]+"), 0, 0);
		if ((first == "Get") || (first == "Hit") || (first == "Ign")) {
			stepProgress();
		}
		else if (first == "Err") {
			stepProgress();
			downloadFail++;
		}
		else if ((first == "Fetched") || (first == "Reading")) {
			stepProgress();
		}
	}
	else if ((state == NETWORK_PREPARE_METAPACKAGE) || (state == NETWORK_PREPARE_UPGRADE)) {
		QRegExp whitespace = QRegExp("\\s+");
		if (msg.startsWith('\'')) {
			/* Possibly a URL to be parsed. */
			/* Expecting the form: 'url' packagename.deb size hash */
			QStringList list = msg.split(whitespace);
			if (list.count() != 4) return;
			if (!list[0].endsWith('\'')) return;
			downloadCount++;
		}
		else {
			/* Parse the summary line to determine how many packages will be changed */
			/* Format: %d upgraded, %d newly installed, %d to remove and %d not upgraded */
			/* Look for the size of the calculated upgrade. */
			QRegExp re("(\\d+)\\s+[,a-z\\s]+");
			int pos = re.indexIn(msg);
			QStringList list = re.capturedTexts();
			unsigned int numUpgrade;
			unsigned int numInstall;
			unsigned int numRemove;

			/* %d upgraded */
			if (pos != 0) return;
			numUpgrade = list[1].toUInt();
			pos = re.indexIn(msg, pos + list[0].length());

			/* %d newly installed */
			if (pos < 0) return;
			list = re.capturedTexts();
			numInstall = list[1].toUInt();
			pos = re.indexIn(msg, pos + list[0].length());

			/* %d to remove */
			if (pos < 0) return;
			list = re.capturedTexts();
			numRemove = list[1].toUInt();

			packageCount = numUpgrade + numInstall + numRemove;
		}
	}
	else if ((state == NETWORK_INSTALL_UPGRADE) || (state == NETWORK_INSTALL_METAPACKAGE)) {
		if (msg.isEmpty()) return; /* eat empty lines, since there's a lot of them */
		if (msg.startsWith("Reading Package lists")) stepProgress();
		else if (msg.startsWith("Building dependency tree")) stepProgress();
		else if (msg.startsWith("Reading state information")) stepProgress();
		/* Move the progress bar on package installation */
		else if (msg.startsWith("Unpacking")) stepProgress();
		else if (msg.startsWith("Setting up")) stepProgress();
		/* Move the progress bar on package downloads */
		else if (msg.startsWith("Get:")) stepProgress();
		else if (msg.startsWith("Hit:")) stepProgress();
		else if (msg.startsWith("Ign:")) stepProgress();
	}
	log(msg);
}
