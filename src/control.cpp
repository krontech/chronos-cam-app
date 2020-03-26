
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <QDebug>
#include <QEventLoop>
#include <QMessageBox>
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
#include "control.h"
#include "camera.h"
#include "util.h"

QVariant Control::getProperty(QString parameter, const QVariant &defval)
{
	QStringList args(parameter);
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	pthread_mutex_lock(&mutex);
	reply = iface.get(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug() << "Failed to get parameter: " + err.name() + " - " + err.message();
		return defval;
	}

	map = reply.value();
	if (map.contains("error")) {
		QVariantMap errdict;
		map["error"].value<QDBusArgument>() >> errdict;

		qDebug() << "Failed to get parameter: " + errdict[parameter].toString();
		return defval;
	}

	return map[parameter];
}

/* Get multiple properties. */
QVariantMap Control::getPropertyGroup(QStringList paramters)
{
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.get(paramters);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug() << "Failed to get parameters: " + err.name() + " - " + err.message();
		return QVariantMap();
	}

	return reply.value();
}

/* Set a single property */
CameraErrortype Control::setProperty(QString parameter, QVariant value)
{
	QVariantMap args;
	QVariantMap map;
	QDBusPendingReply<QVariantMap> reply;

	args.insert(parameter, value);

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug() << "Failed to set parameter " + parameter + ": " + err.name() + " - " + err.message();
		return CAMERA_API_CALL_FAIL;
	}

	map = reply.value();
	if (map.contains("error")) {
		QVariantMap errdict;
		map["error"].value<QDBusArgument>() >> errdict;

		qDebug() << "Failed to set parameter " + parameter + ": " + errdict[parameter].toString();
		return CAMERA_API_CALL_FAIL;
	}

	return SUCCESS;
}

/* Attempt to set multiple properties together as a group. */
CameraErrortype Control::setPropertyGroup(QVariantMap values)
{
	QVariantMap map;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.set(values);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug() << "Failed to set parameters: " + err.name() + " - " + err.message();
		return CAMERA_API_CALL_FAIL;
	}

	map = reply.value();
	if (map.contains("error")) {
		QVariantMap errdict;
		QVariantMap::const_iterator i;
		map["error"].value<QDBusArgument>() >> errdict;

		for (i = errdict.begin(); i != errdict.end(); i++) {
			qDebug() << "Failed to set parameter " + i.key() + ": " + i.value().toString();
		}
		return CAMERA_API_CALL_FAIL;
	}

	return SUCCESS;
}

CameraErrortype Control::getTiming(FrameGeometry *geometry, FrameTiming *timing)
{
	QVariantMap args;
	QVariantMap map;
	QDBusPendingReply<QVariantMap> reply;

	args.insert("hRes", QVariant(geometry->hRes));
	args.insert("vRes", QVariant(geometry->vRes));
	args.insert("vDarkRows", QVariant(geometry->vDarkRows));
	args.insert("minFrameTime", QVariant(geometry->minFrameTime));

	pthread_mutex_lock(&mutex);
	reply = iface.getResolutionTimingLimits(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - getResolutionTimingLimits: %s - %s\n", err.name().data(), err.message().toAscii().data());
		memset(timing, 0, sizeof(FrameTiming));
		return CAMERA_API_CALL_FAIL;
	}

	map = reply.value();
	if (map.contains("error")) {
		memset(timing, 0, sizeof(FrameTiming));
		return CAMERA_INVALID_SETTINGS;
	}

	timing->exposureMax = map["exposureMax"].toInt();
	timing->minFramePeriod = map["minFramePeriod"].toInt();
	timing->exposureMin = map["exposureMin"].toInt();
	timing->cameraMaxFrames = map["cameraMaxFrames"].toInt();
	return SUCCESS;
}

CameraErrortype Control::getResolution(FrameGeometry *geometry)
{
	QVariant qv = getProperty("resolution");
	if (qv.isValid()) {
		QVariantMap dict;
		qv.value<QDBusArgument>() >> dict;
		parseResolution(geometry, dict);

		qDebug("Got resolution %dx%d offset %dx%d %d-bpp",
			   geometry->hRes, geometry->vRes,
			   geometry->hOffset, geometry->vOffset,
			   geometry->bitDepth);

		return SUCCESS;
	}
	else {
		return CAMERA_API_CALL_FAIL;
	}
}

CameraErrortype Control::setResolution(FrameGeometry *geometry)
{
	QVariantMap resolution;
	resolution.insert("hRes", QVariant(geometry->hRes));
	resolution.insert("vRes", QVariant(geometry->vRes));
	resolution.insert("hOffset", QVariant(geometry->hOffset));
	resolution.insert("vOffset", QVariant(geometry->vOffset));
	resolution.insert("vDarkRows", QVariant(geometry->vDarkRows));
	resolution.insert("bitDepth", QVariant(geometry->bitDepth));
	resolution.insert("minFrameTime", QVariant(geometry->minFrameTime));

	return setProperty("resolution", QVariant(resolution));
}

