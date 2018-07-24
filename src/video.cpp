
#include <QObject>
#include <QTimer>

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <QDebug>
#include <memory.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

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


void Video::reload(void)
{
	if(running) {
		kill(pid, SIGHUP);
	}
}

CameraErrortype Video::setScaling(UInt32 startX, UInt32 startY, UInt32 cropX, UInt32 cropY)
{
	/* TODO: Replace with a SIGUSR to change focus aid scaling. */
	return SUCCESS;
}

VideoState Video::getStatus(VideoStatus *st)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	VideoState state;

	pthread_mutex_lock(&mutex);
	reply = iface.status();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);
	if (reply.isError()) {
		/* TODO: Error handling */
		state = VIDEO_STATE_LIVEDISPLAY;
	}
	map = reply.value();
	if (map["recording"].toBool()) {
		state = VIDEO_STATE_RECORDING;
	}
	else if (map["playback"].toBool()) {
		state = VIDEO_STATE_PLAYBACK;
	}
	else {
		state = VIDEO_STATE_LIVEDISPLAY;
	}

	if (st) {
		memset(st, 0, sizeof(VideoState));
		st->state = state;
		st->totalFrames = map["totalFrames"].toUInt();
		st->position = map["position"].toUInt();
		st->framerate = map["framerate"].toDouble();
	}
	return state;
}

UInt32 Video::getPosition(void)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	pthread_mutex_lock(&mutex);
	reply = iface.status();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);
	if (reply.isError()) {
		return 0;
	}
	map = reply.value();
	return map["position"].toUInt();
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

void Video::loopPlayback(unsigned int start, unsigned int length, int rate)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	args.insert("framerate", QVariant(rate));
	args.insert("position", QVariant(start));
	args.insert("loopcount", QVariant(length));

	pthread_mutex_lock(&mutex);
	reply = iface.playback(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to start playback loop: %s - %s\n", err.name().data(), err.message().toAscii().data());
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

static int path_is_mounted(const char *path)
{
	char tmp[PATH_MAX];
	struct stat st;
	struct stat parent;

	/* Get the stats for the given path and check that it's a directory. */
	if ((stat(path, &st) != 0) || !S_ISDIR(st.st_mode)) {
		return FALSE;
	}

	/* Ensure that the parent directly is mounted on a different device. */
	snprintf(tmp, sizeof(tmp), "%s/..", path);
	return (stat(tmp, &parent) == 0) && (parent.st_dev != st.st_dev);
}

int Video::mkfilename(char *path, save_mode_type save_mode)
{
	char fname[1000];
	char sequencePath[1000];

	if(strlen(fileDirectory) == 0)
		return RECORD_NO_DIRECTORY_SET;

	strcpy(path, fileDirectory);

	if(strlen(filename) == 0)
	{
		//Fill timeinfo structure with the current time
		time_t rawtime;
		struct tm * timeinfo;

		time (&rawtime);
		timeinfo = localtime (&rawtime);

		sprintf(fname, "/vid_%04d-%02d-%02d_%02d-%02d-%02d",
					timeinfo->tm_year + 1900,
					timeinfo->tm_mon + 1,
					timeinfo->tm_mday,
					timeinfo->tm_hour,
					timeinfo->tm_min,
					timeinfo->tm_sec);
		strcat(path, fname);
	}
	else
	{
		strcat(path, "/");
		strcat(path, filename);
	}

	switch(save_mode) {
	case SAVE_MODE_H264:
		strcat(path, ".mp4");
		break;
	case SAVE_MODE_RAW16:
	case SAVE_MODE_RAW16RJ:
	case SAVE_MODE_RAW12:
		strcat(path, ".raw");
		break;
	case SAVE_MODE_DNG:
		break;
	}

	//If a file of this name already exists
	struct stat buffer;
	if(stat (path, &buffer) == 0)
	{
		return RECORD_FILE_EXISTS;
	}

	//Check that the directory is writable
	if(access(fileDirectory, W_OK) != 0)
	{	//Not writable
		return RECORD_DIRECTORY_NOT_WRITABLE;
	}
	return SUCCESS;
}

CameraErrortype Video::startRecording(UInt32 sizeX, UInt32 sizeY, UInt32 start, UInt32 length, save_mode_type save_mode)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	struct statvfs statBuf;
	UInt64 estFileSize;
	char path[1000];

	/* Generate the desired filename, and check that we can write it. */
	mkfilename(path, save_mode);
	//fstatvfs(fd, &statBuf);
	//UInt64 freeSpace = statBuf.f_bsize * statBuf.f_bfree;
	UInt64 freeSpace = 0xffffffff; /* TODO: FIXME! */

	/* Attempt to start the video recording process. */
	map.insert("filename", QVariant(path));
	map.insert("start", QVariant(start));
	map.insert("length", QVariant(length));
	switch(save_mode) {
	case SAVE_MODE_H264:
		estFileSize = min(bitsPerPixel * sizeX * sizeY * framerate, min(60000000, (UInt32)(maxBitrate * 1000000.0)) * framerate / 60) / framerate * length / 8;//bitsPerPixel * imgXSize * imgYSize * numFrames / 8;
		map.insert("format", QVariant("h264"));
		map.insert("bitrate", QVariant((uint)maxBitrate));
		break;
	case SAVE_MODE_RAW16:
		estFileSize = 16 * sizeX * sizeY * length / 8;
		map.insert("format", QVariant("y16"));
		break;
	case SAVE_MODE_RAW16RJ:
		estFileSize = 16 * sizeX * sizeY * length / 8;
		map.insert("format", QVariant("y12"));
		break;
	case SAVE_MODE_RAW12:
		estFileSize = 12 * sizeX * sizeY * length / 8;
		map.insert("format", QVariant("y12b"));
		break;
	case SAVE_MODE_DNG:
		estFileSize = 16 * sizeX * sizeY * length / 8;
		estFileSize += (4096 * length);
		map.insert("format", QVariant("dng"));
		break;
	}
	qDebug() << "---- Video Record ---- Estimated file size:" << estFileSize << "bytes, free space:" << freeSpace << "bytes.";
	printf("Saving video to %s\r\n", path);

	/* Send the DBus command to be*/
	pthread_mutex_lock(&mutex);
	reply = iface.recordfile(map);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug() << "Recording failed with error " << err.message();
		return RECORD_ERROR;
	}
	else {
		return SUCCESS;
	}
}

