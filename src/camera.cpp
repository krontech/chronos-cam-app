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
#include "sensor.h"
#include "defines.h"
#include <QWSDisplay>
#include "control.h"
#include "exec.h"
#include "camera.h"
#include "pysensor.h"


void* recDataThread(void *arg);
void recordEosCallback(void * arg);
void recordErrorCallback(void * arg, const char * message);



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

CameraErrortype Camera::init(Video * vinstInst, Control * cinstInst, ImageSensor * sensorInst, UserInterface * userInterface, UInt32 ramSizeVal, bool color)
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

	io = new IO();
	retVal = io->init();
	if(retVal != SUCCESS)
		return retVal;

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

	//Set to full resolution
	ImagerSettings_t settings;
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

	setImagerSettings(settings);

	vinst->bitsPerPixel        = appSettings.value("recorder/bitsPerPixel", 0.7).toDouble();
	vinst->maxBitrate          = appSettings.value("recorder/maxBitrate", 40.0).toDouble();
	vinst->framerate           = appSettings.value("recorder/framerate", 60).toUInt();
	strcpy(vinst->filename,      appSettings.value("recorder/filename", "").toString().toAscii());
	strcpy(vinst->fileDirectory, appSettings.value("recorder/fileDirectory", "").toString().toAscii());
	if(strlen(vinst->fileDirectory) == 0){
		/* Set the default file path, or fall back to the MMC card. */
		int i;
		bool fileDirFoundOnUSB = false;
		for (i = 1; i <= 3; i++) {
			sprintf(vinst->fileDirectory, "/media/sda%d", i);
			if (path_is_mounted(vinst->fileDirectory)) {
				fileDirFoundOnUSB = true;
				break;
			}
		}
		if(!fileDirFoundOnUSB) strcpy(vinst->fileDirectory, "/media/mmcblk1p1");
	}


	ui->setRecLEDFront(false);
	ui->setRecLEDBack(false);

	vinst->setDisplayOptions(getZebraEnable(), getFocusPeakEnable());
	vinst->setDisplayPosition(ButtonsOnLeft ^ UpsideDownDisplay);
	cinst->doReset();

	printf("Video init done\n");
	return SUCCESS;
}