CameraErrortype Control::getArray(QString parameter, UInt32 size, double *values)
{
	QVariant qv = getProperty(parameter);
	if (qv.isValid()) {
		QDBusArgument array = qv.value<QDBusArgument>();
		int index = 0;

		array.beginArray();
		while (!array.atEnd() && (index < size)) {
			array >> values[index++];
		}
		while (index < size) {
			values[index++] = 0.0;
		}
		array.endArray();

		return SUCCESS;
	}
	else {
		return CAMERA_API_CALL_FAIL;
	}
}

CameraErrortype Control::setArray(QString parameter, UInt32 size, const double *values)
{
	int id = qMetaTypeId<double>();
	QVariant qv;
	QDBusArgument list;

	list.beginArray(id);
	for (int i = 0; i < size; i++) {
		list << values[i];
	}
	list.endArray();

	qv.setValue<QDBusArgument>(list);
	return setProperty(parameter, qv);
}

CameraErrortype Control::getSensorInfo(SensorInfo_t *info)
{
	/* Load all the relevant sensor properties. */
	QStringList names = {
		"sensorName",
		"sensorColorPattern",
		"sensorBitDepth",
		"sensorPixelRate",
		"sensorIso",
		"sensorMaxGain",
		"sensorVMax",
		"sensorVMin",
		"sensorVIncrement",
		"sensorHMax",
		"sensorHMin",
		"sensorHIncrement",
		"sensorVDark"
	};
	QVariantMap response = getPropertyGroup(names);
	if (response.isEmpty()) {
		return CAMERA_API_CALL_FAIL;
	}

	/* Extract the maximum geometry. */
	info->geometry.hRes = response["sensorHMax"].toUInt();
	info->geometry.vRes = response["sensorVMax"].toUInt();
	info->geometry.hOffset = 0;
	info->geometry.vOffset = 0;
	info->geometry.vDarkRows = response["sensorVDark"].toUInt();
	info->geometry.bitDepth = response["sensorBitDepth"].toUInt();
	info->geometry.minFrameTime = 0; /* TODO: ??? */

	/* Extract the other parameters.*/
	info->vIncrement = response["sensorVIncrement"].toUInt();
	info->hIncrement = response["sensorHIncrement"].toUInt();
	info->vMinimum = response["sensorVMin"].toUInt();
	info->hMinimum = response["sensorHMin"].toUInt();
	info->minGain = 1; /* TODO: ??? */
	info->maxGain = response["sensorMaxGain"].toUInt();
	info->iso = response["sensorIso"].toUInt();
	info->cfaPattern = response["sensorColorPattern"].toString();
	info->name = response["sensorName"].toString();

	/* TODO: The timebase isn't exposed through the API, it only deals in nanoseconds. */
	info->timingClock = 1000000000;
	return SUCCESS;
}

void Control::parseResolution(FrameGeometry *geometry, const QVariantMap &map)
{
	geometry->hRes = map.value("hRes", 0).toInt();
	geometry->vRes = map.value("vRes", 0).toInt();
	geometry->hOffset = map.value("hOffset", 0).toInt();
	geometry->vOffset = map.value("vOffset", 0).toInt();
	geometry->vDarkRows = map.value("vDarkRows", 0).toInt();
	geometry->bitDepth = map.value("bitDepth", 0).toInt();
	geometry->minFrameTime = map.value("minFrameTime", 0).toDouble();
}

CameraErrortype Control::getImagerSettings(ImagerSettings_t *is)
{
	/* Load all the relevant sensor properties. */
	QStringList names = {
		"resolution",
		"currentGain",
		"digitalGain",
		"framePeriod",
		"exposurePeriod",
		"recMaxFrames",
		"recPreBurst",
		"recSegments",
		"recMode",
		"disableRingBuffer",
		"recTrigDelay"
	};
	QVariantMap response = getPropertyGroup(names);
	QVariantMap res;
	QString mode;

	if (response.isEmpty()) {
		return CAMERA_API_CALL_FAIL;
	}

	/* Load the resolution configuration */
	response["resolution"].value<QDBusArgument>() >> res;
	parseResolution(&is->geometry, res);

	/* Load the rest of the imager settings. */
	is->gain = floor(response.value("currentGain", 1.0).toDouble());
	is->digitalGain = response.value("digitalGain", 1.0).toDouble();
	is->period = response.value("framePeriod", 1000000).toUInt();
	is->exposure = response.value("exposurePeriod", 0).toUInt();
	is->recRegionSizeFrames = response.value("recMaxFrames", 0).toUInt();
	is->recTrigDelay = response.value("recTrigDelay", 0).toUInt();
	is->prerecordFrames = response.value("recPreBurst", 0).toUInt();
	is->segments = response.value("recSegments", 1).toUInt();
	is->disableRingBuffer = response.value("disableRingBuffer", 0).toBool();

	/* Translate the recording mode */
	mode = response.value("recMode", "normal").toString();
	if (mode == "normal") {
		is->mode = RECORD_MODE_NORMAL;
	} else if (mode == "segmented") {
		is->mode = RECORD_MODE_SEGMENTED;
	} else if (mode == "burst") {
		is->mode = RECORD_MODE_GATED_BURST;
	} else {
		is->mode = RECORD_MODE_NORMAL;
	}

	/* Some stuff that doesn't quite fit... */
	is->segmentLengthFrames = is->recRegionSizeFrames / is->segments;

	return SUCCESS;
}

