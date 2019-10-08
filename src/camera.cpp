#define IMAGERSETTINGS_DBUS //eliminates QSettings for recording parameters
//#define DEBUG_DBUS //eliminates recurring Dbus calls
/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <QDebug>
#include <semaphore.h>
#include <QSettings>
#include <QResource>
#include <QDir>
#include <QIODevice>
#include <QApplication>

#include "font.h"
#include "camera.h"
#include "cameraRegisters.h"
#include "util.h"
#include "types.h"
#include "defines.h"
#include <QWSDisplay>
#include "control.h"
#include "exec.h"
#include "camera.h"
#include "pysensor.h"
#include "cammainwindow.h"


void* recDataThread(void *arg);
void recordEosCallback(void * arg);
void recordErrorCallback(void * arg, const char * message);

#ifdef DEBUG_DBUS
bool debugDbus = true;
#else
bool debugDbus = false;
#endif

#ifdef IMAGERSETTINGS_DBUS
bool isDbus = true;
#else
bool isDbus = false;
#endif


Camera::Camera()
{
	QSettings appSettings;

	terminateRecDataThread = false;
	lastRecording = false;
	playbackMode = false;
	recording = false;
	imgGain = 1.0;
	recordingData.hasBeenSaved = true;		//Nothing in RAM at power up so there's nothing to lose
	unsavedWarnEnabled = getUnsavedWarnEnable();
	autoSave = appSettings.value("camera/autoSave", 0).toBool();
	autoRecord = appSettings.value("camera/autoRecord", 0).toBool();
	ButtonsOnLeft = getButtonsOnLeft();
	UpsideDownDisplay = getUpsideDownDisplay();
	strcpy(serialNumber, "Not_Set");

}

Camera::~Camera()
{
	terminateRecDataThread = true;
	pthread_join(recDataThreadID, NULL);
}

