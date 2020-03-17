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
#include <mntent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>
#include <unistd.h>

#include <QDebug>

#include "mediaupdate.h"
#include "networkupdate.h"
#include "packagelist.h"
#include "updatewindow.h"
#include "ui_updatewindow.h"

#include "utils.h"

UpdateWindow::UpdateWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::UpdateWindow)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	recenterWidget(this);

	/*
	 * Check if we are in rescue mode, if so we may need to manually
	 * start networking, since we will be in single-user mode without
	 * networking.
	 */
	inRescueMode = system("systemctl is-active rescue.target") == 0;
	if (inRescueMode && !checkForNetwork()) {
		/* If the link is not up, start dhclient to bring it up */
		dhclient = new QProcess(this);
		connect(dhclient, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(dhclientFinished(int, QProcess::ExitStatus)));
		connect(dhclient, SIGNAL(error(QProcess::ProcessError)), this, SLOT(dhclientError(QProcess::ProcessError)));
		dhclient->start("dhclient", QStringList("eth0"), QProcess::ReadOnly);
	}
	/* Run a timer to scan the media devices for updates. */
	mediaTimer = new QTimer(this);
	connect(mediaTimer, SIGNAL(timeout()), this, SLOT(checkForMedia()));
	mediaTimer->start(1000);

	/* Check which, if any of the GUIs are enabled. */
	ui->comboGui->addItem("None (Headless)");
	ui->comboGui->addItem("Chronos");
	ui->comboGui->addItem("GUI2 (Experimental)");
	if (system("systemctl is-enabled chronos-gui") == 0) {
		ui->comboGui->setCurrentIndex(1);
	}
	else if (system("systemctl is-enabled chronos-gui2") == 0) {
		ui->comboGui->setCurrentIndex(2);
	}
	else {
		ui->comboGui->setCurrentIndex(0);
	}
	ui->cmdApplyGui->setEnabled(false);

	/* Parse the apt sources file to figure out what release we're on. */
	/* TODO: We should get a list of the available releases from somewhere */
	QFile releaseFile("/etc/apt/sources.list.d/krontech-debian.list");
	if (releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream fileStream(&releaseFile);
		QRegExp whitespace = QRegExp("\\s+");
		while (!fileStream.atEnd()) {
			QStringList slist = fileStream.readLine().split(whitespace);
			if (slist.count() < 4) continue;
			if (slist[0] != "deb") continue;
			ui->comboRelease->addItem(slist[2], slist[1]);
		}
		releaseFile.close();
	}

	aptCheck = new QProcess(this);
	connect(aptCheck, SIGNAL(started()), this, SLOT(started()));
	connect(aptCheck, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	connect(aptCheck, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
	connect(aptCheck, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));

	/* Start a process to get the number of packages known for update. */
	QStringList aptCheckArgs = {
		"list", "--upgradable"
	};
	aptUpdateCount = 0;
	aptCheck->start("apt", aptCheckArgs, QProcess::ReadOnly);
}

UpdateWindow::~UpdateWindow()
{
	delete mediaTimer;
	delete aptCheck;
	delete ui;
}

bool UpdateWindow::checkForNetwork()
{
	/* Check if eth0 is up */
	FILE *fp = fopen("/sys/class/net/eth0/operstate", "r");
	char buf[16] = {'\0'};
	if (!fp) false;
	fgets(buf, sizeof(buf), fp);
	fclose(fp);
	return strncmp(buf, "up", 2) == 0;
}