UInt32 Camera::setImagerSettings(ImagerSettings_t settings)
{
	QSettings appSettings;

	if(!sensor->isValidResolution(&settings.geometry) ||
		settings.recRegionSizeFrames < RECORD_LENGTH_MIN ||
		settings.segments > settings.recRegionSizeFrames) {
		return CAMERA_INVALID_IMAGER_SETTINGS;
	}

	QString str;
	sensor->setResolution(&settings.geometry);
	sensor->setGain(settings.gain);
	sensor->setFramePeriod(settings.period, &settings.geometry);
	sensor->setIntegrationTime(settings.exposure, &settings.geometry);

	memcpy(&imagerSettings, &settings, sizeof(settings));

	//Zero trigger delay for Gated Burst
	if(settings.mode == RECORD_MODE_GATED_BURST) {
		io->setTriggerDelayFrames(0, FLAG_TEMPORARY);
	}

	UInt32 maxRecRegionSize = getMaxRecordRegionSizeFrames(&imagerSettings.geometry);
	if(settings.recRegionSizeFrames > maxRecRegionSize) {
		imagerSettings.recRegionSizeFrames = maxRecRegionSize;
	}
	else {
		imagerSettings.recRegionSizeFrames = settings.recRegionSizeFrames;
	}

	qDebug()	<< "\nSet imager settings:\nhRes" << imagerSettings.geometry.hRes
				<< "vRes" << imagerSettings.geometry.vRes
				<< "vDark" << imagerSettings.geometry.vDarkRows
				<< "hOffset" << imagerSettings.geometry.hOffset
				<< "vOffset" << imagerSettings.geometry.vOffset
				<< "exposure" << imagerSettings.exposure
				<< "period" << imagerSettings.period
				<< "frameSizeWords" << getFrameSizeWords(&imagerSettings.geometry)
				<< "recRegionSizeFrames" << imagerSettings.recRegionSizeFrames;

	if (settings.temporary) {
		qDebug() << "--- settings --- temporary, not saving";
	}
	else {
		qDebug() << "--- settings --- saving";
		appSettings.setValue("camera/hRes",                 imagerSettings.geometry.hRes);
		appSettings.setValue("camera/vRes",                 imagerSettings.geometry.vRes);
		appSettings.setValue("camera/hOffset",              imagerSettings.geometry.hOffset);
		appSettings.setValue("camera/vOffset",              imagerSettings.geometry.vOffset);
		appSettings.setValue("camera/gain",                 imagerSettings.gain);
		appSettings.setValue("camera/period",               imagerSettings.period);
		appSettings.setValue("camera/exposure",             imagerSettings.exposure);
		appSettings.setValue("camera/recRegionSizeFrames",  imagerSettings.recRegionSizeFrames);
		appSettings.setValue("camera/disableRingBuffer",    imagerSettings.disableRingBuffer);
		appSettings.setValue("camera/mode",                 imagerSettings.mode);
		appSettings.setValue("camera/prerecordFrames",      imagerSettings.prerecordFrames);
		appSettings.setValue("camera/segmentLengthFrames",  imagerSettings.segmentLengthFrames);
		appSettings.setValue("camera/segments",             imagerSettings.segments);
	}

	return SUCCESS;
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
		qDebug("--- Saving settings --- Exposure time: %d", validTime);
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


	//cinst->stopRecording();
	stopRecordingCamJson(&str);
	qDebug() << str;

	str = "ok then";
	qDebug() << str;

	testResolutionCamJson(&str, &geometry);
	qDebug() << str;


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


	//cinst->getIoSettings();
	//cinst->setIoSettings();

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
	//cinst->startZeroTimeBlackCal();
	//cinst->startAutoWhiteBalance();
	//cinst->startBlackCalibration();
	//cinst->revertAutoWhiteBalance();

	//TESTING Control dbus on pressing record
	//cinst->getCameraData();
	//cinst->getSensorData();
	//cinst->getSensorLimits();
	//cinst->setSensorSettings(1280, 1024);
	//cinst->setSensorWhiteBalance(0.5, 0.5, 0.5);
	//cinst->getSensorWhiteBalance();

	//cs = cinst->getStatus("one", "two");
	//cinst->setDescription("hello", 6);
	//cinst->reinitSystem();
	//cinst->setSensorTiming(500);
	//cinst->getSensorCapabilities();
	//cinst->dbusGetIoCapabilities();
	//cinst->getIoMapping();
	//cinst->setIoMapping();
	//cinst->getCalCapabilities();
	//cinst->calibrate();
	//cinst->getColorMatrix();
	//cinst->setColorMatrix();
	//cinst->getSequencerCapabilities();
	//cinst->getSequencerProgram();
	//cinst->setSequencerProgram();
	//cinst->startRecord();
	//cinst->stopRecord();

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	recordingData.valid = false;
	recordingData.hasBeenSaved = false;
	vinst->flushRegions();

	QString jsonReturn;
	startRecordingCamJson(&jsonReturn);

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

Int32 Camera::setRecSequencerModeGatedBurst(UInt32 prerecord)
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	//Set to one plus the last valid address in the record region
	setRecRegion(REC_REGION_START, imagerSettings.recRegionSizeFrames, &imagerSettings.geometry);

	//Two instruction program
	//Instruction 0 records to a single frame while trigger is inactive
	//Instruction 1 records as normal while trigger is active

	//When trigger is inactive, we sit in this 1-frame block, continuously overwriting that frame
	pgmWord.settings.termRecTrig = 0;
	pgmWord.settings.termRecMem = 0;
	pgmWord.settings.termRecBlkEnd = 0;
	pgmWord.settings.termBlkFull = 0;
	pgmWord.settings.termBlkLow = 0;
	pgmWord.settings.termBlkHigh = 1;       //Terminate when trigger becomes active
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 0;
	pgmWord.settings.next = 1;              //Go to next block when this one terminates
	pgmWord.settings.blkSize = prerecord - 1;           //Set to number of frames desired minus one
	pgmWord.settings.pad = 0;

	writeSeqPgmMem(pgmWord, 0);

	pgmWord.settings.termRecTrig = 0;
	pgmWord.settings.termRecMem = imagerSettings.disableRingBuffer ? 1 : 0;;
	pgmWord.settings.termRecBlkEnd = 0;
	pgmWord.settings.termBlkFull = 0;
	pgmWord.settings.termBlkLow = 1;       //Terminate when trigger becomes inactive
	pgmWord.settings.termBlkHigh = 0;
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 0;
	pgmWord.settings.next = 0;              //Go back to block 0
	pgmWord.settings.blkSize = imagerSettings.recRegionSizeFrames-3; //Set to number of frames desired minus one
	pgmWord.settings.pad = 0;

	qDebug() << "---- Sequencer ---- Set to Gated burst mode, second block size:" << pgmWord.settings.blkSize+1;

	writeSeqPgmMem(pgmWord, 1);

	return SUCCESS;
}