CameraErrortype Camera::init(Video * vinstInst, Control * cinstInst, PySensor * sensorInst, UserInterface * userInterface, UInt32 ramSizeVal, bool color)
{
	CameraErrortype retVal;
	UInt32 ramSizeGBSlot0, ramSizeGBSlot1;
	QSettings appSettings;

	CameraStatus cs;
	CameraData cd;
	VideoStatus st;

	vinst = vinstInst;
	cinst = cinstInst;
	sensor = sensorInst;
	ui = userInterface;


	//Get the memory size
	retVal = (CameraErrortype)getRamSizeGB(&ramSizeGBSlot0, &ramSizeGBSlot1);

	if(retVal != SUCCESS)
		return retVal;


	//Read serial number in
	retVal = (CameraErrortype)readSerialNumber(serialNumber);
	if(retVal != SUCCESS)
		return retVal;

	ramSize = (ramSizeGBSlot0 + ramSizeGBSlot1)*1024/32*1024*1024;
	isColor = true;//readIsColor();
	int err;



	retVal = cinst->status(&cs);
	bool recording = !strcmp(cs.state, "idle");

	//taken from below:
	printf("Starting rec data thread\n");
	terminateRecDataThread = false;

	err = pthread_create(&recDataThreadID, NULL, &recDataThread, this);

	/* Load default recording from sensor limits. */
	imagerSettings.geometry = sensor->getMaxGeometry();
	imagerSettings.geometry.vDarkRows = 0;
	imagerSettings.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&imagerSettings.geometry);
	imagerSettings.period = sensor->getMinFramePeriod(&imagerSettings.geometry);
	imagerSettings.exposure = sensor->getMaxIntegrationTime(imagerSettings.period, &imagerSettings.geometry);
	imagerSettings.disableRingBuffer = 0;
	imagerSettings.mode = RECORD_MODE_NORMAL;
	imagerSettings.prerecordFrames = 1;
	imagerSettings.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
	imagerSettings.segments = 1;

	ImagerSettings_t settings;
	if (isDbus)
	{
		cinst->getResolution(&settings.geometry);
		settings.geometry.minFrameTime	= 0.001;
		cinst->getInt("currentGain", &settings.gain);
		cinst->getInt("framePeriod", &settings.period);
		cinst->getInt("exposurePeriod", &settings.exposure);   // WAS:   = sensor->getMaxIntegrationTime(settings.period, &settings.geometry); //PYCHRONOS - this is needed but suboptimal
		cinst->getInt("recMaxFrames", &settings.recRegionSizeFrames);
		settings.disableRingBuffer      = appSettings.value("camera/disableRingBuffer", 0).toInt();
		QString mode;
		cinst->getString("recMode", &mode);
		if (mode == "normal")
			{ settings.mode = RECORD_MODE_NORMAL; }
		else if (mode == "segmented")
			{ settings.mode = RECORD_MODE_SEGMENTED; }
		else if (mode == "burst")
			{ settings.mode = RECORD_MODE_GATED_BURST; }
		else
			{ settings.mode = RECORD_MODE_NORMAL; }	// default
		cinst->getInt("recPreBurst", &settings.prerecordFrames);
		cinst->getInt("recSegments", &settings.segments);
		UInt32 maxFrames;
		cinst->getInt("cameraMaxFrames", &maxFrames);
		settings.segmentLengthFrames    = maxFrames / settings.segments;
		settings.temporary              = 0;
	}
	else
	{
		settings.geometry.hRes          = appSettings.value("camera/hRes", imagerSettings.geometry.hRes).toInt();
		settings.geometry.vRes          = appSettings.value("camera/vRes", imagerSettings.geometry.vRes).toInt();
		settings.geometry.hOffset       = appSettings.value("camera/hOffset", 0).toInt();
		settings.geometry.vOffset       = appSettings.value("camera/vOffset", 0).toInt();
		settings.geometry.vDarkRows     = 0;
		settings.geometry.bitDepth		= imagerSettings.geometry.bitDepth;
		settings.geometry.minFrameTime	= 0.001;
		settings.gain                   = appSettings.value("camera/gain", 0).toInt();
		settings.period                 = appSettings.value("camera/period", sensor->getMinFramePeriod(&settings.geometry)).toInt();
		settings.exposure               = sensor->getMaxIntegrationTime(settings.period, &settings.geometry); //PYCHRONOS - this is needed but suboptimal
		settings.recRegionSizeFrames    = appSettings.value("camera/recRegionSizeFrames", getMaxRecordRegionSizeFrames(&settings.geometry)).toInt();
		settings.disableRingBuffer      = appSettings.value("camera/disableRingBuffer", 0).toInt();
		settings.mode                   = (CameraRecordModeType)appSettings.value("camera/mode", RECORD_MODE_NORMAL).toInt();
		settings.prerecordFrames        = appSettings.value("camera/prerecordFrames", 1).toInt();
		settings.segmentLengthFrames    = appSettings.value("camera/segmentLengthFrames", settings.recRegionSizeFrames).toInt();
		settings.segments               = appSettings.value("camera/segments", 1).toInt();
		settings.temporary              = 0;
	}

	setImagerSettings(settings);


	vinst->bitsPerPixel        = appSettings.value("recorder/bitsPerPixel", 0.7).toDouble();
	vinst->maxBitrate          = appSettings.value("recorder/maxBitrate", 40.0).toDouble();
	vinst->framerate           = appSettings.value("recorder/framerate", 60).toUInt();
	strcpy(cinst->filename,      appSettings.value("recorder/filename", "").toString().toAscii());
	strcpy(cinst->fileDirectory, appSettings.value("recorder/fileDirectory", "").toString().toAscii());
	strcpy(cinst->fileFolder,    appSettings.value("recorder/fileFolder", "").toString().toAscii());
	if(strlen(cinst->fileDirectory) == 0){
		/* Set the default file path, or fall back to the MMC card. */
		int i;
		bool fileDirFoundOnUSB = false;
		for (i = 1; i <= 3; i++) {
			sprintf(cinst->fileDirectory, "/media/sda%d", i);
			if (path_is_mounted(cinst->fileDirectory)) {
				fileDirFoundOnUSB = true;
				break;
			}
		}
		if(!fileDirFoundOnUSB) strcpy(cinst->fileDirectory, "/media/mmcblk1p1");
	}


	ui->setRecLEDFront(false);
	ui->setRecLEDBack(false);

	//vinst->setDisplayOptions(getZebraEnable(), getFocusPeakEnable());
	vinst->setDisplayPosition(ButtonsOnLeft ^ UpsideDownDisplay);
	usleep(2000000); //needed temporarily with current pychronos
	cinst->doReset(); //also needed temporarily

	printf("Video init done\n");
	int fpColor = getFocusPeakColor();
	if (fpColor == 0)
	{
		setFocusPeakColor(2);	//set to cyan, if starts out set to black
	}
	return SUCCESS;
}


UInt32 Camera::loadImagerSettings(ImagerSettings_t *settings)
{
	//retrieve imagersettings through API
	cinst->getInt("exposurePeriod",&settings->exposure);
	cinst->getInt("framePeriod",&settings->period);
	double gainFloat;
	cinst->getFloat("currentGain", &gainFloat);
	UInt32 gain = (int)gainFloat;
	UInt32 gainIndex = 0;
	for (int i=gain; i > 1; i /= 2) gainIndex++;
	settings->gain = gainIndex;
	return SUCCESS;
}


