
#include <QObject>
#include <QTimer>

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <poll.h>
#include "video.h"
#include "camera.h"

void catch_sigchild(int sig) { /* nop */ }

Int32 Video::init(void)
{
	signal(SIGCHLD, catch_sigchild);
	return SUCCESS;
}

bool Video::setRunning(bool run)
{
	if(!running && run) {
		int child = fork();
		if (child < 0) {
			/* Could not start the video pipeline. */
			return false;
		}
		else if (child == 0) {
			/* child process - start the pipeline */
			const char *path = "/opt/camera/cam-pipeline";
			char display[64];
			char offset[64];
			snprintf(display, sizeof(display), "%ux%u", displayWindowXSize, displayWindowYSize);
			snprintf(offset, sizeof(offset), "%ux%u", displayWindowXOff, displayWindowYOff);
			execl(path, path, display, "--offset", offset, NULL);
			exit(EXIT_FAILURE);
		}
		pid = child;
		running = true;
	}
	else if(running && !run) {
		int status;
		kill(pid, SIGINT);
		waitpid(pid, &status, 0);
		running = false;
	}
	return true;
}

CameraErrortype Video::setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY)
{
	/* TODO: Replace with a SIGUSR to change focus aid scaling. */
	return SUCCESS;
}

UInt32 Video::getPosition(void)
{
	ComKrontechChronosVideoInterface vif("com.krontech.chronos.video", "/com/krontech/chronos/video", QDBusConnection::systemBus());
	QVariantMap reply;
	QDBusError err;

	pthread_mutex_lock(&mutex);
	reply = vif.status();
	err = vif.lastError();
	pthread_mutex_unlock(&mutex);

	if (err.isValid()) {
		return 0;
	}
	return reply["position"].toUInt();
}

void Video::setPosition(unsigned int position, int rate)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	args.insert("framerate", QVariant(rate));
	args.insert("position", QVariant(position));

	pthread_mutex_lock(&mutex);
	reply = iface.playback(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set playback position: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

void Video::setPlayback(int rate)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	args.insert("framerate", QVariant(rate));

	pthread_mutex_lock(&mutex);
	reply = iface.playback(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set playback rate: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

void Video::setDisplayOptions(bool zebra, bool peaking)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	args.insert("zebra", QVariant(zebra));
	args.insert("peaking", QVariant(peaking));

	pthread_mutex_lock(&mutex);
	reply = iface.liveflags(args);
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set live display mode: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

void Video::liveDisplay(void)
{
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.livedisplay();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);
}

void Video::addRegion(UInt32 base, UInt32 size, UInt32 offset)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap region;

	region.insert("base", QVariant(base));
	region.insert("size", QVariant(size));
	region.insert("offset", QVariant(offset));

	pthread_mutex_lock(&mutex);
	reply = iface.addregion(region);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set video region: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

Video::Video() : iface("com.krontech.chronos.video", "/com/krontech/chronos/video", QDBusConnection::systemBus())
{
	pid = -1;
	running = false;

	/* Use the full width with QWS because widgets can autohide. */
#ifdef Q_WS_QWS
	displayWindowXSize = 800;
#else
	displayWindowXSize = 600;
#endif
	displayWindowYSize = 480;
	displayWindowXOff = 0;
	displayWindowYOff = 0;

	pthread_mutex_init(&mutex, NULL);
}

Video::~Video()
{
	setRunning(false);
	pthread_mutex_destroy(&mutex);
}
