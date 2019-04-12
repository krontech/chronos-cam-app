
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
#include "control.h"
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


static CameraStatus *parseCameraStatus(const QVariantMap &args, CameraStatus *cs)
{
	strcpy(cs->state, args["state"].toString().toAscii());

	return cs;
}


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
*/

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


CameraErrortype Control::setSensorTiming(double frameRate)
/* we need to have multiple functions for this, to set frame period and/or exposure */
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
/* we need to have multiple functions for this, to set hRes, vRes, hOffset, vOffset, vDarkRows, bitDepth, framePeriod, frameRate, framePeriod, exposure */
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



void Control::controlTest(const QVariantMap &args)
{
	static CameraStatus st;
   // emit newControlTest(parseControlStatus(args, &st));
}



Control::Control() : iface("com.krontech.chronos.control", "/com/krontech/chronos/control", QDBusConnection::systemBus())
{
    QDBusConnection conn = iface.connection();
    //int i;

    pid = -1;
    running = false;

    pthread_mutex_init(&mutex, NULL);
    //pthread_mutex_lock(&mutex);

    /* Connect DBus signals */
    conn.connect("com.krontech.chronos.control", "/com/krontech/chronos/control", "com.krontech.chronos.control",
                 "controltest", this, SLOT(segment(const QVariantMap&)));

    /* Try to get the PID of the controller. */
    checkpid();
    qDebug("####### Control DBUS initialized.");
}

Control::~Control()
{
    pthread_mutex_destroy(&mutex);
}