UInt32 Camera::setImagerSettings(ImagerSettings_t settings)
{
	QString modes[] = {"normal", "segmented", "burst"};
	QSettings appSettings;
	QVariantMap values;
	QVariantMap resolution;

	if(!sensor->isValidResolution(&settings.geometry) ||
		settings.recRegionSizeFrames < RECORD_LENGTH_MIN ||
		settings.segments > settings.recRegionSizeFrames) {
		return CAMERA_INVALID_IMAGER_SETTINGS;
	}

	QString str;

	resolution.insert("hRes", QVariant(settings.geometry.hRes));
	resolution.insert("vRes", QVariant(settings.geometry.vRes));
	resolution.insert("hOffset", QVariant(settings.geometry.hOffset));
	resolution.insert("vOffset", QVariant(settings.geometry.vOffset));
	resolution.insert("vDarkRows", QVariant(settings.geometry.vDarkRows));
	resolution.insert("bitDepth", QVariant(settings.geometry.bitDepth));
	resolution.insert("minFrameTime", QVariant(settings.geometry.minFrameTime));

	values.insert("resolution", QVariant(resolution));
	values.insert("framePeriod", QVariant(settings.period));
	values.insert("currentGain", QVariant(1 << settings.gain));
	values.insert("exposurePeriod", QVariant(settings.exposure));
	if (settings.mode > 3 ) qFatal("imagerSetting mode is FPN");
	else values.insert("recMode", modes[imagerSettings.mode]);
	values.insert("recSegments", QVariant(settings.segments));
	values.insert("recMaxFrames", QVariant(settings.recRegionSizeFrames));

	CameraErrortype retVal = cinst->setPropertyGroup(values);
	memcpy(&imagerSettings, &settings, sizeof(settings));

	return retVal;
}

UInt32 Camera::getRecordLengthFrames(ImagerSettings_t settings)
{
	if ((settings.mode == RECORD_MODE_NORMAL) || (settings.mode == RECORD_MODE_GATED_BURST)) {
		return settings.recRegionSizeFrames;
	}
	else {
		return (settings.recRegionSizeFrames / settings.segments);
	}
}

UInt32 Camera::getFrameSizeWords(FrameGeometry *geometry)
{
	return ROUND_UP_MULT((geometry->size() + BYTES_PER_WORD - 1) / BYTES_PER_WORD, FRAME_ALIGN_WORDS);
}

UInt32 Camera::getMaxRecordRegionSizeFrames(FrameGeometry *geometry)
{
	return (ramSize - REC_REGION_START) / getFrameSizeWords(geometry);
}

void Camera::updateTriggerValues(ImagerSettings_t settings){
	UInt32 recLengthFrames = getRecordLengthFrames(settings);
	if(getTriggerDelayConstant() == TRIGGERDELAY_TIME_RATIO){
		triggerPostFrames  = triggerTimeRatio * recLengthFrames;
		triggerPostSeconds = triggerPostFrames * ((double)settings.period / 100000000);
	}
	if(getTriggerDelayConstant() == TRIGGERDELAY_SECONDS){
		triggerTimeRatio  = recLengthFrames / ((double)settings.period / 100000000);
		triggerPostFrames = triggerPostSeconds / ((double)settings.period / 100000000);
	}
	if(getTriggerDelayConstant() == TRIGGERDELAY_FRAMES){
		triggerTimeRatio   = (double)triggerPostFrames / recLengthFrames;
		triggerPostSeconds = triggerPostFrames * ((double)settings.period / 100000000);
	}
}

unsigned short Camera::getTriggerDelayConstant(){
	 QSettings appSettings;
	//return appSettings.value("camera/triggerDelayConstant", TRIGGERDELAY_PRERECORDSECONDS).toUInt();
	return TRIGGERDELAY_TIME_RATIO;//With comboBox removed, always use this choice instead.
}

void Camera::setTriggerDelayConstant(unsigned short value){
	 QSettings appSettings;
	 appSettings.setValue("camera/triggerDelayConstant", value);
}

void Camera::setTriggerDelayValues(double ratio, double seconds, UInt32 frames){
	triggerTimeRatio = ratio;
	 triggerPostSeconds = seconds;
	 triggerPostFrames = frames;
}

