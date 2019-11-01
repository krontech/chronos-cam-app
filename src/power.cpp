
#include <QSettings>
#include "power.h"

Power::Power(QObject *parent) : QObject(parent)
{
	/* Connect to pcUtil UNIX socket to get power/battery data */
	socket.connectToServer("/tmp/pcUtil.socket");
	if(socket.waitForConnected(500)){
		qDebug("connected to pcUtil server");
	} else {
		qDebug("could not connect to pcUtil socket");
	}

	/* Clear the battery data. */
	battCapacityPercent = 0;
	battSOHPercent = 0;
	battVoltage = 0;
	battCurrent = 0;
	battHiResCap = 0;
	battHiResSOC = 0;
	battVoltageCam = 0;
	battCurrentCam = 0;
	mbTemperature = 0;
	flags = 0;
	fanPWM = 0;

	connect(&socket, SIGNAL(readyRead()), this, SLOT(on_socket_readyRead()));

	/* Disable Shipping Mode and refresh battery data on boot. */
	setShippingMode(FALSE);
	refreshBatteryData();
}

Power::~Power()
{
	socket.close();
}

void Power::refreshBatteryData()
{
	socket.write("GET_BATTERY_DATA");
}

void Power::setShippingMode(bool state)
{
	if (state) {
		socket.write("SET_SHIPPING_MODE_ENABLED");
	}
	else {
		socket.write("SET_SHIPPING_MODE_DISABLED");
	}
}

bool Power::getShippingMode()
{
	QSettings appSettings;
	return appSettings.value("camera/shippingMode", false).toBool();
}

void Power::setAutoPowerMode(int mode)
{
	QSettings appSettings;

	switch (mode) {
	default:
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_DISABLED);
		/* Fall-through */
	case AUTO_POWER_DISABLED:
		socket.write("SET_POWERUP_MODE_0");
		break;

	case AUTO_POWER_RESTORE_ONLY: //Turn the camera on when the AC adapter is plugged in
		socket.write("SET_POWERUP_MODE_1");
		break;

	case AUTO_POWER_REMOVE_ONLY: //Turn the camera off when the AC adapter is removed
		socket.write("SET_POWERUP_MODE_2");
		break;

	case AUTO_POWER_BOTH:
		socket.write("SET_POWERUP_MODE_3");
		break;
	}
}

int Power::getAutoPowerMode()
{
	QSettings appSettings;
	return appSettings.value("camera/autoPowerMode", 0).toInt();
}

/* Just like strcmp, but it stops comparing at the end of 'key' */
int Power::powercmp(const char *buf, const char *key)
{
	return memcmp(buf, key, strlen(key));
}

void Power::on_socket_readyRead()
{
	QSettings appSettings;
	char buf[256];
	qint64 len;

	/* Read data from the socket. */
	len = socket.read(buf, sizeof(buf));
	if (len <= 0) {
		return;
	}
	if (len >= sizeof(buf)) len = sizeof(buf)-1;
	buf[len] = '\0';

	/* Handle the data that was read. */
	if(powercmp(buf, "pwrmode0") == 0){
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_DISABLED);
	}
	else if (powercmp(buf, "pwrmode1") == 0) {
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_RESTORE_ONLY);
	}
	else if (powercmp(buf,"pwrmode2") == 0){
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_REMOVE_ONLY);
	}
	else if (powercmp(buf,"pwrmode3") == 0){
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_BOTH);
	}
	else if (powercmp(buf,"shipping mode enabled") == 0) {
		appSettings.setValue("camera/shippingMode", TRUE);
	}
	else if (powercmp(buf,"shipping mode disabled") == 0) {
		appSettings.setValue("camera/shippingMode", FALSE);
	}
	/* Otherwise, it might be battery data. */
	else {
		sscanf(buf, "battCapacityPercent %hhu\nbattSOHPercent %hhu\nbattVoltage %u\nbattCurrent %u\nbattHiResCap %u\nbattHiResSOC %u\n"
			   "battVoltageCam %u\nbattCurrentCam %d\nmbTemperature %d\nflags %hhu\nfanPWM %hhu",
			   &battCapacityPercent,
			   &battSOHPercent,
			   &battVoltage,
			   &battCurrent,
			   &battHiResCap,
			   &battHiResSOC,
			   &battVoltageCam,
			   &battCurrentCam,
			   &mbTemperature,
			   &flags,
			   &fanPWM);
	}
}

double Power::getBatteryPercent()
{
	if (flags & POWER_FLAG_BATT_CHARGING) {
		/* Charging */
		return within(((double)battVoltageCam/1000.0 - 10.75) / (12.4 - 10.75) * 80, 0.0, 80.0) +
				20 - 20*within(((double)battCurrentCam/1000.0 - 0.1) / (1.28 - 0.1), 0.0, 1.0);
	}
	else {
		/* Discharging */
		return within(((double)battVoltageCam/1000.0 - 9.75) / (11.8 - 9.75) * 100, 0.0, 100.0);
	}
}
