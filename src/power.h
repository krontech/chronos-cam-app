#ifndef POWER_H
#define POWER_H

#include <QObject>
#include <QLocalSocket>

#include "types.h"

typedef enum AutoPowerModes{
	AUTO_POWER_DISABLED = 0,
	AUTO_POWER_RESTORE_ONLY,
	AUTO_POWER_REMOVE_ONLY,
	AUTO_POWER_BOTH
} AutoPowerModesType;

typedef enum PowerFlags {
	POWER_FLAG_BATT_PRESENT  = 1,
	POWER_FLAG_AC_PRESENT    = 2,
	POWER_FLAG_BATT_CHARGING = 4,
	POWER_FLAG_AUTO_POWERON  = 8,
	POWER_FLAG_OVER_TEMP     = 16,
	POWER_FLAG_SHIPPING_MODE = 32,
	POWER_FLAG_SHUTDOWN_REQ  = 64
} PowerFlagsType;

class Power : public QObject
{
	Q_OBJECT
public:
	explicit Power(QObject *parent = 0);
	~Power();

	void refreshBatteryData();
	void setShippingMode(bool state);
	bool getShippingMode();
	void setAutoPowerMode(int mode);
	int getAutoPowerMode();

	double getBatteryPercent();

	/* Battery data from ENEL4A.c */
	UInt8 battCapacityPercent;
	UInt8 battSOHPercent;
	UInt32 battVoltage;
	UInt32 battCurrent;
	UInt32 battHiResCap;
	UInt32 battHiResSOC;
	UInt32 battVoltageCam;
	Int32 battCurrentCam;
	Int32 mbTemperature;
	UInt8 flags;
	UInt8 fanPWM;

private:
	QLocalSocket socket;
	int powercmp(const char *buf, const char *key);

signals:

public slots:
	void on_socket_readyRead();
};

#endif // POWER_H