UInt32 Camera::setIntegrationTime(double intTime, FrameGeometry *fSize, Int32 flags)
{
	QSettings appSettings;
	UInt32 validTime;
	UInt32 defaultTime = sensor->getMaxIntegrationTime(sensor->getFramePeriod(), fSize);
	if (flags & SETTING_FLAG_USESAVED) {
		validTime = appSettings.value("camera/exposure", defaultTime).toInt();
		qDebug("--- Using old settings --- Exposure time: %d (default: %d)", validTime, defaultTime);
		cinst->setInt("exposurePeriod", validTime * 1e9);
	}
	else {
		cinst->setInt("exposurePeriod", intTime * 1e9);
		validTime = intTime; // * 1e9;
	}

	if (!(flags & SETTING_FLAG_TEMPORARY)) {
		//qDebug("--- Saving settings --- Exposure time: %d", validTime);
		appSettings.setValue("camera/exposure", validTime);
		imagerSettings.exposure = validTime;
	}
	return SUCCESS;
}

void Camera::updateVideoPosition()
{
	vinst->setDisplayPosition(ButtonsOnLeft ^ UpsideDownDisplay);
}


void Camera::getSensorInfo(Control *c)
{
	c->getInt("sensorHIncrement", &sensorHIncrement);
	c->getInt("sensorVIncrement", &sensorVIncrement);
	c->getInt("sensorHMax", &sensorHMax);
	c->getInt("sensorVMax", &sensorVMax);
	c->getInt("sensorHMin", &sensorHMin);
	c->getInt("sensorVMin", &sensorVMin);
	c->getInt("sensorVDark", &sensorVDark);
	c->getInt("sensorBitDepth", &sensorBitDepth);

}

Int32 Camera::startRecording(void)
{
	qDebug("===== Camera::startRecording()");

	QString str;

	FrameGeometry geometry;
	geometry.hRes          = 1280;
	geometry.vRes          = 1024;
	geometry.hOffset       = 0;
	geometry.vOffset       = 0;
	geometry.vDarkRows     = 0;
	geometry.bitDepth	   = 12;
	geometry.minFrameTime  = 0.0002; //arbitrary here!

	cinst->stopRecording();

	// For testing individual API calls using the record button:

	//double wbTest[3];
	//double cmTest[9];
	//double testArray[9] = {1,0,0,0,1,0,0,0,1};
	//CameraStatus cs;
	//UInt32 i;
	//double d;
	//bool b;

	//UInt32 sizeGB;
	//cinst->getInt("cameraMemoryGB", &sizeGB);
	//cinst->getString("cameraDescription", &str);
	//cinst->getString("state", &str);
	//cinst->setResolution(&geometry);

	//cinst->setArray("wbMatrix", 3, (double *)&testArray);
	//cinst->setArray("colorMatrix", 9, (double *)&testArray);


	//cinst->getResolution(&geometry);
	//cinst->getString("cameraDescription", &str);
	//cinst->getArray("wbMatrix", 3, (double *)&wbTest);
	//cinst->getArray("colorMatrix", 9, (double *)&cmTest);
	//cinst->getDict("resolution");
	//cinst->getString("exposureMode", &str);
	//cinst->getInt("exposureMode", &i);

	//cinst->startRecording();
	//cinst->getString("cameraApiVersion");
	//cinst->setInt("exposurePeriod", 9876);
	//cinst->setFloat("frameRate", 555);
	//cinst->setString("cameraDescription", "this model");
	//cinst->getString("cameraDescription", &str);
	//cinst->setBool("overlayEnable", true);
	//cinst->getInt("exposurePeriod", &i);
	//cinst->getFloat("frameRate", &d);
	//cinst->getString("cameraDescription", &str);
	//cinst->getString("cameraDescription", &str);
	//cinst->getBool("overlayEnable", &b);
	//cinst->status();
	//cinst->startAutoWhiteBalance();
	//cinst->revertAutoWhiteBalance();

	//cs = cinst->getStatus("one", "two");
	//cinst->getCalCapabilities();
	//cinst->calibrate();
	//cinst->startRecord();
	//cinst->stopRecord();

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	recordingData.valid = false;
	recordingData.hasBeenSaved = false;
	vinst->flushRegions();

	cinst->startRecording();

	recording = true;
	videoHasBeenReviewed = false;

	return SUCCESS;
}

Int32 Camera::setRecSequencerModeNormal()
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;


	//TODO add sequencing
	return SUCCESS;
}

Int32 Camera::stopRecording(void)
{
	if(!recording)
		return CAMERA_NOT_RECORDING;

	cinst->stopRecording();
	//recording = false;

	return SUCCESS;
}

bool Camera::getIsRecording(void)
{
	return recording;
}

UInt32 Camera::setPlayMode(bool playMode)
{
	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(!recordingData.valid)
		return CAMERA_NO_RECORDING_PRESENT;

	playbackMode = playMode;

	if(playMode)
	{
		vinst->setPosition(0);
	}
	else
	{
		vinst->liveDisplay(imagerSettings.geometry.hRes, imagerSettings.geometry.vRes);
	}
	return SUCCESS;
}


