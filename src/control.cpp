
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


//
// NEW API
//


//
// parsers
//

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
	//args.insert("error", QVariant(error));

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
	//args.insert("error", QVariant(error));

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
	}
	map = reply.value();
	//qDebug() << "map:" << map;


	*value = map[parameter].toUInt();

	//qDebug() << "getInt():" << *value;

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

	qDebug() << "getFloat():" << *value;

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
	//qDebug() << map;
	*str = map[parameter].toString();
	//qDebug() << "getString():" << *str;
	//qDebug() << "map[p]:" << map[parameter];

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

	qDebug() << "boolMap" << map;

	qDebug() << "getBool():" << *value;

	return SUCCESS;
}

/*
const QDBusArgument &operator>>(const QDBusArgument &argument, QList &mystruct)
{
	argument.beginStructure();
	argument >> mystruct.count >> mystruct.name;
	argument.endStructure();
	return argument;
}*/

// extract a MyArray array of MyElement elements
void parseArray(QDBusArgument &argument)
{
	argument.beginArray();
	//myarray.clear();

	while ( !argument.atEnd() ) {
		//MyElement element;
		qDebug() << "argument";
		//myarray.append( element );
	}

	argument.endArray();
	//return argument;
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
	qDebug() << jsonString;
	parseJsonArray(parameter, jsonString, size, values);
}

CameraErrortype Control::setArray(QString parameter, UInt32 size, double *values)
{
	QString jsonString;
	buildJsonArray(parameter, &jsonString, size, values);
	setCamJson(jsonString);
	qDebug() << jsonString;
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
	//qDebug() << jsonString;
}



