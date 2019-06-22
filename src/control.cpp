
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <QDebug>
//#include <qjsonobject.h>
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
#include "exec.h"
#include "camera.h"
#include "util.h"

void catch_sigchild2(int sig) { /* nop */ }

/* Check for the PID of the control daemon. */
void Control::checkpid(void)
{
    FILE *fp = popen("pidof cam-control", "r");
    char line[64];
    if (fp == NULL) {
        return;
    }
    if (fgets(line, sizeof(line), fp) != NULL) {
        pid = strtol(line, NULL, 10);

    }
}

UInt32 sensorHIncrement = 0;
UInt32 sensorVIncrement = 0;
UInt32 sensorHMax = 0;
UInt32 sensorVMax = 0;
UInt32 sensorHMin = 0;
UInt32 sensorVMin = 0;
UInt32 sensorVDark = 0;
UInt32 sensorBitDepth = 0;
double sensorMinFrameTime = 0.001;

static CameraStatus *parseCameraStatus(const QVariantMap &args, CameraStatus *cs)
{
	strcpy(cs->state, args["state"].toString().toAscii());

	return cs;
}

CameraErrortype Control::setInt(QString parameter, UInt32 value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	//qDebug() << "setInt " << parameter << value;

	args.insert(parameter, QVariant(value));

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set int: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}

	return SUCCESS;
}

CameraErrortype Control::setBool(QString parameter, bool value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	qDebug() << "setBool " << parameter << value;

	args.insert(parameter, QVariant(value));

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to set int: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}

	return SUCCESS;
}

CameraErrortype Control::setFloat(QString parameter, double value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setFloat");

	args.insert(parameter, QVariant(value));

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - setFloat: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::setString(QString parameter, QString str)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setString");

	args.insert(parameter, QVariant(str));

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - setString: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

QVariant Control::getProperty(QString parameter)
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
		return QVariant();
	}

	map = reply.value();
	if (map.contains("error")) {
		QVariantMap errdict;
		map["error"].value<QDBusArgument>() >> errdict;

		qDebug() << "Failed to get parameter: " + errdict[parameter].toString();
		return QVariant();
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
		qDebug() << "Failed to get parameter: " + err.name() + " - " + err.message();
		return CAMERA_API_CALL_FAIL;
	}

	map = reply.value();
	if (map.contains("error")) {
		QVariantMap errdict;
		map["error"].value<QDBusArgument>() >> errdict;

		qDebug() << "Failed to get parameter: " + errdict[parameter].toString();
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
			qDebug() << "Failed to get parameter " + i.key() + ": " + i.value().toString();
		}
		return CAMERA_API_CALL_FAIL;
	}

	return SUCCESS;
}

CameraErrortype Control::getInt(QString parameter, UInt32 *value)
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
		fprintf(stderr, "Failed to get int: %s - %s\n", err.name().data(), err.message().toAscii().data());
		return DBUS_CONTROL_FAIL;
	}
	map = reply.value();

	*value = map[parameter].toUInt();

	return SUCCESS;
}

CameraErrortype Control::getFloat(QString parameter, double *value)
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
		fprintf(stderr, "Failed to get float: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	*value = map[parameter].toDouble();

	return SUCCESS;
}

CameraErrortype Control::getString(QString parameter, QString *str)
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
		fprintf(stderr, "Failed to get string: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	*str = map[parameter].toString();

	return SUCCESS;
}


