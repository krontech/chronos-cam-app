#ifndef CONTROL_H
#define CONTROL_H


#include <pthread.h>
#include <poll.h>
#include "errorCodes.h"
#include "chronosControlInterface.h"
#include "types.h"
#include "video.h"
#include "frameGeometry.h"

#include <QObject>

/******************************************************************************/

#define FOCUS_PEAK_THRESH_LOW	35
#define FOCUS_PEAK_THRESH_MED	25
#define FOCUS_PEAK_THRESH_HIGH	15

#define FOCUS_PEAK_API_LOW	0.33
#define FOCUS_PEAK_API_MED	0.67
#define FOCUS_PEAK_API_HIGH	1.0

struct CameraStatus {
	char state[128];
};

typedef struct {
	FrameGeometry geometry;
	UInt32 vIncrement;
	UInt32 hIncrement;
	UInt32 vMinimum;
	UInt32 hMinimum;
	UInt32 minGain;
	UInt32 maxGain;
	UInt32 iso;
	QString cfaPattern;
	QString name;
	UInt32 timingClock;
} SensorInfo_t;

struct SensorWhiteBalance {
    double red;
    double green;
    double blue;
};

class ControlNotify;

class Control : public QObject {
    Q_OBJECT

	friend class ControlParam;

public:
	/* Settings moved over from the VideoRecord class, into Video class, and then into Control */
	char filename[1000];
	char fileDirectory[1000];
	char fileFolder[1000];

    Control();
    ~Control();

	/* Get and set functions */
	QVariant getProperty(QString parameter);
	QVariantMap getPropertyGroup(QStringList paramters);
	CameraErrortype setProperty(QString parameter, QVariant value);
	CameraErrortype setPropertyGroup(QVariantMap values);

	/* Wrapper functions to get a property as a type. */
	template<typename T> CameraErrortype getPropertyValue(QString parameter, T *value);
	CameraErrortype getInt(QString parameter, UInt32 *value)	{ return getPropertyValue<UInt32>(parameter, value); }
	CameraErrortype getString(QString parameter, QString *str)	{ return getPropertyValue<QString>(parameter, str);  }
	CameraErrortype getFloat(QString parameter, double *value)	{ return getPropertyValue<double>(parameter, value); }
	CameraErrortype getBool(QString parameter, bool *value)		{ return getPropertyValue<bool>(parameter, value);   }

	/* Wrapper functions to set a property from a type. */
	CameraErrortype setInt(QString parameter, UInt32 value)   { return setProperty(parameter, QVariant(value)); }
	CameraErrortype setString(QString parameter, QString str) { return setProperty(parameter, QVariant(str));   }
	CameraErrortype setFloat(QString parameter, double value) { return setProperty(parameter, QVariant(value)); }
	CameraErrortype setBool(QString parameter, bool value)	  { return setProperty(parameter, QVariant(value)); }

	/* Helper function to get all the sensor constants */
	CameraErrortype getSensorInfo(SensorInfo_t *info);

	/* API Methods */
	CameraErrortype startRecording(void);
	CameraErrortype stopRecording(void);
	CameraErrortype doReset(void);
	CameraErrortype testResolution(void);
	CameraErrortype startAutoWhiteBalance(void);
	CameraErrortype revertAutoWhiteBalance(void);
	CameraErrortype startCalibration(QString calType, bool saveCal=false);
	CameraErrortype startCalibration(QStringList calTypes, bool saveCal=false);

	CameraErrortype status(CameraStatus *cs);
	CameraErrortype availableKeys(void);
	CameraErrortype availableCalls(void);
	CameraErrortype getResolution(FrameGeometry *geometry);
	CameraErrortype getArray(QString parameter, UInt32 size, double *values);
	CameraErrortype setArray(QString parameter, UInt32 size, double *values);

	CameraStatus getStatus(const char * lastState, const char * error);
	CameraErrortype setResolution(FrameGeometry *geometry);

	CameraErrortype getTiming(FrameGeometry *geometry, FrameTiming *timing);

    CameraErrortype calibrate(void);
    CameraErrortype startRecord(void);
    CameraErrortype stopRecord(void);
    CameraErrortype getCalCapabilities(void);
	int mkfilename(char *path, save_mode_type save_mode);
	CameraErrortype saveRecording(UInt32 start, UInt32 length, save_mode_type save_mode, UInt32 framerate, UInt32 maxBitrate);

	void listen(QString name, QObject *receiver, const char *method);

private:
    pthread_mutex_t mutex;
	QMultiHash<QString, ControlNotify *> params;
	CaKrontechChronosControlInterface iface;

	void notifyParam(QString name, const QVariant &value);

private slots:
	/* D-Bus signal handler for parameter changes. */
	void notify(const QVariantMap &args);

};

/* Template wrapper to get a property and convert it to a type. */
template <typename T>
CameraErrortype Control::getPropertyValue(QString parameter, T *value)
{
	QVariant qv = getProperty(parameter);
	if (!qv.isValid()) {
		return CAMERA_API_CALL_FAIL;
	}
	if (!qv.canConvert<T>()) {
		qDebug() << "Failed to convert parameter " + parameter + " from type " + qv.typeName();
		return CAMERA_API_CALL_FAIL;
	}
	*value = qv.value<T>();
	return SUCCESS;
}

/* Helper class to emit property changed signals */
class ControlNotify : public QObject {
	Q_OBJECT

	friend class Control;

public:
	ControlNotify();
	~ControlNotify();

private:
	QMultiHash<QString, ControlNotify *> *hash;
	QString name;

signals:
	void valueChanged(const QVariant &value);
};

#endif // CONTROL_H