Int32 Camera::setRecSequencerModeSingleBlock(UInt32 blockLength, UInt32 frameOffset)
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	if((blockLength + frameOffset) > imagerSettings.recRegionSizeFrames)
		blockLength = imagerSettings.recRegionSizeFrames - frameOffset;

	//Set to one plus the last valid address in the record region
	setRecRegion(REC_REGION_START, imagerSettings.recRegionSizeFrames + frameOffset, &imagerSettings.geometry);

	pgmWord.settings.termRecTrig = 0;
	pgmWord.settings.termRecMem = 0;
	pgmWord.settings.termRecBlkEnd = 1;
	pgmWord.settings.termBlkFull = 1;
	pgmWord.settings.termBlkLow = 0;
	pgmWord.settings.termBlkHigh = 0;
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 0;
	pgmWord.settings.next = 0;
	pgmWord.settings.blkSize = blockLength-1; //Set to number of frames desired minus one
	pgmWord.settings.pad = 0;

	writeSeqPgmMem(pgmWord, 0);

	return SUCCESS;
}

Int32 Camera::setRecSequencerModeCalLoop(void)
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	//Set to one plus the last valid address in the record region

	//add cinst rec region

	pgmWord.settings.termRecTrig = 0;
	pgmWord.settings.termRecMem = 0;
	pgmWord.settings.termRecBlkEnd = 1;
	pgmWord.settings.termBlkFull = 0;
	pgmWord.settings.termBlkLow = 0;
	pgmWord.settings.termBlkHigh = 0;
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 0;
	pgmWord.settings.next = 0;
	pgmWord.settings.blkSize = CAL_REGION_FRAMES - 1;
	pgmWord.settings.pad = 0;

	//pychronos set sequence has not been implemented yet!
	return SUCCESS;
}