CameraErrortype Video::stopRecording()
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	pthread_mutex_lock(&mutex);
	reply = iface.status();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);
	if (reply.isError()) {
		return RECORD_ERROR;
	}
	map = reply.value();
	return SUCCESS;
}

double Video::getFramerate()
{
	/* TODO: Implement Me! */
	return 3.14159;
}

Video::Video() : iface("com.krontech.chronos.video", "/com/krontech/chronos/video", QDBusConnection::systemBus())
{
	QDBusConnection conn = iface.connection();
	int i;

	eosCallback = NULL;
	eosCallbackArg = NULL;
	errorCallback = NULL;
	errorCallbackArg = NULL;
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

	/* Prepare the recording parameters */
	bitsPerPixel = 0.7;
	maxBitrate = 40.0;
	framerate = 60;
	profile = OMX_H264ENC_PROFILE_HIGH;
	level = OMX_H264ENC_LVL_51;
	strcpy(filename, "");

	/* Set the default file path, or fall back to the MMC card. */
	for (i = 1; i <= 3; i++) {
		sprintf(fileDirectory, "/media/sda%d", i);
		if (path_is_mounted(fileDirectory)) {
			return;
		}
	}
	strcpy(fileDirectory, "/media/mmcblk1p1");


	pthread_mutex_init(&mutex, NULL);

	/* Connect DBus signals */
	conn.connect("com.krontech.chronos.video", "/com/krontech/chronos/video", "com.krontech.chronos.video",
				 "sof", this, SLOT(sof(const QVariantMap&)));
	conn.connect("com.krontech.chronos.video", "/com/krontech/chronos/video", "com.krontech.chronos.video",
				 "eof", this, SLOT(eof(const QVariantMap&)));
}

Video::~Video()
{
	setRunning(false);
	pthread_mutex_destroy(&mutex);
}

