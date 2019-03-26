
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

static ControlState parseControlState(const QVariantMap &args)
{
    if (args["state1"].toBool()) {
        return CONTROL_STATE_3;
    }
    else if (args["state2"].toBool()) {
        return CONTROL_STATE_2;
    }
    else {
        return CONTROL_STATE_3;
    }
}

static ControlStatus *parseControlStatus(const QVariantMap &args, ControlStatus *st)
{
    st->state = parseControlState(args);

    st->status1 = args["status1"].toUInt();
    st->status2 = args["status2"].toUInt();
    st->status3 = args["status3"].toUInt();

    return st;
}

static CameraData *parseCameraData(const QVariantMap &args, CameraData *cd)
{
    strcpy(cd->apiVersion, args["apiVersion"].toString().toAscii());
    strcpy(cd->model, args["model"].toString().toAscii());
    strcpy(cd->description, args["description"].toString().toAscii());
    strcpy(cd->serial, args["serial"].toString().toAscii());

    return cd;
}


CameraErrortype Control::getCameraData()
{
    QDBusPendingReply<QVariantMap> reply;
    QVariantMap map;

    CameraData cData;

    qDebug("##### Trying getCameraData");
    pthread_mutex_lock(&mutex);
    qDebug("##### calling...");
    reply = iface.getCameraData();
    qDebug("##### waiting...");

    reply.waitForFinished();
    pthread_mutex_unlock(&mutex);

    if (reply.isError()) {
        qDebug("##### Error");
        return RECORD_ERROR;
    }
    map = reply.value();
    parseCameraData(map, &cData);
    qDebug("##### Success!");

    return SUCCESS;
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
    /*
    qDebug() << "name:" << sd->name;
    qDebug() << "pixelFormat:" << sd->pixelFormat;
    qDebug() << "pixelRate:" << sd->pixelRate;
    qDebug() << "hMax:" << sd->hMax;
    qDebug() << "hMin:" << sd->hMin;
    qDebug() << "hIncrement:" << sd->hIncrement;
    qDebug() << "vMax:" << sd->vMax;
    qDebug() << "vMin:" << sd->vMin;
    qDebug() << "vIncrement:" << sd->vIncrement;
    */











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

void Control::controlTest(const QVariantMap &args)
{
    static ControlStatus st;
    emit newControlTest(parseControlStatus(args, &st));
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

