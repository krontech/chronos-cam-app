#include "updatewindow.h"
#include "ui_updatewindow.h"

#include <QDesktopWidget>
#include <QTextStream>
#include <QTimer>
#include <QDebug>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

UpdateWindow::UpdateWindow(QString manifestPath, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::UpdateWindow)
{
	QRect screen = QApplication::desktop()->screenGeometry();

	ui->setupUi(this);
	setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	move((screen.width() - width()) / 2, (screen.height() - height()) / 2);

	process = new QProcess(this);
	connect(process, SIGNAL(started()), this, SLOT(started()));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
	connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));

	manifest = QFileInfo(manifestPath);
	location = manifest.dir();
	process->setWorkingDirectory(location.absolutePath());

	/* Prepare a timer to reboot */
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));

	/* Progress bar should start at 0% */
	ui->progress->setMaximum(100);
	ui->progress->setValue(0);
	ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");

	/* Start by validating the manifest hashes */
	QStringList checksumArgs = {
		"-c", manifest.fileName()
	};
	process->start("md5sum", checksumArgs, QProcess::ReadOnly);
	state = UPDATE_CHECKSUMS;
}

UpdateWindow::~UpdateWindow()
{
	delete ui;
}

void UpdateWindow::started()
{
	ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");
	switch (state) {
		case UPDATE_CHECKSUMS:
			ui->title->setText("Validating Checksums");
			break;
		case UPDATE_PREINST:
			ui->title->setText("Running Preinstall Hooks");
			break;
		case UPDATE_DEBPKG:
			ui->title->setText("Installing Packages");
			break;
		case UPDATE_POSTINST:
			ui->title->setText("Running Postinstall Hooks");
			break;
	}

	/* Increment the progress slider */
	int count = ui->progress->value();
	if (count < ui->progress->maximum()) ui->progress->setValue(count+1);
}

void UpdateWindow::parseManifest()
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

void UpdateWindow::startReboot(int secs)
{
	ui->progress->setValue(0);
	ui->progress->setMaximum(secs);
	state = UPDATE_REBOOT;
	countdown = secs;
	timer->start(1000);
}

void UpdateWindow::finished(int code, QProcess::ExitStatus status)
{
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
			ui->title->setText("Update Successful");
			startReboot(10);
			return;
	}
}

void UpdateWindow::error(QProcess::ProcessError err)
{
	qDebug("Process error in state %d: %d", state, (int)err);
}

void UpdateWindow::readyReadStandardOutput()
{
	QString msg;
	char buf[1024];
	qint64 len;

	process->setReadChannel(QProcess::StandardOutput);
	len = process->readLine(buf, sizeof(buf));
	if (len < 0) {
		return;
	}
	msg = QString(buf).trimmed();

	/* Special case when handling package installation. */
	if (state == UPDATE_DEBPKG) {
		/* Special case: parse the dpkg status format and use it to increment the progress bar. */
		if (msg.startsWith("status: ")) return;
		if (msg.startsWith("processing: ")) {
			QString value = msg.section(QRegExp("\\s+"), 1, 1);
			int count = ui->progress->value() + 1;
			if (count > ui->progress->maximum()) return;
			else if (value == "upgrade:") ui->progress->setValue(count);
			else if (value == "install:") ui->progress->setValue(count);
			else if (value == "configure:") ui->progress->setValue(count);
			return;
		}
	}

	/* Otherwise, just display it. */
	qDebug() << msg;
	ui->label->setText(msg);
}

void UpdateWindow::readyReadStandardError()
{
	QString msg;
	char buf[1024];
	qint64 len;

	process->setReadChannel(QProcess::StandardError);
	len = process->readLine(buf, sizeof(buf));
	if (len > 0) {
		msg = QString(buf).trimmed();
		qDebug() << msg;
		ui->label->setText(buf);
	}
}

void UpdateWindow::timeout()
{
	int count = ui->progress->value();
	if (count < ui->progress->maximum()) ui->progress->setValue(count+1);
	ui->label->setText(QString("Restarting in %1").arg(countdown));
	if (!countdown--) {
		QApplication::quit();
	}
}
