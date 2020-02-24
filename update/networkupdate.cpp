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
	repoCount = 0;
	packageCount = 0;
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
	int reply;

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
			if (system(checkpkg) != 0) {
				/* Install the chronos-essentials metapackage */
				ui->progress->setValue(0);
				ui->progress->setMaximum(100); /* Just a random guess for now... */
				QStringList aptInstallArgs = { "--assume-no", "--dry-run", "install", "chronos-essential"};
				process->start("apt-get", aptInstallArgs, QProcess::ReadOnly);
				state = NETWORK_PREPARE_METAPACKAGE;
			}
			else {
				/* Perform a dry-run to calculate the dist-upgrade. */
				ui->progress->setValue(0);
				ui->progress->setMaximum(100); /* Just a random guess for now... */

				QStringList aptUgradeArgs = { "--assume-no", "--dry-run", "dist-upgrade" };
				process->start("apt-get", aptUgradeArgs, QProcess::ReadOnly);
				state = NETWORK_PREPARE_UPGRADE;
			}
			break;
		}

		case NETWORK_PREPARE_METAPACKAGE: {
			/* Start installation of the chronos-essentials package */
			ui->progress->setValue(1);
			ui->progress->setMaximum(packageCount + 4);

			QStringList aptInstallArgs = { "-y", "install", "chronos-essential" };
			process->start("apt-get", aptInstallArgs, QProcess::ReadOnly);
			state = NETWORK_INSTALL_METAPACKAGE;
			break;
		}

		case NETWORK_INSTALL_METAPACKAGE:{
			/* Perform a dry-run to calculate the dist-upgrade. */
			ui->progress->setValue(0);
			ui->progress->setMaximum(100); /* Just a random guess for now... */

			QStringList aptUgradeArgs = { "--assume-no", "--dry-run", "dist-upgrade" };
			process->start("apt-get", aptUgradeArgs, QProcess::ReadOnly);
			state = NETWORK_PREPARE_UPGRADE;
			break;
		}

		case NETWORK_PREPARE_UPGRADE:{
			/* Prompt the user about our findings, and ask to continue. */
			if (packageCount == 0) {
				reply = QMessageBox::information(this, "apt update", "No updates were found.", QMessageBox::Ok);
				close();
				return;
			}
			reply = QMessageBox::question(this, "apt update", QString("Found %1 packages eligible for upgrade, download and install them?").arg(packageCount), QMessageBox::Yes|QMessageBox::No);
			if (reply != QMessageBox::Yes) {
				close();
				return;
			}

			/*
			 * Steps during an upgrade:
			 *   'Reading package lists...'
			 *   'Building dependency tree...'
			 *   'Reading state information...'
			 *
			 * For each package there is also:
			 *   'Unpacking <package> ...'
			 */
			ui->progress->setValue(1);
			ui->progress->setMaximum(packageCount + 4);

			/* Continue with the upgrade. */
			QStringList aptUpgradeArgs = {"-y", "dist-upgrade"};
			process->start("apt-get", aptUpgradeArgs, QProcess::ReadOnly);
			state = NETWORK_INSTALL_UPGRADE;
			break;
		}

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
		QString first = msg.section(QRegExp("\\s+"), 0, 0);
		if ((first == "Get") || (first == "Hit")) {
			stepProgress();
		}
		else if ((first == "Fetched") || (first == "Reading")) {
			stepProgress();
		}
	}
	else if ((state == NETWORK_PREPARE_METAPACKAGE) || (state == NETWORK_PREPARE_UPGRADE)) {
		QStringList list = msg.split(",");
		QString::SectionFlags flags = QString::SectionSkipEmpty;
		QRegExp whitespace = QRegExp("\\s+");
		if (list.count() < 3) return;

		packageCount = list[0].section(whitespace, 0, 0, flags).toUInt();  /* %d upgraded */
		packageCount += list[1].section(whitespace, 0, 0, flags).toUInt(); /* %d newly installed */
		packageCount += list[2].section(whitespace, 0, 0, flags).toUInt(); /* %d to remove */
		/* and %d not upgraded */
	}
	else if ((state == NETWORK_INSTALL_UPGRADE) || (state == NETWORK_INSTALL_METAPACKAGE)) {
		if (msg.isEmpty()) return; /* eat empty lines, since there's a lot of them */
		if (msg.startsWith("Reading Package lists")) stepProgress();
		else if (msg.startsWith("Building dependency tree")) stepProgress();
		else if (msg.startsWith("Reading state information")) stepProgress();
		else if (msg.startsWith("Unpacking")) stepProgress();
	}
	log(msg);
}