CameraErrortype Control::startRecording(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.startRecording(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startRecording: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::stopRecording(void)
{
	QDBusPendingReply<QVariantMap> reply;

	qDebug("stopRecording");

	pthread_mutex_lock(&mutex);
	reply = iface.stopRecording();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - stopRecording: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::reboot(bool settings)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap args;

	args.insert("settings", settings);

	pthread_mutex_lock(&mutex);
	reply = iface.reboot(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - reboot: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::startWhiteBalance(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("startWhiteBalance");

	pthread_mutex_lock(&mutex);
	reply = iface.startWhiteBalance(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	return waitAsyncComplete(reply);
}

CameraErrortype Control::asyncCalibration(QStringList calTypes, bool saveCal)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QStringList::const_iterator i;

	for (i = calTypes.constBegin(); i != calTypes.constEnd(); i++) {
		args[*i] = QVariant(true);
	}
	args["saveCal"] = QVariant(saveCal);

	pthread_mutex_lock(&mutex);
	reply = iface.startCalibration(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);
	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startCalibration: %s - %s\n", err.name().data(), err.message().toAscii().data());
		return CAMERA_API_CALL_FAIL;
	}
	return SUCCESS;
}

CameraErrortype Control::startCalibration(QStringList calTypes, bool saveCal)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QStringList::const_iterator i;

	for (i = calTypes.constBegin(); i != calTypes.constEnd(); i++) {
		args[*i] = QVariant(true);
	}
	args["saveCal"] = QVariant(saveCal);

	pthread_mutex_lock(&mutex);
	reply = iface.startCalibration(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	return waitAsyncComplete(reply, 30000);
}

CameraErrortype Control::startCalibration(QString calType, bool saveCal)
{
	return startCalibration(QStringList(calType), saveCal);
}

CameraErrortype Control::exportCalData(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.exportCalData(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - exportCalData: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}

	return waitAsyncComplete(reply, 180000);
}

CameraErrortype Control::importCalData(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.importCalData(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - importCalData: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::availableKeys(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.availableKeys(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - availableKeys: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::availableCalls(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.availableCalls(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - availableCalls: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

Control::Control() : iface("ca.krontech.chronos.control", "/ca/krontech/chronos/control", QDBusConnection::systemBus())
{
    QDBusConnection conn = iface.connection();
	int i;

	// Prepare the recording parameters
	strcpy(filename, "");

	// Set the default file path, or fall back to the MMC card.
	for (i = 1; i <= 3; i++) {
		sprintf(fileDirectory, "/media/sda%d", i);
		if (pathIsMounted(fileDirectory)) {
			break;
		}
	}
	strcpy(fileDirectory, "/media/mmcblk1p1");

    pthread_mutex_init(&mutex, NULL);
	evloop = NULL;

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timer->setSingleShot(true);

	// Connect DBus signals
	conn.connect("ca.krontech.chronos.control", "/ca/krontech/chronos/control", "ca.krontech.chronos.control",
				 "notify", this, SLOT(notify(const QVariantMap&)));
	conn.connect("ca.krontech.chronos.control", "/ca/krontech/chronos/control", "ca.krontech.chronos.control",
				 "complete", this, SLOT(complete(const QVariantMap&)));

    qDebug("####### Control DBUS initialized.");
}

Control::~Control()
{
    pthread_mutex_destroy(&mutex);
}

void Control::notify(const QVariantMap &args)
{
	for (auto e : args.keys()) {
		notifyParam(e, args.value(e));
	}
}

void Control::complete(const QVariantMap &args)
{
	QString err;
	int code;

	if (!args.contains("error")) {
		qDebug() << args["method"].toString() << "completed";
		if (evloop) evloop->exit(SUCCESS);
		return;
	}

	/* Log errors */
	qDebug() << args["method"].toString() << "failed:" << args["error"].toString() + "(" + args.value("message", "").toString() + ")";

	/* Attempt to convert the error into a code. */
	err = args["error"].toString();
	code = CAMERA_API_CALL_FAIL;
	if (err == "SignalClippingError") code = CAMERA_CLIPPED_ERROR;
	else if (err == "LowSignalError") code = CAMERA_LOW_SIGNAL_ERROR;
	if (evloop) evloop->exit(code);
}

void Control::timeout(void)
{
	qDebug() << "API Call timed out";
	if (evloop) {
		evloop->exit(CAMERA_API_CALL_FAIL);
	}
}

CameraErrortype Control::waitAsyncComplete(QDBusPendingReply<QVariantMap> &reply, int timeout)
{
	QVariantMap map;

	/* Check if the D-Bus call failed. */
	if (reply.isError()) {
		QDBusError err = reply.error();
		qDebug("D-Bus failed: %s", err.name().toUtf8().data());
		return CAMERA_API_CALL_FAIL;
	}
	/* Check if there was a synchronos error */
	map = reply.value();
	if (map.contains("error")) {
		qDebug() << "Call failed:" << map["error"].toString();
		return CAMERA_API_CALL_FAIL;
	}
	/* Wait for the asynchronos reply. */
	else {
		QEventLoop evwait;
		int ret;

		timer->start(timeout);

		evloop = &evwait;
		ret = evwait.exec();
		evloop = NULL;

		timer->stop();
		return (CameraErrortype)ret;
	}
}

int Control::mkfilename(char *path, save_mode_type save_mode)
{
	char fname[1000];
	char folderPath[1000];

	if(strlen(fileDirectory) == 0)
		return RECORD_NO_DIRECTORY_SET;

	strcpy(path, fileFolder);
	if(strlen(filename) == 0)
	{
		//Fill timeinfo structure with the current time
		time_t rawtime;
		struct tm * timeinfo;

		time (&rawtime);
		timeinfo = localtime (&rawtime);

		sprintf(fname, "vid_%04d-%02d-%02d_%02d-%02d-%02d",
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
		strcat(path, filename);
	}

	//If the folder does not exist
	strcpy(folderPath, fileDirectory);
	strcat(folderPath, "/");
	strcat(folderPath, fileFolder);
	if(access(folderPath, W_OK) != 0)
	{	//Not existing
		return RECORD_FOLDER_DOES_NOT_EXIST;
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

CameraErrortype Control::saveRecording(UInt32 start, UInt32 length, save_mode_type save_mode, UInt32 framerate, UInt32 maxBitrate)
{
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	char path[1000];

	/* Generate the desired filename, and check that we can write it. */
	int ret = mkfilename(path, save_mode);
	if(ret != SUCCESS) return (CameraErrortype)ret;

	/* Attempt to start the video recording process. */
	map.insert("filename", QVariant(path));
	qDebug() << "==== filename:" << path;
	map.insert("start", QVariant(start));
	map.insert("length", QVariant(length));
	QString device = QString(fileDirectory).split("media/").value(1);
	map.insert("device", QVariant(device));
	switch(save_mode) {
	case SAVE_MODE_H264:
		map.insert("format", QVariant("h264"));
		map.insert("bitrate", QVariant((uint)maxBitrate));
		map.insert("framerate", QVariant((uint)framerate));
		break;
	case SAVE_MODE_RAW16:
		map.insert("format", QVariant("y16"));
		break;
	case SAVE_MODE_RAW12:
		map.insert("format", QVariant("y12b"));
		break;
	case SAVE_MODE_DNG:
		map.insert("format", QVariant("dng"));
		break;
	case SAVE_MODE_TIFF:
		map.insert("format", QVariant("tiff"));
		break;
	case SAVE_MODE_TIFF_RAW:
		map.insert("format", QVariant("tiffraw"));
		break;
	}
	printf("Saving video to %s\r\n", path);

	qDebug() << map;
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

ControlNotify::ControlNotify()
{
	name = QString("");
	hash = NULL;
}

ControlNotify::~ControlNotify()
{
	if (hash) {
		hash->remove(name, this);
	}
}

void Control::listen(QString name, QObject *receiver, const char *method)
{
	ControlNotify *cnotify = new ControlNotify();
	if (cnotify) {
		/* Link the parameter listener into the control class. */
		cnotify->hash = &params;
		cnotify->name = name;
		params.insert(name, cnotify);
		QObject::connect(cnotify, SIGNAL(valueChanged(const QVariant &)), receiver, method);

		/* Setup garbage collection when the receiver is deleted. */
		cnotify->setParent(receiver);
	}
}

void Control::notifyParam(QString name, const QVariant &value)
{
	QHash<QString, ControlNotify *>::iterator iter = params.find(name);

	/* For each listener, emit a valueChanged signal. */
	while ((iter != params.end()) && (iter.key() == name)) {
		emit iter.value()->valueChanged(value);
		iter++;
	}
}
