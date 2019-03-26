#ifndef CONTROL_H
#define CONTROL_H


#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosControlInterface.h"
#include "types.h"

#include <QObject>

/******************************************************************************/

void* frameThread(void *arg);

typedef enum {
    CONTROL_STATE_1 = 1,
    CONTROL_STATE_2 = 2,
    CONTROL_STATE_3 = 3,
} ControlState;


struct ControlStatus {
    ControlState state;
    UInt32 status1;
    UInt32 status2;
    UInt32 status3;
};

struct CameraData {
    char apiVersion[128];
    char model[128];
    char description[128];
    char serial[128];
};

struct SensorData {
    char name[128];
    char pixelFormat[128];
    UInt32 hMax;
    UInt32 hMin;
    UInt32 hIncrement;
    UInt32 vMax;
    UInt32 vMin;
    UInt32 vIncrement;
    double pixelRate;
};

class Control : public QObject {
    Q_OBJECT

public:
    Control();
    ~Control();

    CameraErrortype getCameraData(void);
    CameraErrortype getSensorData(void);
    CameraErrortype setCameraData(void);
    CameraData cd;

signals:
    void newControlTest(ControlStatus *status);

private:
    int pid;
    bool running;
    pthread_mutex_t mutex;

    void checkpid(void);

    ComKrontechChronosControlInterface iface;


    /* D-Bus signal handlers. */
private slots:
    void controlTest(const QVariantMap &args);
};

#endif // CONTROL_H