void UpdateWindow::checkForMedia()
{
	FILE* mtab = setmntent("/etc/mtab", "r");
	char mpath[PATH_MAX];
	int count = ui->comboMedia->count();
	int stale = (1 << count) - 1;
	struct mntent* m;
	struct mntent mnt;

	while ((m = getmntent_r(mtab, &mnt, mpath, sizeof(mpath)))) {
		struct stat st;
		char manifest[PATH_MAX];
		int isusb, ismmc;
		int index;

		/* Sanity check the mount */
		if (mnt.mnt_dir == NULL) continue;
		isusb = (strstr(mnt.mnt_dir, "/media/mmcblk1") != NULL);
		ismmc = (strstr(mnt.mnt_dir, "/media/sd") != NULL);
		if (!isusb && !ismmc) continue;

		/* Look for a software update on the media device. */
		snprintf(manifest, sizeof(manifest), "%s/camUpdate/manifest.md5sum", mnt.mnt_dir);
		manifest[sizeof(manifest)-1] = '\0'; /* just in case */
		if (stat(manifest, &st) != 0) continue;
		if (!S_ISREG(st.st_mode)) continue;

		/* Add the update if doesn't already exist. */
		index = ui->comboMedia->findData(QString(manifest));
		if (index < 0) {
			ui->comboMedia->addItem(QString(mnt.mnt_dir), QString(manifest));
		}
		/* Indicate that it's still there */
		else {
			stale &= ~(1 << index);
		}
	}
	endmntent(mtab);

	/* Remove any items that disappeared */
	while (count >= 0) {
		if (stale & (1 << count)) ui->comboMedia->removeItem(count);
		count--;
	}

	ui->comboMedia->setEnabled(ui->comboMedia->count() > 0);
	ui->cmdApplyMedia->setEnabled(ui->comboMedia->count() > 0);

	/* Check for network link (excluding recovery mode, which uses dhclient) */
	if (!inRescueMode) {
		ui->cmdApplyNetwork->setEnabled(checkForNetwork());
	}
}

void UpdateWindow::updateClosed()
{
	this->show();
}

void UpdateWindow::on_cmdApplyMedia_clicked()
{
	int index = ui->comboMedia->currentIndex();
	MediaUpdate *w = new MediaUpdate(ui->comboMedia->itemData(index).toString(), NULL);
	this->hide();
	connect(w, SIGNAL(destroyed()), this, SLOT(updateClosed()));
	w->show();
}

void UpdateWindow::on_cmdApplyNetwork_clicked()
{
	NetworkUpdate *w = new NetworkUpdate(NULL);
	this->hide();
	connect(w, SIGNAL(destroyed()), this, SLOT(updateClosed()));
	w->show();
}

void UpdateWindow::on_comboGui_currentIndexChanged(int index)
{
	ui->cmdApplyGui->setEnabled(true);
}

void UpdateWindow::on_cmdApplyGui_clicked()
{
	int index = ui->comboGui->currentIndex();
	if (index == 1) {
		system("systemctl disable chronos-gui2");
		system("systemctl enable chronos-gui");
	}
	else if (index == 2) {
		system("systemctl disable chronos-gui");
		system("systemctl enable chronos-gui2");
	}
	else {
		system("systemctl disable chronos-gui");
		system("systemctl disable chronos-gui2");
	}
}

void UpdateWindow::on_cmdListSoftware_clicked()
{
	PackageList *w = new PackageList(NULL);
	this->hide();
	connect(w, SIGNAL(destroyed()), this, SLOT(updateClosed()));
	w->show();
}

void UpdateWindow::on_cmdReboot_clicked()
{
	system("shutdown -hr now");
	QApplication::quit();
}

void UpdateWindow::on_cmdQuit_clicked()
{
	int index = ui->comboGui->currentIndex();
	if (inRescueMode) {
		/*
		 * What is the *appropriate* way to exit rescue mode?
		 * Attempting to reboot the system just results in the
		 * system hanging on the recovery shell.
		 */
		system("killall -HUP sulogin");
	}
	else if (index == 1) {
		system("service chronos-gui start");
	}
	else if (index == 2) {
		system("service chronos-gui2 start");
	}
	QApplication::quit();
}

void UpdateWindow::started()
{
	qDebug("apt check started");
}

void UpdateWindow::finished(int code, QProcess::ExitStatus status)
{
	qDebug("apt check finished: %d", code);
}

void UpdateWindow::error(QProcess::ProcessError err)
{
	qDebug("apt check failed: %d", err);
}

void UpdateWindow::dhclientFinished(int code, QProcess::ExitStatus status)
{
	if ((status == QProcess::NormalExit) && (code == 0)) {
		ui->cmdApplyNetwork->setEnabled(true);
	}
}

void UpdateWindow::dhclientError(QProcess::ProcessError err)
{
	qDebug("dhclient failed: %d", err);
}

void UpdateWindow::readyReadStandardOutput()
{
	QString msg;
	char buf[1024];
	qint64 len;

	aptCheck->setReadChannel(QProcess::StandardOutput);
	len = aptCheck->readLine(buf, sizeof(buf));
	if (len < 0) {
		return;
	}
	msg = QString(buf).trimmed();

	if (msg.contains("upgradable")) aptUpdateCount++;
	qDebug() << "apt:" << msg;
}
