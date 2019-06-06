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
	CameraErrortype setProperty(QString parameter, QVariant value);
	CameraErrortype setPropertyGroup(QVariantMap values);

	CameraErrortype startRecording(void);
	CameraErrortype stopRecording(void);
	CameraErrortype doReset(void);
	CameraErrortype testResolution(void);
	CameraErrortype startAnalogCalibration(void);
	CameraErrortype set(void);
	CameraErrortype startAutoWhiteBalance(void);
	CameraErrortype revertAutoWhiteBalance(void);
	CameraErrortype startZeroTimeBlackCal(void);
	CameraErrortype startBlackCalibration(void);

	CameraErrortype status(CameraStatus *cs);
	CameraErrortype availableKeys(void);
	CameraErrortype availableCalls(void);
	CameraErrortype getInt(QString parameter, UInt32 *value);
	CameraErrortype getString(QString parameter, QString *str);
	CameraErrortype getFloat(QString parameter, double *value);
	CameraErrortype getBool(QString parameter, bool *value);
	CameraErrortype getArray(QString parameter, UInt32 size, double *values);
	CameraErrortype getResolution(FrameGeometry *geometry);
	CameraErrortype getIoSettings(void);
	CameraErrortype setIoSettings(void);

	CameraErrortype oldGetArray(QString parameter, bool *value);
	CameraErrortype oldGetDict(QString parameter);

	CameraErrortype setInt(QString parameter, UInt32 value);
	CameraErrortype setString(QString parameter, QString str);
	CameraErrortype setFloat(QString parameter, double value);
	CameraErrortype setBool(QString parameter, bool value);
	CameraErrortype setArray(QString parameter, UInt32 size, double *values);
	CameraErrortype setWbMatrix(void);






	CameraData getCameraData(void);
	CameraErrortype getSensorData(void);
    CameraErrortype getSensorSettings(void);
    CameraErrortype getSensorLimits(void);
    CameraErrortype setCameraData(void);
    CameraErrortype getSensorWhiteBalance(void);
    CameraErrortype setSensorWhiteBalance(double red, double green, double blue);
	CameraStatus getStatus(const char * lastState, const char * error);
    CameraErrortype setDescription(const char * description, int idNumber);
    CameraErrortype reinitSystem(void);
	//CameraErrortype setSensorTiming(long framePeriod, long exposure);
    CameraErrortype setSensorSettings(int hRes, int vRes);
	/* CameraErrortype setResolution(int hRes,
		int vRes,
		int hOffset,
		int vOffset,
		int vDarkRows,
		int bitDepth); */
	CameraErrortype setResolution(FrameGeometry *geometry);
	CameraErrortype setGain(UInt32 gainSetting);
	CameraErrortype setFramePeriod(UInt32 period);
	CameraErrortype setIntegrationTime(UInt32 exposure);


	CameraErrortype getTiming(FrameGeometry *geometry, FrameTiming *timing);

    CameraErrortype getIoMapping(void);
    CameraErrortype setIoMapping(void);
    CameraErrortype calibrate(void);
    CameraErrortype getColorMatrix(void);
    CameraErrortype setColorMatrix(void);
    CameraErrortype startRecord(void);
    CameraErrortype stopRecord(void);
    CameraErrortype getSensorCapabilities(void);
    CameraErrortype dbusGetIoCapabilities(void);
    CameraErrortype getCalCapabilities(void);
    CameraErrortype getSequencerCapabilities(void);
    CameraErrortype getSequencerProgram(void);
    CameraErrortype setSequencerProgram(void);






    CameraData cd;

signals:
	//void newControlTest(ControlStatus *status);

private:
    int pid;
    bool running;
    pthread_mutex_t mutex;

    void checkpid(void);

	CaKrontechChronosControlInterface iface;


    /* D-Bus signal handlers. */
private slots:
    void controlTest(const QVariantMap &args);
};

#endif // CONTROL_H