CameraErrortype Control::oldGetArray(QString parameter, bool *value)
{
	QStringList args(parameter);
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map, map2;

	pthread_mutex_lock(&mutex);
	reply = iface.get(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to get array: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();

	//parseArray(map[parameter].convert(QDBusArgument);
	qDebug() << "map.data:" << map[parameter].data();

	//QVariantMap elems = qdbus_cast<QVariantMap>( map.value<QDBusArgument>() );

	QList<QVariant> lst = map[parameter].toList();
	//QJsonObject jobj = map[parameter].toJsonObject();

	qDebug() << "map[parameter].typeName:" << map[parameter].typeName();
	qDebug() << "map[parameter].isValid:" << map[parameter].isValid();

	qDebug() << "map:" << map;
	qDebug() << "map[parameter]:" << map[parameter];

	qDebug() << map[parameter];
	qDebug() << map["wbMatrix"];

	qDebug() << "toMap:" << map[parameter].toMap();

	qDebug() << "lst:" << lst << lst.size();

	qDebug() << "arrayMap" << map;

	return SUCCESS;
}


CameraErrortype Control::oldGetDict(QString parameter)
{
	QStringList args(parameter);
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	QVariantMap map2;
	QVariant qv;

	pthread_mutex_lock(&mutex);
	reply = iface.get(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to get array: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();

	qDebug() << "map:" << map["hRes"];

	qDebug() << map[parameter].toMap();

	//int h = map["horizontal"].toString().toAscii();
	//qDebug() << "getArray():" << *value;
	qv = map[0];
	//qv = map["resolution"];
	//qDebug() << qv.key;
	QVariant someValue = map.value(0);

	qDebug() << map2[0] << someValue;

	return SUCCESS;
}




CameraErrortype Control::startRecording(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("startRecording");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("startAnalogCalibration");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("revertAutoWhiteBalance");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("startZeroTimeBlackCal");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("startCalibration");

	args.insert("zeroTimeBlackCal", 1);
	//args.insert("error", QVariant(error));

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

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("availableKeys");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

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

	qDebug("availableCalls");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.availableCalls(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed - availableCalls: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

















// OLD API:



/*

static CameraData *parseCameraData(const QVariantMap &args, CameraData *cd)
{
    strcpy(cd->apiVersion, args["apiVersion"].toString().toAscii());
    strcpy(cd->model, args["model"].toString().toAscii());
    strcpy(cd->description, args["description"].toString().toAscii());
    strcpy(cd->serial, args["serial"].toString().toAscii());

    return cd;
}






CameraData Control::getCameraData()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    CameraData cData;

    pthread_mutex_lock(&mutex);
    reply = iface.getCameraData();

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        qDebug("##### Error");
		return cData;
    }
    map = reply.value();
    parseCameraData(map, &cData);
    qDebug("##### Success!");

	return cData;
}


static SensorData *parseSensorData(const QVariantMap &args, SensorData *sd)
{
    strcpy(sd->name, args["name"].toString().toAscii());
    strcpy(sd->pixelFormat, args["pixelFormat"].toString().toAscii());
    sd->hMax = args["hMax"].toUInt();
    sd->vMax = args["vMax"].toUInt();
    sd->hIncrement = args["hIncrement"].toUInt();
    sd->vIncrement = args["vIncrement"].toUInt();
    sd->hMin = args["hMin"].toUInt();
    sd->vMin = args["vMin"].toUInt();
    sd->pixelRate = args["pixelRate"].toDouble();

    qDebug("SensorData:");
    qDebug() << " name:" << sd->name;
    qDebug() << " pixelFormat:" << sd->pixelFormat;
    qDebug() << " pixelRate:" << sd->pixelRate;
    qDebug() << " hMax:" << sd->hMax;
    qDebug() << " hMin:" << sd->hMin;
    qDebug() << " hIncrement:" << sd->hIncrement;
    qDebug() << " vMax:" << sd->vMax;
    qDebug() << " vMin:" << sd->vMin;
    qDebug() << " vIncrement:" << sd->vIncrement;

    return sd;
}

CameraErrortype Control::getSensorData()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    SensorData sData;

    pthread_mutex_lock(&mutex);
    reply = iface.getSensorData();

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        //qDebug("##### Error");
        return RECORD_ERROR;
    }
    map = reply.value();
    parseSensorData(map, &sData);
    //qDebug("##### Success!");
    return SUCCESS;
}

static SensorSettings *parseSensorSettings(const QVariantMap &args, SensorSettings *ss)
{
    ss->exposure = args["exposure"].toDouble();
    ss->framePeriod = args["framePeriod"].toDouble();
    ss->frameRate = args["frameRate"].toDouble();
    ss->vDarkRows = args["vDarkRows"].toUInt();
    ss->bitDepth = args["bitDepth"].toUInt();
    ss->vRes = args["vRes"].toUInt();
    ss->hOffset = args["hOffset"].toUInt();
    ss->hRes = args["hRes"].toUInt();
    ss->vOffset = args["vOffset"].toUInt();

    qDebug("SensorSettings:");
    qDebug() << " exposure:" << ss->exposure;
    qDebug() << " framePeriod:" << ss->framePeriod;
    qDebug() << " frameRate:" << ss->frameRate;
    qDebug() << " vDarkRows:" << ss->vDarkRows;
    qDebug() << " bitDepth:" << ss->bitDepth;
    qDebug() << " vRes:" << ss->vRes;
    qDebug() << " hOffset:" << ss->hOffset;
    qDebug() << " hRes:" << ss->hRes;
    qDebug() << " vOffset:" << ss->vOffset;

    return ss;
}

CameraErrortype Control::getSensorSettings()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    qDebug("running RPC: getSensorSettings");

    SensorSettings sSettings;

    pthread_mutex_lock(&mutex);
    reply = iface.getSensorSettings();

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        //qDebug("##### Error");
        return RECORD_ERROR;
    }
    map = reply.value();
    parseSensorSettings(map, &sSettings);
    //qDebug("##### Success!");
    return SUCCESS;
}

static SensorLimits *parseSensorLimits(const QVariantMap &args, SensorLimits *sl)
{
    sl->maxPeriod = args["maxPeriod"].toDouble();
    sl->minPeriod = args["minPeriod"].toDouble();
    qDebug("SensorLimits:");
    qDebug() << " maxPeriod:" << sl->maxPeriod;
    qDebug() << " minPeriod:" << sl->minPeriod;

    return sl;
}

CameraErrortype Control::getSensorLimits()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    qDebug("running RPC: getSensorLimits");

    SensorLimits sLimits;

    pthread_mutex_lock(&mutex);

    //TODO: get this working
	reply = iface.getSensorLimits();

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
		qDebug("##### Error");
        return RECORD_ERROR;
    }
    map = reply.value();
    parseSensorLimits(map, &sLimits);
	qDebug("##### Success!");
    return SUCCESS;
}

static SensorWhiteBalance *parseSensorWhiteBalance(const QVariantMap &args, SensorWhiteBalance *swb)
{
    swb->red = args["red"].toDouble();
    swb->green = args["green"].toDouble();
    swb->blue = args["blue"].toDouble();
    qDebug("SensorWhiteBalance:");
    qDebug() << " red:" << swb->red;
    qDebug() << " green:" << swb->green;
    qDebug() << " blue:" << swb->blue;

    return swb;
}

CameraErrortype Control::getSensorWhiteBalance()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    qDebug("running RPC: getSensorWhiteBalance");

    SensorWhiteBalance sWhiteBalance;

    pthread_mutex_lock(&mutex);
    reply = iface.getWhiteBalance();

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        //qDebug("##### Error");
        return RECORD_ERROR;
    }
    map = reply.value();
    parseSensorWhiteBalance(map, &sWhiteBalance);
    //qDebug("##### Success!");
    return SUCCESS;
}

CameraErrortype Control::setSensorWhiteBalance(double red, double green, double blue)
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

    qDebug("setSensorWhiteBalance");

    args.insert("red", QVariant(red));
    args.insert("green", QVariant(green));
    args.insert("blue", QVariant(blue));

    pthread_mutex_lock(&mutex);
    reply = iface.setWhiteBalance(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to set white balance: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }

	return SUCCESS;

}

CameraErrortype Control::setDescription(const char * description, int idNumber)
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;

    qDebug("setDescription");

    args.insert("description", QVariant(description));
    args.insert("idNumber", QVariant(idNumber));

    pthread_mutex_lock(&mutex);
    reply = iface.setDescription(args);
    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to set description: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }
	return SUCCESS;
}


static SensorWhiteBalance *parseSensorWhiteBalance(const QVariantMap &args, SensorWhiteBalance *swb)
{
	swb->red = args["red"].toDouble();
	swb->green = args["green"].toDouble();
	swb->blue = args["blue"].toDouble();
	qDebug("SensorWhiteBalance:");
	qDebug() << " red:" << swb->red;
	qDebug() << " green:" << swb->green;
	qDebug() << " blue:" << swb->blue;

	return swb;
}


CameraStatus Control::getStatus(const char * lastState, const char * error)
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	CameraStatus cStatus;
	//qDebug("status");

    args.insert("lastState", QVariant(lastState));
    args.insert("error", QVariant(error));

    pthread_mutex_lock(&mutex);
    reply = iface.status(args);
    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to set status: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }
	map = reply.value();
	parseCameraStatus(map, &cStatus);

	return cStatus;
}



CameraErrortype Control::reinitSystem(void)
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;

    qDebug("reinitSystem");

    //args.insert("lastState", QVariant(lastState));
    //args.insert("error", QVariant(error));

    pthread_mutex_lock(&mutex);
    reply = iface.reinitSystem(args);
    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to reinitSystem: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }
}
*/

CameraErrortype Control::setFramePeriod(UInt32 period)
{
	setInt("framePeriod", period);

	/*
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setFramePeriod");

	args.insert("framePeriod", QVariant(period / 100000000.0));

	pthread_mutex_lock(&mutex);
	reply = iface.setSensorSettings(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to setSensorTiming: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	*/

}

/*
CameraErrortype Control::setSensorTiming(UInt32 frameRate)
// we need to have multiple functions for this, to set frame period and/or exposure
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;

	qDebug("setSensorTiming");

    args.insert("frameRate", QVariant(frameRate));

    pthread_mutex_lock(&mutex);
    reply = iface.setSensorTiming(args);
    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to setSensorTiming: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }
}


CameraErrortype Control::setSensorSettings(int hRes, int vRes)
// we need to have multiple functions for this, to set hRes, vRes, hOffset, vOffset, vDarkRows, bitDepth, framePeriod, frameRate, framePeriod, exposure
{
    QVariantMap args;
    QDBusPendingReply<QVariantMap> reply;

    qDebug("reinitSystem");

    args.insert("hRes", QVariant(hRes));
    args.insert("vRes", QVariant(vRes));

    pthread_mutex_lock(&mutex);
    reply = iface.setSensorSettings(args);
    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        QDBusError err = reply.error();
        fprintf(stderr, "Failed to setSensorSettings: %s - %s\n", err.name().data(), err.message().toAscii().data());
    }
}

//CameraErrortype setResolution(FrameGeometry *size)
//{
//
//}


CameraErrortype setResolution(int hRes,
	int vRes,
	int hOffset,
	int vOffset,
	int vDarkRows,
	int bitDepth)
{
	return SUCCESS;
}

CameraErrortype setGain(UInt32 gainSetting)
{
	return SUCCESS;

}

*/

CameraErrortype Control::setIntegrationTime(UInt32 exposure)
{

	setInt("exposurePeriod", exposure);
}

/*

CameraErrortype Control::getIoMapping(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getIoMapping");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getIoMapping(args);
	//reply = iface.calibrate(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getIoMapping: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::setIoMapping(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setIoMapping");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.setIoMapping(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to setIoMapping: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::calibrate(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("calibrate");

	args.insert("analog", QVariant(true));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.calibrate(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to calibrate: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::getColorMatrix(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getColorMatrix");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getColorMatrix();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getColorMatrix: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::setColorMatrix(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setColorMatrix");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.setColorMatrix();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to setColorMatrix: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::startRecord(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("startRecord");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.startRecord(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to startRecord: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::stopRecord(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("stopRecord");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.stopRecord(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to stopRecord: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::getSensorCapabilities(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getSensorCapabilities");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getSensorCapabilities();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getSensorCapabilities: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::dbusGetIoCapabilities(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("dbusGetIoCapabilities");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.dbusGetIoCapabilities();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to dbusGetIoCapabilities: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::getCalCapabilities(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getCalCapabilities");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getCalCapabilities();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getCalCapabilities: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::getSequencerCapabilities(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getSequencerCapabilities");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getSequencerCapabilities();
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getSequencerCapabilities: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::getSequencerProgram(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("getSequencerProgram");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.getSequencerProgram(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to getSequencerProgram: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}

CameraErrortype Control::setSequencerProgram(void)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;

	qDebug("setSequencerProgram");

	//args.insert("lastState", QVariant(lastState));
	//args.insert("error", QVariant(error));

	pthread_mutex_lock(&mutex);
	reply = iface.setSequencerProgram(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to setSequencerProgram: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
}


THIS MARKS THE END OF THE OLD API
*/



/*
void Control::controlTest(const QVariantMap &args)
{
	static CameraStatus st;
   // emit newControlTest(parseControlStatus(args, &st));
}
*/


Control::Control() : iface("ca.krontech.chronos.control", "/ca/krontech/chronos/control", QDBusConnection::systemBus())
{
    QDBusConnection conn = iface.connection();
    //int i;

    pid = -1;
    running = false;

    pthread_mutex_init(&mutex, NULL);
    //pthread_mutex_lock(&mutex);

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
		qDebug() << e << "," << args.value(e);

//int

		if (e == "framePeriod") {
			int period = args.value(e).toInt();
			qDebug() << "framePeriod" << period;
			QString str = "framePeriod";
			emit apiSetFramePeriod(period);
			emit apiSetFramePeriod3(period);
			emit apiSetInt(str, 12345);
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
			//QVariant qv = args.value(e);
			emit apiSetWbMatrix(args.value(e));
			}

//dict
		else if (e == "ioMapping") {
			qDebug() << args.value(e);}
		else if (e == "resolution") {
			emit apiSetResolution(args.value(e)); }

				//emit apiSetWbMatrix(11);
				//QVariantMap map2 = args.value(e);
				//qDebug() << args.value(e)["wbMatrix"];
				//QList<QVariant> root_map = args.value(e).toList(); //  qjo.toVariantMap();
				//qDebug() << root_map;
				//QMap<QString, QVariant> map = args.value(e).toMap();
				//qDebug() << map << map.size();
				//qDebug() << args.value(e).toList();



		else qDebug() << "IGNORED:" << e << "," << args.value(e);


	}

}

