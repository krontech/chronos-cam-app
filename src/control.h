#ifndef CONTROL_H
#define CONTROL_H


#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosControlInterface.h"
#include "types.h"
#include "frameGeometry.h"

#include <QObject>

/******************************************************************************/

void* frameThread(void *arg);

/*
typedef enum {
    CONTROL_STATE_1 = 1,
    CONTROL_STATE_2 = 2,
    CONTROL_STATE_3 = 3,
} ControlState2;


struct ControlStatus {
	//ControlState state;
    UInt32 status1;
    UInt32 status2;
    UInt32 status3;
};
*/

typedef enum {
	CONTROL_STATE_LIVEDISPLAY = 0,
	CONTROL_STATE_PLAYBACK = 1,
	CONTROL_STATE_FILESAVE = 2,
} ControlStatus;


struct CameraStatus {
	char state[128];
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

struct SensorSettings {
    double exposure;
    double framePeriod;
    double frameRate;
    UInt32 vDarkRows;
    UInt32 bitDepth;
    UInt32 vRes;
    UInt32 hOffset;
    UInt32 hRes;
    UInt32 vOffset;

};

struct SensorLimits {
    double maxPeriod;
    double minPeriod;
};

struct SensorWhiteBalance {
    double red;
    double green;
    double blue;
};

class Control : public QObject {
    Q_OBJECT

public:
    Control();
    ~Control();

	QVariant getProperty(QString parameter);
	QVariantMap getPropertyGroup(QStringList paramters);
	CameraErrortype setProperty(QString parameter, QVariant value);
	CameraErrortype setPropertyGroup(QVariantMap values);

	CameraErrortype startRecording(void);
	CameraErrortype stopRecording(void);
	CameraErrortype doReset(void);
	CameraErrortype testResolution(void);
	CameraErrortype startAnalogCalibration(void);
	CameraErrortype startAutoWhiteBalance(void);
	CameraErrortype revertAutoWhiteBalance(void);
	QString startCalibration(QString calType);


	CameraErrortype status(CameraStatus *cs);
	CameraErrortype availableKeys(void);
	CameraErrortype availableCalls(void);
	CameraErrortype getInt(QString parameter, UInt32 *value);
	CameraErrortype getString(QString parameter, QString *str);
	CameraErrortype getFloat(QString parameter, double *value);
	CameraErrortype getBool(QString parameter, bool *value);
	CameraErrortype getArray(QString parameter, UInt32 size, double *values);
	CameraErrortype getResolution(FrameGeometry *geometry);

	CameraErrortype setInt(QString parameter, UInt32 value)   { return setProperty(parameter, QVariant(value)); }
	CameraErrortype setString(QString parameter, QString str) { return setProperty(parameter, QVariant(str));   }
	CameraErrortype setFloat(QString parameter, double value) { return setProperty(parameter, QVariant(value)); }
	CameraErrortype setBool(QString parameter, bool value)	  { return setProperty(parameter, QVariant(value)); }
	CameraErrortype setArray(QString parameter, UInt32 size, double *values);

	CameraStatus getStatus(const char * lastState, const char * error);
	CameraErrortype setResolution(FrameGeometry *geometry);

	CameraErrortype getTiming(FrameGeometry *geometry, FrameTiming *timing);

    CameraErrortype calibrate(void);
    CameraErrortype startRecord(void);
    CameraErrortype stopRecord(void);
    CameraErrortype getCalCapabilities(void);
	ControlStatus parseNotification(const QVariantMap &args);





    CameraData cd;



private:
    int pid;
    bool running;
    pthread_mutex_t mutex;

    void checkpid(void);

	CaKrontechChronosControlInterface iface;

signals:
	void apiSetInt(QString param, UInt32 value);
	void apiSetFramePeriod(UInt32 period);
	void apiSetExposurePeriod(UInt32 period);
	void apiSetCurrentIso(UInt32 iso);
	void apiSetCurrentGain(UInt32 gain);
	void apiSetPlaybackPosition(UInt32 frame);
	void apiSetPlaybackStart(UInt32 frame);
	void apiSetPlaybackLength(UInt32 frames);
	void apiSetWbTemperature(UInt32 temp);
	void apiSetRecMaxFrames(UInt32 frames);
	void apiSetRecSegments(UInt32 seg);
	void apiSetRecPreBurst(UInt32 frames);

	void apiSetExposurePercent(double percent);
	void apiSetExposureNormalized(double norm);
	void apiSetIoDelayTime(double IoDelayTime);
	void apiSetFrameRate(double rate);
	void apiSetShutterAngle(double angle);

	void apiSetExposureMode(QString mode);
	void apiSetCameraTallyMode(QString mode);
	void apiSetCameraDescription(QString desc);
	void apiSetNetworkHostname(QString name);

	void apiSetWbMatrix(QVariant wb);
	void apiSetColorMatrix(QVariant wb);
	void apiSetResolution(QVariant wb);

	void notified(ControlStatus state);



    /* D-Bus signal handlers. */
private slots:
	void notify(const QVariantMap &args);

};

#endif // CONTROL_H