void Camera::setTriggerDelayFrames(UInt32 delayFrames)
{
	qDebug("FIXME! setTriggerDelayFrames is not implemented!");
}

void Camera::setBncDriveLevel(UInt32 level)
{
	QVariantMap config;

	config.insert("driveStrength", QVariant(level ? 3 : 0));
	config.insert("debounce", QVariant(true));
	config.insert("invert", QVariant(false));
	config.insert("source", QVariant("alwaysHigh"));

	cinst->setProperty("ioMappingIo1", config);
}

Int32 Camera::autoColGainCorrection(void)
{

}

#define COLOR_MATRIX_MAXVAL	((1 << SENSOR_DATA_WIDTH) * (1 << COLOR_MATRIX_INT_BITS))

void Camera::setCCMatrix(const double *matrix)
{
	double ccmd[9];

	int ccm[] = {
		/* First row */
		(int)(4096.0 * matrix[0]),
		(int)(4096.0 * matrix[1]),
		(int)(4096.0 * matrix[2]),
		/* Second row */
		(int)(4096.0 * matrix[3]),
		(int)(4096.0 * matrix[4]),
		(int)(4096.0 * matrix[5]),
		/* Third row */
		(int)(4096.0 * matrix[6]),
		(int)(4096.0 * matrix[7]),
		(int)(4096.0 * matrix[8])
	};

	/* Check if the colour matrix has clipped, and scale it back down if necessary. */
	int i;
	int peak = 4096;
	for (i = 0; i < 9; i++) {
		if (ccm[i] > peak) peak = ccm[i];
		if (-ccm[i] > peak) peak = -ccm[i];
	}
	if (peak > COLOR_MATRIX_MAXVAL) {
		fprintf(stderr, "Warning: Color matrix has clipped - scaling down to fit\n");
		for (i = 0; i < 9; i++) ccm[i] = (ccm[i] * COLOR_MATRIX_MAXVAL) / peak;
	}

	fprintf(stderr, "Setting Color Matrix:\n");
	fprintf(stderr, "\t%06f %06f %06f\n",   ccm[0] / 4096.0, ccm[1] / 4096.0, ccm[2] / 4096.0);
	fprintf(stderr, "\t%06f %06f %06f\n",   ccm[3] / 4096.0, ccm[4] / 4096.0, ccm[5] / 4096.0);
	fprintf(stderr, "\t%06f %06f %06f\n\n", ccm[6] / 4096.0, ccm[7] / 4096.0, ccm[8] / 4096.0);

	for (i = 0; i < 9; i++) ccmd[i] = ccm[i] / 4096.0;
	cinst->setArray("colorMatrix", 9, (double *)&ccmd);
}

void Camera::setWhiteBalance(const double *rgb)
{


	double wrgb[3];
	wrgb[0] = within(rgb[0] * imgGain, 0.0, 8.0);
	wrgb[1] = within(rgb[1] * imgGain, 0.0, 8.0);
	wrgb[2] = within(rgb[2] * imgGain, 0.0, 8.0);

	fprintf(stderr, "Setting WB Matrix: %06f %06f %06f\n", wrgb[0], wrgb[1], wrgb[2]);

	cinst->setArray("wbMatrix", 3, (double *)&wrgb);
}

Int32 Camera::autoWhiteBalance(UInt32 x, UInt32 y)
{
	cinst->startAutoWhiteBalance();
}

UInt8 Camera::getWBIndex(){
	QSettings appsettings;
	return appsettings.value("camera/WBIndex", 2).toUInt();
}

void Camera::setWBIndex(UInt8 index){
	QSettings appsettings;
	appsettings.setValue("camera/WBIndex", index);
}


void Camera::setFocusAid(bool enable)
{
	UInt32 startX, startY, cropX, cropY;

	if(enable)
	{
		//Set crop window to native resolution of LCD or unchanged if we're scaling up
		if(imagerSettings.geometry.hRes > 600 || imagerSettings.geometry.vRes > 480)
		{
			//Depending on aspect ratio, set the display window appropriately
			if((imagerSettings.geometry.vRes * MAX_FRAME_SIZE_H) > (imagerSettings.geometry.hRes * MAX_FRAME_SIZE_V))	//If it's taller than the display aspect
			{
				cropY = 480;
				cropX = cropY * imagerSettings.geometry.hRes / imagerSettings.geometry.vRes;
				if(cropX & 1)	//If it's odd, round it up to be even
					cropX++;
				startX = (imagerSettings.geometry.hRes - cropX) / 2;
				startY = (imagerSettings.geometry.vRes - cropY) / 2;

			}
			else
			{
				cropX = 600;
				cropY = cropX * imagerSettings.geometry.vRes / imagerSettings.geometry.hRes;
				if(cropY & 1)	//If it's odd, round it up to be even
					cropY++;
				startX = (imagerSettings.geometry.hRes - cropX) / 2;
				startY = (imagerSettings.geometry.vRes - cropY) / 2;

			}
			qDebug() << "Setting startX" << startX << "startY" << startY << "cropX" << cropX << "cropY" << cropY;
			vinst->setScaling(startX & 0xFFFF8, startY, cropX, cropY);	//StartX must be a multiple of 8
		}
	}
	else
	{
		vinst->setScaling(0, 0, imagerSettings.geometry.hRes, imagerSettings.geometry.vRes);
	}
}

