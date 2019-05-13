
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

	qDebug() << "setInt " << parameter << value;

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


CameraErrortype Control::getInt(QString parameter, UInt32 *value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	//TODO - is this the way to do this?
	args.insert(parameter, 0);

	pthread_mutex_lock(&mutex);
	reply = iface.get(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to get int: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	qDebug() << "map:" << map;


	*value = map[parameter].toUInt();

	qDebug() << "getInt():" << *value;

	return SUCCESS;
}

CameraErrortype Control::getFloat(QString parameter, double *value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	//TODO - is this the way to do this?
	args.insert(parameter, 0);

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
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	//TODO - is this the way to do this?
	args.insert(parameter, true);

	pthread_mutex_lock(&mutex);
	reply = iface.get(args);
	reply.waitForFinished();
	pthread_mutex_unlock(&mutex);

	if (reply.isError()) {
		QDBusError err = reply.error();
		fprintf(stderr, "Failed to get string: %s - %s\n", err.name().data(), err.message().toAscii().data());
	}
	map = reply.value();
	qDebug() << map;
	*str = map[parameter].toString();
	qDebug() << "getString():" << *str;
	qDebug() << "map[p]:" << map[parameter];

	return SUCCESS;
}


CameraErrortype Control::getBool(QString parameter, bool *value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;

	//TODO - is this the way to do this?
	args.insert(parameter, 0);

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


CameraErrortype Control::getResolution(FrameGeometry *geometry)
{
	QString jsonString;
	getCamJson("resolution", &jsonString);
	qDebug() << jsonString;

	parseJsonResolution(jsonString, geometry);

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
	QString jsonString;
	buildJsonIo(&jsonString);
	setCamJson(jsonString);
	//qDebug() << jsonString;
}

CameraErrortype Control::setWbMatrix(void)
{
	QString jsonString;
	buildJsonIo(&jsonString);
	//qDebug() << jsonString;
}



CameraErrortype Control::oldGetArray(QString parameter, bool *value)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map, map2;

	//TODO - is this the way to do this?
	args.insert(parameter, 0);

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

	//qDebug() << "getArray():" << *value;

	//qDebug() << (map[parameter].key().asVariant());


	//map2 = map[parameter];
	//for(QVariantMap::const_iterator iter = map2.begin(); iter != map2.end(); ++iter) {
	//  qDebug() << iter.key() << iter.value();
	//}



	return SUCCESS;
}


CameraErrortype Control::oldGetDict(QString parameter)
{
	QVariantMap args;
	QDBusPendingReply<QVariantMap> reply;
	QVariantMap map;
	QVariantMap map2;
	QVariant qv;

	//TODO - is this the way to do this?
	args.insert(parameter, 0);

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

	qDebug("startBlackCalibration");

	//args.insert("lastState", QVariant(lastState));
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

/*
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




void Control::controlTest(const QVariantMap &args)
{
	static CameraStatus st;
   // emit newControlTest(parseControlStatus(args, &st));
}



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
                 "controltest", this, SLOT(segment(const QVariantMap&)));

    /* Try to get the PID of the controller. */
    checkpid();
    qDebug("####### Control DBUS initialized.");
}

Control::~Control()
{
    pthread_mutex_destroy(&mutex);
}

