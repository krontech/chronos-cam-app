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

typedef enum CameraRecordModes
{
	RECORD_MODE_NORMAL = 0,
	RECORD_MODE_SEGMENTED,
	RECORD_MODE_GATED_BURST,
	RECORD_MODE_LIVE,
	RECORD_MODE_FPN
} CameraRecordModeType;

typedef struct {
	FrameGeometry geometry;		//Frame geometry.
	UInt32 exposure;            //10ns increments
	UInt32 period;              //Frame period in 10ns increments
	UInt32 gain;
	double digitalGain;
	UInt32 recRegionSizeFrames; //Number of frames in the entire record region
	UInt32 recTrigDelay;        //Number of frames to delay the trigger event by.
	CameraRecordModeType mode;  //Recording mode
	UInt32 segments;            //Number of segments in segmented mode
	UInt32 segmentLengthFrames; //Length of segment in segmented mode
	UInt32 prerecordFrames;     //Number of frames to record before each burst in Gated Burst mode

	unsigned int disableRingBuffer; //Set this to disable the ring buffer (record ends when at the end of memory rather than rolling over to the beginning)
} ImagerSettings_t;

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
	QVariant getProperty(QString parameter, const QVariant & defval = QVariant());
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

	/* Helper functions to get a bundle of settings together */
	CameraErrortype getResolution(FrameGeometry *geometry);
	CameraErrortype getImagerSettings(ImagerSettings_t *is);
	CameraErrortype getSensorInfo(SensorInfo_t *info);

	/* API Methods */
	CameraErrortype startRecording(void);
	CameraErrortype stopRecording(void);
	CameraErrortype reboot(bool settings);
	CameraErrortype testResolution(void);
	CameraErrortype startAutoWhiteBalance(void);
	CameraErrortype revertAutoWhiteBalance(void);
	CameraErrortype startCalibration(QString calType, bool saveCal=false);
	CameraErrortype startCalibration(QStringList calTypes, bool saveCal=false);
	CameraErrortype asyncCalibration(QStringList calTypes, bool saveCal=false);
	CameraErrortype exportCalData(void);
	CameraErrortype importCalData(void);

	CameraErrortype status(CameraStatus *cs);
	CameraErrortype availableKeys(void);
	CameraErrortype availableCalls(void);
	CameraErrortype getArray(QString parameter, UInt32 size, double *values);
	CameraErrortype setArray(QString parameter, UInt32 size, const double *values);

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

	bool exposurePending = false;


private:
    pthread_mutex_t mutex;
	QMultiHash<QString, ControlNotify *> params;
	CaKrontechChronosControlInterface iface;
	QEventLoop *evloop;
	QTimer *timer;

	void notifyParam(QString name, const QVariant &value);
	void parseResolution(FrameGeometry *geometry, const QVariantMap &map);
	CameraErrortype waitAsyncComplete(QDBusPendingReply<QVariantMap> &reply, int timeout = 1000);

private slots:
	/* D-Bus signal handler for parameter changes. */
	void notify(const QVariantMap &args);
	/* D-Bus signal handler for async call completion. */
	void complete(const QVariantMap &args);
	/* Qt timer to cleanup if we lose the async completion. */
	void timeout();
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