bool Camera::getFocusAid()
{
	/* FIXME: Not implemented */
	return false;
}

/* Nearest multiple rounding */
static inline UInt32
round(UInt32  x, UInt32 mult)
{
	UInt32 offset = (x) % (mult);
	return (offset >= mult/2) ? x - offset + mult : x - offset;
}


Int32 Camera::takeWhiteReferences(void)
{
}

bool Camera::getButtonsOnLeft(void){
	QSettings appSettings;
	return (appSettings.value("camera/ButtonsOnLeft", false).toBool());
}

void Camera::setButtonsOnLeft(bool en){
	QSettings appSettings;
	ButtonsOnLeft = en;
	appSettings.setValue("camera/ButtonsOnLeft", en);
}

bool Camera::getUpsideDownDisplay(){
	QSettings appSettings;
	return (appSettings.value("camera/UpsideDownDisplay", false).toBool());
}

void Camera::setUpsideDownDisplay(bool en){
	QSettings appSettings;
	UpsideDownDisplay = en;
	appSettings.setValue("camera/UpsideDownDisplay", en);
}

bool Camera::RotationArgumentIsSet(){
	QString appArguments;
	for(int argumentIndex = 0; argumentIndex < 5 ; argumentIndex++){
		appArguments.append(QApplication::argv()[argumentIndex]);
	}

	if(appArguments.contains("display") && appArguments.contains("transformed:rot0"))
		return true;
	else	return false;
}


void Camera::upsideDownTransform(int rotation){
	QWSDisplay::setTransformation(rotation);
}

bool Camera::getFocusPeakEnable(void)
{
	double level;
	cinst->getFloat("focusPeakingLevel", &level);
	return (level > 0.1);

}
void Camera::setFocusPeakEnable(bool en)
{
	cinst->setFloat("focusPeakingLevel", en ? focusPeakLevel : 0.0);
	focusPeakEnabled = en;
	//vinst->setDisplayOptions(zebraEnabled, focusPeakEnabled);
}

int Camera::getFocusPeakColor(){
	return getFocusPeakColorLL();
}

void Camera::setFocusPeakColor(int value){
	setFocusPeakColorLL(value);
	focusPeakColorIndex = value;
}


bool Camera::getZebraEnable(void)
{
	QSettings appSettings;
	return appSettings.value("camera/zebra", true).toBool();
}

void Camera::setZebraEnable(bool en)
{
	QSettings appSettings;
	zebraEnabled = en;
	appSettings.setValue("camera/zebra", en);
	vinst->setZebra(zebraEnabled);
}

bool Camera::getFanDisable(void)
{
	QSettings appSettings;
	return appSettings.value("camera/fanDisabled", true).toBool();
}

void Camera::setFanDisable(bool en)
{
	QSettings appSettings;
	fanDisabled = en;
	appSettings.setValue("camera/fanDisabled", en);
	//TODO: cinst->setInt("fanSpeed", arg1);
}

int Camera::getUnsavedWarnEnable(void){
	QSettings appSettings;
	return appSettings.value("camera/unsavedWarn", 1).toInt();
	//If there is unsaved video in RAM, prompt to start record.  2=always, 1=if not reviewed, 0=never
}

void Camera::setUnsavedWarnEnable(int newSetting){
	QSettings appSettings;
	unsavedWarnEnabled = newSetting;
	appSettings.setValue("camera/unsavedWarn", newSetting);
}

int Camera::getAutoPowerMode(void){
	QSettings appSettings;
	return appSettings.value("camera/autoPowerMode", 1).toInt();
	//0 = disabled, 1 = turn on if AC inserted, 2 = turn off if AC removed, 3 = both
}

void Camera::setAutoPowerMode(int newSetting){
	qDebug() << "AutoPowerMode:" << newSetting;
	QSettings appSettings;
	autoPowerMode = newSetting;
	appSettings.setValue("camera/autoPowerMode", newSetting);
	bool mainsConnectTurnOn = newSetting & 1;
	bool mainsDisconnectTurnOff = newSetting & 2;
	cinst->setBool("powerOnWhenMainsConnected", mainsConnectTurnOn);
	cinst->setBool("powerOffWhenMainsDisconnected", mainsDisconnectTurnOff);
}