CameraErrortype Control::getBool(QString parameter, bool *value)
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
		fprintf(stderr, "Failed to get bool %s: %s - %s\n", parameter, err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	*value = map[parameter].toBool();

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

	pthread_mutex_lock(&mutex);
	reply = iface.testResolution(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - testResolution: %s - %s\n", err.name().data(), err.message().toAscii().data());
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

		geometry->hRes = dict["hRes"].toInt();
		geometry->vRes = dict["vRes"].toInt();
		geometry->hOffset = dict["hOffset"].toInt();
		geometry->vOffset = dict["vOffset"].toInt();
		geometry->vDarkRows = dict["vDarkRows"].toInt();
		geometry->bitDepth = dict["bitDepth"].toInt();

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

CameraErrortype Control::getIoSettings(void)
{
	QString jsonString;
	getCamJson("ioMapping", &jsonString);
	qDebug() << jsonString;

	parseJsonIoSettings(jsonString);
}

CameraErrortype Control::setResolution(FrameGeometry *geometry)
{
	QString jsonString;

	buildJsonResolution(&jsonString, geometry);
	setCamJson(jsonString);
}

CameraErrortype Control::getArray(QString parameter, UInt32 size, double *values)
{
	QString jsonString;
	getCamJson(parameter, &jsonString);
	parseJsonArray(parameter, jsonString, size, values);
}

CameraErrortype Control::setArray(QString parameter, UInt32 size, double *values)
{
	QString jsonString;
	buildJsonArray(parameter, &jsonString, size, values);
	setCamJson(jsonString);
}

CameraErrortype Control::setIoSettings(void)
{
	QVariantMap ioGroup;
	QVariantMap ioStart;
	QVariantMap io1In;
	QVariantMap io2In;

	ioStart.insert("source", "none");
	ioStart.insert("debounce", QVariant(false));
	ioStart.insert("invert", QVariant(false));

	io1In.insert("threshold", QVariant(pychIo1Threshold));
	io2In.insert("threshold", QVariant(pychIo2Threshold));

	ioGroup.insert("start", QVariant(ioStart));
	ioGroup.insert("io1In", QVariant(io1In));
	ioGroup.insert("io2In", QVariant(io2In));

	return setProperty("ioMapping", ioGroup);
}

CameraErrortype Control::setWbMatrix(void)
{
	QString jsonString;
	buildJsonIo(&jsonString);
}

CameraErrortype Control::startRecording(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("startRecording");

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
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("stopRecording");

	pthread_mutex_lock(&mutex);
	reply = iface.stopRecording(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - stopRecording: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::testResolution(void)
{
	// TODO: implement various different ways to call this
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("testResolution");

	pthread_mutex_lock(&mutex);
	reply = iface.testResolution(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - testResolution: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::doReset(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("doReset");

	pthread_mutex_lock(&mutex);
	reply = iface.doReset(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - doReset: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::startAnalogCalibration(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.startAnalogCalibration(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startAnalogCalibration: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::set(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("set");

	pthread_mutex_lock(&mutex);
	reply = iface.set(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - set: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::startAutoWhiteBalance(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("startAutoWhiteBalance");

	pthread_mutex_lock(&mutex);
	reply = iface.startAutoWhiteBalance(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startAutoWhiteBalance: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::revertAutoWhiteBalance(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.revertAutoWhiteBalance(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - revertAutoWhiteBalance: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::startZeroTimeBlackCal(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	pthread_mutex_lock(&mutex);
	reply = iface.startZeroTimeBlackCal(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startZeroTimeBlackCal: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::startBlackCalibration(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;


	args.insert("zeroTimeBlackCal", 1);

	pthread_mutex_lock(&mutex);
	reply = iface.startBlackCalibration(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - startBlackCalibration: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}



CameraErrortype Control::status(CameraStatus *cs){
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	static CameraStatus st;

	pthread_mutex_lock(&mutex);
	reply = iface.status(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - status: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	parseCameraStatus(map, &st);

	return SUCCESS;


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

CameraErrortype Control::availableCalls(void){
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




CameraErrortype Control::setFramePeriod(UInt32 period)
{
	setInt("framePeriod", period);

}

CameraErrortype Control::setIntegrationTime(UInt32 exposure)
{

	setInt("exposurePeriod", exposure);
}


Control::Control() : iface("ca.krontech.chronos.control", "/ca/krontech/chronos/control", QDBusConnection::systemBus())
{
    QDBusConnection conn = iface.connection();
    //int i;

    pid = -1;
    running = false;

    pthread_mutex_init(&mutex, NULL);

	// Connect DBus signals
	conn.connect("ca.krontech.chronos.control", "/ca/krontech/chronos/control", "ca.krontech.chronos.control",
				 "notify", this, SLOT(notify(const QVariantMap&)));

    /* Try to get the PID of the controller. */
    checkpid();


    qDebug("####### Control DBUS initialized.");
}

Control::~Control()
{
    pthread_mutex_destroy(&mutex);
}


void Control::notify(const QVariantMap &args)
{
	emit notified(parseNotification(args));
}

ControlStatus Control::parseNotification(const QVariantMap &args)
{
	qDebug() << "-------------------------------------";

	for(auto e : args.keys())
	{
		//qDebug() << e << "," << args.value(e);

//int

		if (e == "framePeriod") {
			int period = args.value(e).toInt();
			emit apiSetFramePeriod(period);
		}
		else if (e == "exposurePeriod") {
			int period = args.value(e).toInt();
			emit apiSetExposurePeriod(period); }
		else if (e == "currentIso") {
			int iso = args.value(e).toInt();
			emit apiSetCurrentIso(iso); }
		else if (e == "currentGain") {
			int gain = args.value(e).toInt();
			emit apiSetCurrentGain(gain); }
		else if (e == "playbackPosition") {
			int frame = args.value(e).toInt();
			emit apiSetPlaybackPosition(frame); }
		else if (e == "playbackStart") {
			int frame = args.value(e).toInt();
			emit apiSetPlaybackStart(frame); }
		else if (e == "playbackLength") {
			int frames = args.value(e).toInt();
			emit apiSetPlaybackLength(frames); }
		else if (e == "wbTemperature") {
			int temp = args.value(e).toInt();
			emit apiSetWbTemperature(temp); }
		else if (e == "recMaxFrames") {
			int frames = args.value(e).toInt();
			emit apiSetRecMaxFrames(frames); }
		else if (e == "recSegments") {
			int seg = args.value(e).toInt();
			emit apiSetRecSegments(seg); }
		else if (e == "recPreBurst") {
			int frames = args.value(e).toInt();
			emit apiSetRecPreBurst(frames); }

//float
		else if (e == "exposurePercent") {
			emit apiSetExposurePercent(args.value(e).toDouble()); }
		else if (e == "ExposureNormalized") {
			emit apiSetExposureNormalized(args.value(e).toDouble()); }
		else if (e == "ioDelayTime") {
			emit apiSetIoDelayTime(args.value(e).toDouble()); }
		else if (e == "frameRate") {
			emit apiSetFrameRate(args.value(e).toDouble()); }
		else if (e == "shutterAngle") {
			emit apiSetShutterAngle(args.value(e).toDouble()); }

//string
		else if (e == "exposureMode") {
			emit apiSetExposureMode(args.value(e).toString()); }
		else if (e == "cameraTallyMode") {
			emit apiSetCameraTallyMode(args.value(e).toString()); }
		else if (e == "cameraDescription") {
			emit apiSetCameraDescription(args.value(e).toString()); }
		else if (e == "networkHostname") {
			emit apiSetNetworkHostname(args.value(e).toString()); }

//array
		else if (e == "wbMatrix") {
			emit apiSetWbMatrix(args.value(e));
			}
		else if (e == "wbMatrix") {
			emit apiSetColorMatrix(args.value(e));
			}

//dict
		else if (e == "ioMapping") {
			qDebug() << args.value(e);}
		else if (e == "resolution") {
			emit apiSetResolution(args.value(e)); }

		else qDebug() << "IGNORED:" << e << "," << args.value(e);
	}
}

