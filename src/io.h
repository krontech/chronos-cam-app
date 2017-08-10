#ifndef IO_H
#define IO_H
#include "dm8148PWM.h"
#include "errorCodes.h"
#include "types.h"
#include "gpmc.h"
#endif // IO_H


class IO {
public:
	IO(GPMC * gpmcInst);
	CameraErrortype init();
	bool readIO(UInt32 io);
	void writeIO(UInt32 io, UInt32 val);

	void setThreshold(UInt32 io, double thresholdVolts);
	double getThreshold(UInt32 io);
	void setTriggerEnable(UInt32 source);
	void setTriggerInvert(UInt32 invert);
	void setTriggerDebounceEn(UInt32 dbEn);
	void setTriggerDelayFrames(UInt32 delayFrames);
	UInt32 getTriggerEnable();
	UInt32 getTriggerInvert();
	UInt32 getTriggerDebounceEn();
	UInt32 getTriggerDelayFrames();
	void setOutLevel(UInt32 level);
	void setOutSource(UInt32 source);
	void setOutInvert(UInt32 invert);
	UInt32 getOutLevel();
	UInt32 getOutSource();
	UInt32 getOutInvert();
	UInt32 getIn();

	bool getTriggeredExpEnable();
	UInt32 getExtShutterSrcEn();
	bool getShutterGatingEnable();
	void setTriggeredExpEnable(bool en);
	void setExtShutterSrcEn(UInt32 extShutterSrcEn);
	void setShutterGatingEnable(bool en);

private:
	dm8148PWM io1DAC, io2DAC;
	GPMC * gpmc;
	UInt32 IO2InFD;
	double io1Thresh, io2Thresh;
};
/*
enum {
	IO_SUCCESS = 0,
	IO_ERROR_OPEN,
	IO_FILE_ERROR
};
*/