int Camera::getAutoSavePercent(void){
	QSettings appSettings;
	return appSettings.value("camera/autoAutoSavePercent", 1).toInt();
}

void Camera::setAutoSavePercent(int newSetting){
	QSettings appSettings;
	autoSavePercent = newSetting;
	appSettings.setValue("camera/autoAutoSavePercent", newSetting);
	cinst->setFloat("saveAndPowerDownLowBatteryLevelPercent", (double)newSetting);
}

bool Camera::getAutoSavePercentEnabled(void){
	QSettings appSettings;
	return appSettings.value("camera/autoAutoSavePercentEnabled", 1).toBool();
}

void Camera::setAutoSavePercentEnabled(bool newSetting){
	QSettings appSettings;
	autoSavePercentEnabled = newSetting;
	appSettings.setValue("camera/autoAutoSavePercent", newSetting);
	cinst->setBool("saveAndPowerDownWhenLowBattery", newSetting);
}

bool Camera::getShippingMode(void){
	QSettings appSettings;
	return appSettings.value("camera/shippingMode", 1).toBool();
	//0 = disabled, 1 = turn on if AC inserted, 2 = turn off if AC removed, 3 = both
}

void Camera::setShippingMode(int newSetting){
	QSettings appSettings;
	shippingMode = newSetting;
	appSettings.setValue("camera/shippingMode", newSetting);
	cinst->setBool("shippingMode", newSetting);
}



void Camera::set_autoSave(bool state) {
	QSettings appSettings;
	autoSave = state;
	appSettings.setValue("camera/autoSave", state);
}

bool Camera::get_autoSave() {
	QSettings appSettings;
	return appSettings.value("camera/autoSave", false).toBool();
}


void Camera::set_autoRecord(bool state) {
	QSettings appSettings;
	autoRecord = state;
	appSettings.setValue("camera/autoRecord", state);
}

bool Camera::get_autoRecord() {
	QSettings appSettings;
	return appSettings.value("camera/autoRecord", false).toBool();
}


void Camera::set_demoMode(bool state) {
	QSettings appSettings;
	demoMode = state;
	appSettings.setValue("camera/demoMode", state);
}

bool Camera::get_demoMode() {
	QSettings appSettings;
	return appSettings.value("camera/demoMode", false).toBool();
}

void* recDataThread(void *arg)
{
	Camera * cInst = (Camera *)arg;
	int ms = 100;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	bool recording;

	//DEBUG!

	while(!debugDbus && !cInst->terminateRecDataThread) {
		//On the falling edge of recording, call the user callback
		recording = cInst->getRecording();
		if(!recording && (cInst->lastRecording || cInst->recording))	//Take care of situtation where recording goes low->high-low between two interrutps by checking the cInst->recording flag
		{
			recording = false;
			cInst->ui->setRecLEDFront(false);
			cInst->ui->setRecLEDBack(false);
			cInst->recording = false;

		}
		cInst->lastRecording = recording;

		nanosleep(&ts, NULL);
	}

	pthread_exit(NULL);
}

extern CamMainWindow *camMainWindow;



void Camera::apiDoSetFramePeriod(UInt32 period)
{
	sensor->setCurrentPeriod(period);
	//qDebug() << "apiDoSetFramePeriod";
}

void moveCamMainWindow();
void Camera::apiDoSetExposurePeriod(UInt32 period)
{
	camMainWindow->updateParameters();
	//emit receivedParameters();  - can't get this signal to work
	sensor->setCurrentExposure(period);
	//qDebug() << "apiDoSetExposurePeriod";
}

void Camera::apiDoSetCurrentIso(UInt32 iso )
{
	qDebug() << "apiDoSetCurrentIso";
}

void Camera::apiDoSetCurrentGain(UInt32 )
{
	qDebug() << "apiDoSetCurrentGain";
}

void Camera::apiDoSetPlaybackPosition(UInt32 frame)
{
	qDebug() << "apiDoSetPlaybackPosition";
}

void Camera::apiDoSetPlaybackStart(UInt32 frame)
{
	qDebug() << "apiDoSetPlaybackStart";
}

void Camera::apiDoSetPlaybackLength(UInt32 frames)
{
	qDebug() << "apiDoSetPlaybackLength";
}

void Camera::apiDoSetWbTemperature(UInt32 temp)
{
	qDebug() << "apiDoSetWbTemperature";
}