Int32 Camera::stopRecording(void)
{
	CameraStatus cs;

	if(!recording)
		return CAMERA_NOT_RECORDING;


	QString jsonReturn;
	stopRecordingCamJson(&jsonReturn);
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


Int32 Camera::autoColGainCorrection(void)
{

}

//Will only decrease exposure, fails if the current set exposure is not clipped
Int32 Camera::adjustExposureToValue(UInt32 level, UInt32 tolerance, bool includeFPNCorrection)
{
	Int32 retVal;
	ImagerSettings_t _is = getImagerSettings();
	UInt32 val;
	UInt32 iterationCount = 0;
	const UInt32 iterationCountMax = 32;

	//Repeat recording frames and halving the exposure until the pixel value is below the desired level
	do {
		retVal = recordFrames(1);
		if(SUCCESS != retVal)
			return retVal;

		val = getMiddlePixelValue(includeFPNCorrection);
		qDebug() << "Got val of" << val;

		//If the pixel value is above the limit, halve the exposure
		if(val > level)
		{
			qDebug() << "Reducing exposure";
			_is.exposure /= 2;
			retVal = setImagerSettings(_is);
			if(SUCCESS != retVal)
				return retVal;
		}
		iterationCount++;
	} while (val > level && iterationCount <= iterationCountMax);

	if(iterationCount > iterationCountMax)
		return CAMERA_ITERATION_LIMIT_EXCEEDED;

	//If we only went through one iteration, the signal wasn't clipped, return failure
	if(1 == iterationCount)
		return CAMERA_LOW_SIGNAL_ERROR;

	//At this point the exposure is set so that the pixel is at or below the desired level
	iterationCount = 0;
	do
	{
		double ratio = (double)level / (double)val;
		qDebug() << "Ratio is "<< ratio << "level:" << level << "val:" << val << "oldExp:" << _is.exposure;
		//Scale exposure by the ratio, this should produce the correct exposure value to get the desired level
		_is.exposure = (UInt32)((double)_is.exposure * ratio);
		qDebug() << "newExp:" << _is.exposure;
		retVal = setImagerSettings(_is);
		if(SUCCESS != retVal)
			return retVal;

		//Check the value was correct
		retVal = recordFrames(1);
		if(SUCCESS != retVal)
			return retVal;

		val = getMiddlePixelValue(includeFPNCorrection);
		iterationCount++;
	} while(abs(val - level) > tolerance && iterationCount <= iterationCountMax);
	qDebug() << "Final exposure set to" << ((double)_is.exposure / 100.0) << "us, final pixel value" << val << "expected pixel value" << level;

	if(iterationCount > iterationCountMax)
		return CAMERA_ITERATION_LIMIT_EXCEEDED;

	return SUCCESS;
}


Int32 Camera::recordFrames(UInt32 numframes)
{
	Int32 retVal;
	UInt32 count;
	const int countMax = 500;
	int ms = 1;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };

	qDebug() << "Starting record of one frame";

	CameraRecordModeType oldMode = imagerSettings.mode;
	imagerSettings.mode = RECORD_MODE_FPN;

	retVal = setRecSequencerModeSingleBlock(numframes + 1);
	if(SUCCESS != retVal)
		return retVal;

	retVal = startRecording();
	if(SUCCESS != retVal)
		return retVal;

	for(count = 0; (count < countMax) && recording; count++) {nanosleep(&ts, NULL);}

	if(count == countMax)	//If after the timeout recording hasn't finished
	{
		qDebug() << "Error: Record failed to stop within timeout period.";

		retVal = stopRecording();
		if(SUCCESS != retVal)
			qDebug() << "Error: Stop Record failed";

		return CAMERA_RECORD_FRAME_ERROR;
	}

	qDebug() << "Record done";

	imagerSettings.mode = oldMode;

	return SUCCESS;
}

