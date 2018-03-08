
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

//setRunning: Turn the video off, or turn it on. Requires the area of the screen to display the video on.
//(Turning on the video twice will restart the video, even if the parameters are the same. Do not depend on this, it is likely to change.)
bool Video::setRunning(void* area)
{
	if(area) { throw std::invalid_argument( "setRunning must be called with NULL or a QRect." ); }
	
	if(running) {
		int status;
		kill(pid, SIGINT);
		waitpid(pid, &status, 0);
		running = false;
	}
	
	return true;
}

bool Video::setRunning(QRect area)
{
	setRunning(NULL);
	
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
		//qDebug() << "__RES 1" << displayWindowXSize << displayWindowYSize << displayWindowXOff << displayWindowYOff;
		snprintf(display, sizeof(display), "%ux%u", area.width(), area.height());
		snprintf(offset, sizeof(offset), "%ux%u", area.left(), area.top());
		execl(path, path, display, "--offset", offset, NULL);
		exit(EXIT_FAILURE);
	}
	pid = child;
	running = true;
	
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
	QVariantMap reply;
	QDBusError err;
	args.insert("framerate", QVariant(rate));
	args.insert("position", QVariant(position));

	pthread_mutex_lock(&mutex);
	reply = iface.playback(args);
	err = iface.lastError();
	pthread_mutex_unlock(&mutex);

	if (err.isValid()) {
		printf("Failed to set playback position: %s - %s", err.name().data(), err.message().toAscii().data());
	}
}

void Video::setPlayback(int rate)
{
	QVariantMap args;
	QVariantMap reply;
	QDBusError err;
	args.insert("framerate", QVariant(rate));

	pthread_mutex_lock(&mutex);
	reply = iface.playback(args);
	err = iface.lastError();
	pthread_mutex_unlock(&mutex);

	if (err.isValid()) {
		printf("Failed to set playback rate: %s - %s", err.name().data(), err.message().toAscii().data());
	}
}

void Video::addRegion(UInt32 base, UInt32 size, UInt32 offset)
{
	QVariantMap region;
	QVariantMap reply;
	QDBusError err;

	region.insert("base", QVariant(base));
	region.insert("size", QVariant(size));
	region.insert("offset", QVariant(offset));

	pthread_mutex_lock(&mutex);
	reply = iface.addregion(region);
	err = iface.lastError();
	pthread_mutex_unlock(&mutex);

	if (err.isValid()) {
		printf("Failed to set video region: %s - %s", err.name().data(), err.message().toAscii().data());
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
	setRunning(NULL);
	pthread_mutex_destroy(&mutex);
}
