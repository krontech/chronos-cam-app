#include "updatewindow.h"
#include "ui_updatewindow.h"

#include <QDesktopWidget>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

const char *data_fifo_path = "/tmp/update.fifo";

static void data_atexit(void) { unlink(data_fifo_path); }

/* Data message handler */
void *data_thread(void *arg)
{
	char buf[1024];
	FILE *fp;

	/* Attempt to create the frame-grabber FIFO */
	unlink(data_fifo_path);
	if (mkfifo(data_fifo_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
		qDebug("Failed to create FIFO: %s", strerror(errno));
		return NULL;
	}
	/* Ensure we cleanup after ourselves */
	atexit(data_atexit);

	while ((fp = fopen(data_fifo_path, "r")) != NULL) {
		char *line;
		while ((line = fgets(buf, sizeof(buf), fp)) != NULL) {
			/* strip trailing whitespace */
			char *end = strchr(line, '\0') - 1;
			while (end > line && isspace(*end)) *end-- = '\0';

			/* Pass the message to the widget */
			QString msg(buf);
			QMetaObject::invokeMethod((QObject *)arg, "message", Qt::QueuedConnection, Q_ARG(QString, msg));
		}
		if (ferror(fp)) {
			qDebug("Error occured while reading FIFO: %s", strerror(errno));
		}
		/* Close and re-open the socket */
		fclose(fp);
	}
	/* Remove the socket if it already happens to exist. */
	return NULL;
}

UpdateWindow::UpdateWindow(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::UpdateWindow)
{
	QRect screen = QApplication::desktop()->screenGeometry();

	ui->setupUi(this);
	setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	move((screen.width() - width()) / 2, (screen.height() - height()) / 2);

	/* Launch a thread to manage the gather progress input. */
	pthread_create(&handle, NULL, data_thread, this);

	/* Progress bar should start at 0% */
	ui->progress->setMaximum(100);
	ui->progress->setValue(0);

	ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");
}

UpdateWindow::~UpdateWindow()
{
	delete ui;
}

void UpdateWindow::message(QString msg)
{
	QRegExp whitespace("\\s+");
	QString command;
	QString value;

	/* Special case: parse the dpkg status format and use it to increment the progress bar. */
	if (msg.startsWith("status: ")) return;
	if (msg.startsWith("processing: ")) {
		value = msg.section(whitespace, 1, 1);
		int count = ui->progress->value();
		if (count < ui->progress->maximum()) {
			if (value == "upgrade:") ui->progress->setValue(count+1);
			else if (value == "install:") ui->progress->setValue(count+1);
			else if (value == "configure:") ui->progress->setValue(count+1);
		}
		return;
	}

	/* If the line does not begin with a percent, then just update the status text. */
	if (!msg.startsWith('%')) {
		ui->label->setText(msg);
		return;
	}

	/* Otherwise, we have some special command */
	command = msg.section(whitespace, 0, 0);
	value = msg.remove(0, command.length()).trimmed();
	if (command == "%error") {
		ui->title->setText(QString(value));
		ui->title->setStyleSheet("QLabel {font-weight: bold; color: red; }");
	}
	else if (command == "%title") {
		ui->title->setText(QString(value));
		ui->title->setStyleSheet("QLabel {font-weight: bold; color: black; }");
	}
	else if (command == "%count") {
		int count = value.toInt();
		ui->progress->setMaximum(count);
		if (ui->progress->value() > count) ui->progress->setValue(count);
	}
	else if (command == "%step") {
		int count = ui->progress->value();
		if (count < ui->progress->maximum()) ui->progress->setValue(count+1);
	}
	else if (command == "%exit") {
		QApplication::quit();
	}
	/* Otherwise, it's probably a progress percentage */
	else {
		int count = command.remove(0, 1).toInt();
		ui->progress->setValue(count);
	}
}