UInt32 Camera::getMiddlePixelValue(bool includeFPNCorrection)
{
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::getRawCorrectedFrame(UInt32 frame, UInt16 * frameBuffer)
{
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::getRawCorrectedFramesAveraged(UInt32 frame, UInt32 framesToAverage, UInt16 * frameBuffer)
{
	Int32 retVal;
	double gainCorrection[16];
	UInt32 pixelsPerFrame = recordingData.is.geometry.pixels();

	retVal = readDCG(gainCorrection);
	if(SUCCESS != retVal)
		return retVal;

	UInt16 * fpnUnpacked = new UInt16[pixelsPerFrame];
	if(NULL == fpnUnpacked)
		return CAMERA_MEM_ERROR;

	retVal = readFPN(fpnUnpacked);
	if(SUCCESS != retVal)
	{
		delete fpnUnpacked;
		return retVal;
	}

	//Allocate memory for sum buffer
	UInt32 * summingBuffer = new UInt32[pixelsPerFrame];

	if(NULL == summingBuffer)
		return CAMERA_MEM_ERROR;

	//Zero sum buffer
	for(int i = 0; i < pixelsPerFrame; i++)
		summingBuffer[i] = 0;

	//For each frame to average
	for(int i = 0; i < framesToAverage; i++)
	{
		//Read in the frame
		retVal = readCorrectedFrame(frame + i, frameBuffer, fpnUnpacked, gainCorrection);
		if(SUCCESS != retVal)
		{
			delete fpnUnpacked;
			delete summingBuffer;
			return retVal;
		}

		//Add pixels to sum buffer
		for(int j = 0; j < pixelsPerFrame; j++)
			summingBuffer[j] += frameBuffer[j];

	}

	//Divide to get average and put in result buffer
	for(int i = 0; i < pixelsPerFrame; i++)
		frameBuffer[i] = summingBuffer[i] / framesToAverage;

	delete fpnUnpacked;
	delete summingBuffer;

	return SUCCESS;
}

Int32 Camera::readDCG(double * gainCorrection)
{

}

Int32 Camera::readFPN(UInt16 * fpnUnpacked)
{
}

Int32 Camera::readCorrectedFrame(UInt32 frame, UInt16 * frameBuffer, UInt16 * fpnInput, double * gainCorrection)
{
}

void Camera::loadCCMFromSettings(void)
{
	QSettings appSettings;
	QString currentName = appSettings.value("colorMatrix/current", "").toString();

	/* Special case for the custom color matrix. */
	if (currentName == "Custom") {
		if (appSettings.beginReadArray("colorMatrix") >= 9) {
			for (int i = 0; i < 9; i++) {
				appSettings.setArrayIndex(i);
				colorCalMatrix[i] = appSettings.value("ccmValue").toDouble();
			}
			appSettings.endArray();
			return;
		}
		appSettings.endArray();
	}

	/* For preset matricies, look them up by name. */
	for (int i = 0; i < sizeof(ccmPresets)/sizeof(ccmPresets[0]); i++) {
		if (currentName == ccmPresets[i].name) {
			memcpy(colorCalMatrix, ccmPresets[i].matrix, sizeof(ccmPresets[0].matrix));
			return;
		}
	}

	/* Otherwise, use the default matrix. */
	memcpy(colorCalMatrix, ccmPresets[0].matrix, sizeof(ccmPresets[0].matrix));
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

Int32 Camera::blackCalAllStdRes(bool factory)
{
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
	QSettings appSettings;
	return appSettings.value("camera/focusPeak", false).toBool();
}
void Camera::setFocusPeakEnable(bool en)
{
	QSettings appSettings;
	focusPeakEnabled = en;
	appSettings.setValue("camera/focusPeak", en);
	vinst->setDisplayOptions(zebraEnabled, focusPeakEnabled);
}

int Camera::getFocusPeakColor(){
	QSettings appSettings;
	return appSettings.value("camera/focusPeakColorIndex", 2).toInt();//default setting of 3 is cyan
}

void Camera::setFocusPeakColor(int value){
	QSettings appSettings;
	setFocusPeakColorLL(value);
	focusPeakColorIndex = value;
	appSettings.setValue("camera/focusPeakColorIndex", value);
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
	vinst->setDisplayOptions(zebraEnabled, focusPeakEnabled);
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

	while(!cInst->terminateRecDataThread) {
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



void Camera::apiDoSetFramePeriod(UInt32 period)
{
	sensor->setCurrentPeriod(period);
	//qDebug() << "apiDoSetFramePeriod";
}

void Camera::apiDoSetExposurePeriod(UInt32 period)
{
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