void Camera::apiDoSetRecMaxFrames(UInt32 frames)
{
	qDebug() << "apiDoSetRecMaxFrames";
}

void Camera::apiDoSetRecSegments(UInt32 seg)
{
	qDebug() << "apiDoSetRecSegments";
}

void Camera::apiDoSetRecPreBurst(UInt32 frames)
{
	qDebug() << "apiDoSetRecPreBurst";
}

void Camera::apiDoSetExposurePercent(double percent)
{
	qDebug() << "apiDoSetExposurePercent" << percent;
}

void Camera::apiDoSetExposureNormalized(double norm)
{
	qDebug() << "apiDoSetExposureNormalized";
}

void Camera::apiDoSetIoDelayTime(double delay)
{
	qDebug() << "apiDoSetIoDelayTime" << delay;
}

void Camera::apiDoSetFrameRate(double rate)
{
	qDebug() << "apiDoSetFrameRate";
}

void Camera::apiDoSetShutterAngle(double angle)
{
	qDebug() << "apiDoSetShutterAngle";
}

void Camera::apiDoSetExposureMode(QString mode)
{
	qDebug() << "apiDoSetExposureMode";
}

void Camera::apiDoSetCameraTallyMode(QString mode)
{
	qDebug() << "apiDoSetCameraTallyMode";
}

void Camera::apiDoSetCameraDescription(QString desc)
{
	qDebug() << "apiDoSetCameraDescription";
}

void Camera::apiDoSetNetworkHostname(QString name)
{
	qDebug() << "apiDoSetNetworkHostname";
}

void Camera::apiDoStateChanged(QString state)
{
	qDebug() << "new state:" << state;

	if ((lastState == "idle") && (state == "recording"))
	{
		qDebug() << "### API recording";
		recording = true;
	}
	if ((lastState == "recording") && (state == "idle"))
	{
		qDebug() << "### API stop recording";
	}
	lastState = state;
}

void Camera::apiDoSetWbMatrix(QVariant wb)
{
	QDBusArgument dbusArgs = wb.value<QDBusArgument>();
	dbusArgs.beginArray();
	for (int i=0; i<3; i++)
	{
		whiteBalMatrix[i] = dbusArgs.asVariant().toDouble();
	}
	dbusArgs.endArray();
}

void Camera::apiDoSetColorMatrix(QVariant wb)
{
	QDBusArgument dbusArgs = wb.value<QDBusArgument>();
	dbusArgs.beginArray();
	for (int i=0; i<9; i++)
	{
		colorCalMatrix[i] = dbusArgs.asVariant().toDouble();
	}
	dbusArgs.endArray();
}

void Camera::apiDoSetResolution(QVariant res)
{
	ImagerSettings_t is = getImagerSettings();
	FrameGeometry *geometry = &this->imagerSettings.geometry;
	QVariant qv = res;
	if (qv.isValid()) {
		QVariantMap dict;
		qv.value<QDBusArgument>() >> dict;

		geometry->hRes = dict["hRes"].toInt();
		geometry->vRes = dict["vRes"].toInt();
		geometry->hOffset = dict["hOffset"].toInt();
		geometry->vOffset = dict["vOffset"].toInt();
		geometry->vDarkRows = dict["vDarkRows"].toInt();
		geometry->bitDepth = dict["bitDepth"].toInt();

	}
	else {

	}
}


void Camera::on_chkLiveLoop_stateChanged(int arg1)
{
	if (arg1)
	{
		//enable loop timer
		loopTimer = new QTimer(this);
		connect(loopTimer, SIGNAL(timeout()), this, SLOT(onLoopTimer()));
		loopTimer->start(2000);
		loopTimerEnabled = true;

	}
	else
	{
		//disable loop timer
		loopTimer->stop();
		delete loopTimer;
		vinst->liveDisplay(imagerSettings.geometry.hRes, imagerSettings.geometry.vRes);
		loopTimerEnabled = false;

	}

}


void Camera::onLoopTimer()
{
	//record snippet
	qDebug() << "loop timer";

	cinst->startRecording();

	struct timespec t = {0, 150000000};
	nanosleep(&t, NULL);

	cinst->stopRecording();

	//play back snippet
	vinst->loopPlayback(1, 400, 30);
}

void Camera::on_spinLiveLoopTime_valueChanged(int arg1)
{
	if (arg1 < 250) //minimum time
	{
		return;
	}

	if (loopTimerEnabled)
	{
		//disable loop timer
		loopTimer->stop();
		delete loopTimer;

		//re-enable loop timer
		loopTimer = new QTimer(this);
		connect(loopTimer, SIGNAL(timeout()), this, SLOT(onLoopTimer()));
		loopTimer->start(arg1);
	}
}
