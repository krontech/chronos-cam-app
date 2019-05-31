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
#include "gpmc.h"
#include "gpmcRegs.h"
#include "cameraRegisters.h"
#include "util.h"
#include "types.h"
#include "lux1310.h"
#include "ecp5Config.h"
#include "defines.h"
#include <QWSDisplay>

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

	//Connect to pcUtil UNIX socket to get power/battery data
	powerDataSocket.connectToServer("/tmp/pcUtil.socket");
	if(powerDataSocket.waitForConnected(500)){
		qDebug("connected to pcUtil server");
	} else {
		qDebug("could not connect to pcUtil socket");
	}

	//Disable Shipping Mode
	this->set_shippingMode(FALSE);

}

Camera::~Camera()
{
	terminateRecDataThread = true;
	pthread_join(recDataThreadID, NULL);
	powerDataSocket.close();
}

CameraErrortype Camera::init(GPMC * gpmcInst, Video * vinstInst, LUX1310 * sensorInst, UserInterface * userInterface, UInt32 ramSizeVal, bool color)
{
	CameraErrortype retVal;
	UInt32 ramSizeGBSlot0, ramSizeGBSlot1;
	QSettings appSettings;

	//Get the memory size
	retVal = (CameraErrortype)getRamSizeGB(&ramSizeGBSlot0, &ramSizeGBSlot1);

	if(retVal != SUCCESS)
		return retVal;

	//Configure FPGA
	Ecp5Config * config;
	const char * configFileName;
	QFileInfo fpgaConfigFile("fpga:FPGA.bit");
	if (fpgaConfigFile.exists() && fpgaConfigFile.isFile()) 
		configFileName = fpgaConfigFile.absoluteFilePath().toLocal8Bit().constData();
	else {
		qDebug("Error: FPGA bitfile not found");
		return CAMERA_FILE_NOT_FOUND;
	}

	config = new Ecp5Config();
	config->init(IMAGE_SENSOR_SPI, FPGA_PROGRAMN_PATH, FPGA_INITN_PATH, FPGA_DONE_PATH, FPGA_SN_PATH, FPGA_HOLDN_PATH, 33000000);
	retVal = (CameraErrortype)config->configure(configFileName);
	delete config;

	if(retVal != SUCCESS)
		return retVal;


	//Read serial number in
	retVal = (CameraErrortype)readSerialNumber(serialNumber);
	if(retVal != SUCCESS)
		return retVal;

	gpmc = gpmcInst;
	vinst = vinstInst;
	sensor = sensorInst;
	ui = userInterface;
	ramSize = (ramSizeGBSlot0 + ramSizeGBSlot1)*1024/32*1024*1024;
	isColor = readIsColor();
	int err;

	//dummy read
	if(getRecording())
		qDebug("rec true at init");
	int v = 0;

	if(1)
	{

		//Reset FPGA
		gpmc->write16(SYSTEM_RESET_ADDR, 1);
		v++;
		//Give the FPGA some time to reset
		delayms(200);
	}

	if(ACCEPTABLE_FPGA_VERSION != getFPGAVersion())
	{
		return CAMERA_WRONG_FPGA_VERSION;
	}

	gpmc->write16(IMAGE_SENSOR_FIFO_START_ADDR, 0x0100);
	gpmc->write16(IMAGE_SENSOR_FIFO_STOP_ADDR, 0x0100);

	gpmc->write32(SEQ_LIVE_ADDR_0_ADDR, LIVE_REGION_START + MAX_FRAME_WORDS*0);
	gpmc->write32(SEQ_LIVE_ADDR_1_ADDR, LIVE_REGION_START + MAX_FRAME_WORDS*1);
	gpmc->write32(SEQ_LIVE_ADDR_2_ADDR, LIVE_REGION_START + MAX_FRAME_WORDS*2);

    if (ramSizeGBSlot1 != 0) {
	if      (ramSizeGBSlot0 == 0)                         { gpmc->write16(MMU_CONFIG_ADDR, MMU_INVERT_CS);      qDebug("--- memory --- invert CS remap"); }
	else if (ramSizeGBSlot0 == 8 && ramSizeGBSlot1 == 16) { gpmc->write16(MMU_CONFIG_ADDR, MMU_INVERT_CS);		qDebug("--- memory --- invert CS remap"); }
		else if (ramSizeGBSlot0 == 8 && ramSizeGBSlot1 == 8)  { gpmc->write16(MMU_CONFIG_ADDR, MMU_SWITCH_STUFFED); qDebug("--- memory --- switch stuffed remap"); }
		else {
			qDebug("--- memory --- no remap");
		}
    }
	else {
		qDebug("--- memory --- no remap");
	}
	qDebug("--- memory --- remap register: 0x%04X", gpmc->read32(MMU_CONFIG_ADDR));


	//enable video readout
	gpmc->write32(DISPLAY_CTL_ADDR, (gpmc->read32(DISPLAY_CTL_ADDR) & ~DISPLAY_CTL_READOUT_INH_MASK) | (isColor ? DISPLAY_CTL_COLOR_MODE_MASK : 0));

	printf("Starting rec data thread\n");
	terminateRecDataThread = false;

	err = pthread_create(&recDataThreadID, NULL, &recDataThread, this);
	if(err)
		return CAMERA_THREAD_ERROR;


	retVal = sensor->init(gpmc);
//mem problem before this
	if(retVal != SUCCESS)
	{
		return retVal;
	}

	io = new IO(gpmc);
	retVal = io->init();
	if(retVal != SUCCESS)
		return retVal;

	/* Load default recording from sensor limits. */
	imagerSettings.geometry = sensor->getMaxGeometry();
	imagerSettings.geometry.vDarkRows = 0;
	imagerSettings.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&imagerSettings.geometry);
	imagerSettings.period = sensor->getMinFramePeriod(&imagerSettings.geometry);
	imagerSettings.exposure = sensor->getMaxExposure(imagerSettings.period);
    imagerSettings.disableRingBuffer = 0;
    imagerSettings.mode = RECORD_MODE_NORMAL;
    imagerSettings.prerecordFrames = 1;
    imagerSettings.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
    imagerSettings.segments = 1;

	setLiveOutputTiming(&imagerSettings.geometry, MAX_LIVE_FRAMERATE);

	vinst->init();

	//Set to full resolution
	ImagerSettings_t settings;

	settings.geometry.hRes          = appSettings.value("camera/hRes", imagerSettings.geometry.hRes).toInt();
	settings.geometry.vRes          = appSettings.value("camera/vRes", imagerSettings.geometry.vRes).toInt();
	settings.geometry.hOffset       = appSettings.value("camera/hOffset", 0).toInt();
	settings.geometry.vOffset       = appSettings.value("camera/vOffset", 0).toInt();
	settings.geometry.vDarkRows     = 0;
	settings.geometry.bitDepth		= imagerSettings.geometry.bitDepth;
	settings.gain                   = appSettings.value("camera/gain", 0).toInt();
	settings.period                 = appSettings.value("camera/period", sensor->getMinFramePeriod(&settings.geometry)).toInt();
    settings.exposure               = appSettings.value("camera/exposure", sensor->getMaxExposure(settings.period)).toInt();
	settings.recRegionSizeFrames    = appSettings.value("camera/recRegionSizeFrames", getMaxRecordRegionSizeFrames(&settings.geometry)).toInt();
    settings.disableRingBuffer      = appSettings.value("camera/disableRingBuffer", 0).toInt();
    settings.mode                   = (CameraRecordModeType)appSettings.value("camera/mode", RECORD_MODE_NORMAL).toInt();
    settings.prerecordFrames        = appSettings.value("camera/prerecordFrames", 1).toInt();
    settings.segmentLengthFrames    = appSettings.value("camera/segmentLengthFrames", settings.recRegionSizeFrames).toInt();
    settings.segments               = appSettings.value("camera/segments", 1).toInt();
    settings.temporary              = 0;

	setImagerSettings(settings);
    setDisplaySettings(false, MAX_LIVE_FRAMERATE);

    io->setTriggerDelayFrames(0, FLAG_USESAVED);
    setTriggerDelayValues((double) io->getTriggerDelayFrames() / settings.recRegionSizeFrames,
			     io->getTriggerDelayFrames() * ((double)settings.period / 100000000),
			     io->getTriggerDelayFrames());


	vinst->setRunning(true);

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

	liveColumnCalibration();

	maxPostFramesRatio = 1;

	if(CAMERA_FILE_NOT_FOUND == loadFPNFromFile()) {
		fastFPNCorrection();
	}

	/* Load color matrix from settings */
	if (isColor) {
		/* White Balance. */
		whiteBalMatrix[0] = appSettings.value("whiteBalance/currentR", 1.35).toDouble();
		whiteBalMatrix[1] = appSettings.value("whiteBalance/currentG", 1.00).toDouble();
		whiteBalMatrix[2] = appSettings.value("whiteBalance/currentB", 1.584).toDouble();

		/* Color Matrix */
		loadCCMFromSettings();
	}
	setCCMatrix(colorCalMatrix);
	setWhiteBalance(whiteBalMatrix);

	vinst->setDisplayOptions(getZebraEnable(), getFocusPeakEnable() ? (FocusPeakColors)getFocusPeakColor() : FOCUS_PEAK_DISABLE);
	vinst->liveDisplay();
	setFocusPeakThresholdLL(appSettings.value("camera/focusPeakThreshold", 25).toUInt());

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

	sensor->seqOnOff(false);
	delayms(10);
	qDebug() << "Settings.period is" << settings.period;

	sensor->setResolution(&settings.geometry);
	sensor->setFramePeriod((double)settings.period/100000000.0, &settings.geometry);
	sensor->setGain(settings.gain);
	sensor->updateWavetableSetting();
	gpmc->write16(SENSOR_LINE_PERIOD_ADDR, max((sensor->currentRes.hRes / LUX1310_HRES_INCREMENT)+2, (sensor->wavetableSize + 3)) - 1);	//Set the timing generator to handle the line period
	delayms(10);
	qDebug() << "About to sensor->setSlaveExposure"; sensor->setSlaveExposure(settings.exposure);

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
	setRecRegion(REC_REGION_START, imagerSettings.recRegionSizeFrames, &imagerSettings.geometry);

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
	io->setTriggerDelayFrames(triggerPostFrames);
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
	if (flags & SETTING_FLAG_USESAVED) {
		validTime = appSettings.value("camera/exposure", sensor->getMaxCurrentIntegrationTime() * 100000000.0).toInt();
		qDebug("--- Using old settings --- Exposure time: %d (default: %d)", validTime, (int)(sensor->getMaxCurrentIntegrationTime() * 100000000.0));
		validTime = (UInt32) (sensor->setIntegrationTime((double)validTime / 100000000.0, fSize) * 100000000.0);
	}
	else {
		validTime = (UInt32) (sensor->setIntegrationTime(intTime, fSize) * 100000000.0);
	}

	if (!(flags & SETTING_FLAG_TEMPORARY)) {
		qDebug("--- Saving settings --- Exposure time: %d", validTime);
		appSettings.setValue("camera/exposure", validTime);
	}
	return SUCCESS;
}

UInt32 Camera::setDisplaySettings(bool encoderSafe, UInt32 maxFps)
{
	if(!sensor->isValidResolution(&imagerSettings.geometry))
		return CAMERA_INVALID_IMAGER_SETTINGS;

	setLiveOutputTiming(&imagerSettings.geometry, maxFps);

	vinst->reload();

	return SUCCESS;
}

void Camera::updateVideoPosition(){
	vinst->setDisplayWindowStartX(ButtonsOnLeft ^ UpsideDownDisplay); //if both of these are true, the video position should actually be 0
	//qDebug()<< "updateVideoPosition() called. ButtonsOnLeft value:  " << ButtonsOnLeft;
}


Int32 Camera::startRecording(void)
{
	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

    switch(imagerSettings.mode)
    {
	case RECORD_MODE_NORMAL:
	case RECORD_MODE_SEGMENTED:
	    setRecSequencerModeNormal();
	break;

	case RECORD_MODE_GATED_BURST:
	    setRecSequencerModeGatedBurst(imagerSettings.prerecordFrames);
	break;

	case RECORD_MODE_FPN:
	    //setRecSequencerModeSingleBlock(17);   //Don't set this here, leave blank so the existing sequencer mode setting for FPN still works
	break;

    }

	recordingData.valid = false;
	recordingData.hasBeenSaved = false;
	vinst->flushRegions();
	startSequencer();
	ui->setRecLEDFront(true);
	ui->setRecLEDBack(true);
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

	setRecRegion(REC_REGION_START, imagerSettings.recRegionSizeFrames, &imagerSettings.geometry);

	pgmWord.settings.termRecTrig = 0;
    pgmWord.settings.termRecMem = imagerSettings.disableRingBuffer ? 1 : 0;     //This currently doesn't work, bug in record sequencer hardware
    pgmWord.settings.termRecBlkEnd = (RECORD_MODE_SEGMENTED == imagerSettings.mode && imagerSettings.segments > 1) ? 0 : 1;
	pgmWord.settings.termBlkFull = 0;
	pgmWord.settings.termBlkLow = 0;
	pgmWord.settings.termBlkHigh = 0;
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 1;
	pgmWord.settings.next = 0;
    pgmWord.settings.blkSize = (imagerSettings.mode == RECORD_MODE_NORMAL ?
				    imagerSettings.recRegionSizeFrames :
				    imagerSettings.recRegionSizeFrames / imagerSettings.segments) - 1; //Set to number of frames desired minus one
	pgmWord.settings.pad = 0;

    qDebug() << "Setting record sequencer mode to" << (imagerSettings.mode == RECORD_MODE_NORMAL ? "normal" : "segmented") << ", disableRingBuffer =" << imagerSettings.disableRingBuffer << "segments ="
	     << imagerSettings.segments << "blkSize =" << pgmWord.settings.blkSize;
	writeSeqPgmMem(pgmWord, 0);

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
	setRecRegion(CAL_REGION_START, CAL_REGION_FRAMES, &imagerSettings.geometry);

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

	writeSeqPgmMem(pgmWord, 0);
	return SUCCESS;
}

Int32 Camera::stopRecording(void)
{
	if(!recording)
		return CAMERA_NOT_RECORDING;

	terminateRecord();
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
		vinst->liveDisplay();
	}
	return SUCCESS;
}

/* Camera::readPixel
 *
 * Reads a 10-bit pixel out of acquisition RAM
 *
 * pixel:	Number of pixels into the image to read from
 * offset:	Offset in bytes
 *
 * returns: Pixel value
 **/
UInt16 Camera::readPixel(UInt32 pixel, UInt32 offset)
{
	UInt32 address = pixel * 10 / 8 + offset;
	UInt8 shift = (pixel & 0x3) * 2;
	return ((gpmc->readRam8(address) >> shift) | (((UInt16)gpmc->readRam8(address+1)) << (8 - shift))) & ((1 << BITS_PER_PIXEL) - 1);
}

/* Camera::writePixel
 *
 * Writes a 10-bit pixel into acquisition RAM
 *
 * pixel:	Number of pixels into the image to write to
 * offset:	Offset in bytes
 * value:	Pixel value to write
 *
 * returns: nothing
 **/
void Camera::writePixel(UInt32 pixel, UInt32 offset, UInt16 value)
{
	UInt32 address = pixel * 10 / 8 + offset;
	UInt8 shift = (pixel & 0x3) * 2;
	UInt8 dataL = gpmc->readRam8(address), dataH = gpmc->readRam8(address+1);

	UInt8 maskL = ~(0xFF << shift);
	UInt8 maskH = ~(0x3FF >> (8 - shift));
	dataL &= maskL;
	dataH &= maskH;

	dataL |= (value << shift);
	dataH |= (value >> (8 - shift));
	gpmc->writeRam8(address, dataL);
	gpmc->writeRam8(address+1, dataH);
}

/* Camera::readPixel12
 *
 * Reads a 12-bit pixel out of acquisition RAM
 *
 * pixel:	Number of pixels into the image to read from
 * offset:	Offset in bytes
 *
 * returns: Pixel value
 **/
UInt16 Camera::readPixel12(UInt32 pixel, UInt32 offset)
{
	UInt32 address = pixel * 12 / 8 + offset;
	UInt8 shift = (pixel & 0x1) * 4;
	return ((gpmc->readRam8(address) >> shift) | (((UInt16)gpmc->readRam8(address+1)) << (8 - shift))) & ((1 << BITS_PER_PIXEL) - 1);
}

/* Camera::readPixel
 *
 * Reads a 10-bit pixel out of a local buffer containing 10-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the buffer to read from
 *
 * returns: Pixel value
 **/
UInt16 Camera::readPixelBuf(UInt8 * buf, UInt32 pixel)
{
	UInt32 address = pixel * 10 / 8;
	UInt8 shift = (pixel & 0x3) * 2;
	return ((buf[address] >> shift) | (((UInt16)buf[address+1]) << (8 - shift))) & ((1 << BITS_PER_PIXEL) - 1);
}

/* Camera::writePixel
 *
 * Writes a 10-bit pixel into a local buffer containing 10-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the image to write to
 * value:	Pixel value to write
 *
 * returns: nothing
 **/
void Camera::writePixelBuf(UInt8 * buf, UInt32 pixel, UInt16 value)
{
	UInt32 address = pixel * 10 / 8;
	UInt8 shift = (pixel & 0x3) * 2;
	UInt8 dataL = buf[address], dataH = buf[address+1];
	//uint8 dataL = 0, dataH = 0;

	UInt8 maskL = ~(0xFF << shift);
	UInt8 maskH = ~(0x3FF >> (8 - shift));
	dataL &= maskL;
	dataH &= maskH;

	dataL |= (value << shift);
	dataH |= (value >> (8 - shift));
	buf[address] = dataL;
	buf[address+1] = dataH;
}

/* Camera::readPixel12
 *
 * Reads a 12-bit pixel out of a local buffer containing 12-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the buffer to read from
 *
 * returns: Pixel value
 **/
UInt16 Camera::readPixelBuf12(UInt8 * buf, UInt32 pixel)
{
	UInt32 address = pixel * 12 / 8;
	UInt8 shift = (pixel & 0x1) * 4;
	return ((buf[address] >> shift) | (((UInt16)buf[address+1]) << (8 - shift))) & ((1 << BITS_PER_PIXEL) - 1);
}

/* Camera::writePixel
 *
 * Writes a 12-bit pixel into a local buffer containing 10-bit packed data
 *
 * buf:		Pointer to image buffer
 * pixel:	Number of pixels into the image to write to
 * value:	Pixel value to write
 *
 * returns: nothing
 **/
void Camera::writePixelBuf12(UInt8 * buf, UInt32 pixel, UInt16 value)
{
	UInt32 address = pixel * 12 / 8;
	UInt8 shift = (pixel & 0x1) * 4;
	UInt8 dataL = buf[address], dataH = buf[address+1];
	//uint8 dataL = 0, dataH = 0;

	UInt8 maskL = ~(0xFF << shift);
	UInt8 maskH = ~(0xFFF >> (8 - shift));
	dataL &= maskL;
	dataH &= maskH;

	dataL |= (value << shift);
	dataH |= (value >> (8 - shift));
	buf[address] = dataL;
	buf[address+1] = dataH;
}

void Camera::loadFPNCorrection(FrameGeometry *geometry, const UInt16 *fpnBuffer, UInt32 framesToAverage)
{
	UInt32 *fpnColumns = (UInt32 *)calloc(geometry->hRes, sizeof(UInt32));
	UInt8  *pixBuffer = (UInt8  *)calloc(1, geometry->size());
	UInt32 maxColumn = 0;

	for(int row = 0; row < geometry->vRes; row++) {
		for(int col = 0; col < geometry->hRes; col++) {
			fpnColumns[col] += fpnBuffer[row * geometry->hRes + col];
		}
	}

	/*
	 * For each column, the sum gives the DC component of the FPN, which
	 * gets applied to the column calibration as the constant term, and
	 * should take ADC gain and curvature into consideration.
	 */
	for (int col = 0; col < geometry->hRes; col++) {
		UInt32 scale = (geometry->vRes * framesToAverage);
		Int64 square = (Int64)fpnColumns[col] * (Int64)fpnColumns[col];
		UInt16 gain = gpmc->read16(COL_GAIN_MEM_START_ADDR + (2 * col));
		Int16 curve = gpmc->read16(COL_CURVE_MEM_START_ADDR + (2 * col));

		/* Column calibration is denoted by:
		 *  f(x) = a*x^2 + b*x + c
		 *
		 * For FPN to output black, f(fpn) = 0 and therefore:
		 *  c = -a*fpn^2 - b*fpn
		 */
		Int64 fpnLinear = -((Int64)fpnColumns[col] * gain);
		Int64 fpnCurved = -(square * curve) / (scale << (COL_CURVE_FRAC_BITS - COL_GAIN_FRAC_BITS));
		Int16 offset = (fpnLinear + fpnCurved) / (scale << COL_GAIN_FRAC_BITS);
		gpmc->write16(COL_OFFSET_MEM_START_ADDR + (2 * col), offset);

#if 0
		fprintf(stderr, "FPN Column %d: gain=%f curve=%f sum=%d, offset=%d\n", col,
				(double)gain / (1 << COL_GAIN_FRAC_BITS),
				(double)curve / (1 << COL_CURVE_FRAC_BITS),
				fpnColumns[col], offset);
#endif

		/* Keep track of the maximum column FPN while we're at it. */
		if ((fpnColumns[col] / scale) > maxColumn) maxColumn = (fpnColumns[col] / scale);
	}

	/* The AC component of each column remains as the per-pixel FPN. */
	for(int row = 0; row < geometry->vRes; row++) {
		for(int col = 0; col < geometry->hRes; col++) {
			int i = row * geometry->hRes + col;
			Int32 fpn = (fpnBuffer[i] - (fpnColumns[col] / geometry->vRes)) / framesToAverage;
			writePixelBuf12(pixBuffer, i, (unsigned)fpn & 0xfff);
		}
	}
	writeAcqMem((UInt32 *)pixBuffer, FPN_ADDRESS, geometry->size());

	/* Update the image gain to compensate for dynamic range lost to the FPN. */
	imgGain = 4096.0 / (double)((1 << geometry->bitDepth) - maxColumn) * IMAGE_GAIN_FUDGE_FACTOR;
	qDebug() << "imgGain set to" << imgGain;
	setWhiteBalance(whiteBalMatrix);

	free(fpnColumns);
	free(pixBuffer);
}

void Camera::computeFPNCorrection(FrameGeometry *geometry, UInt32 wordAddress, UInt32 framesToAverage, bool writeToFile, bool factory)
{
	UInt32 pixelsPerFrame = geometry->pixels();
	const char *formatStr;
	QString filename;
	std::string fn;
	QFile fp;

	// If writing to file - generate the filename and open for writing
	if(writeToFile)
	{
		//Generate the filename for this particular resolution and offset
		if(factory) {
			formatStr = "cal/factoryFPN/fpn_%dx%doff%dx%d";
		}
		else {
			formatStr = "userFPN/fpn_%dx%doff%dx%d";
		}
		
		filename.sprintf(formatStr, geometry->hRes, geometry->vRes, geometry->hOffset, geometry->vOffset);
		fn = sensor->getFilename("", ".raw");
		filename.append(fn.c_str());
		
		qDebug("Writing FPN to file %s", filename.toUtf8().data());

		fp.setFileName(filename);
		fp.open(QIODevice::WriteOnly);
		if(!fp.isOpen())
		{
			qDebug() << "Error: File couldn't be opened";
			return;
		}
	}

	UInt16 * fpnBuffer = (UInt16 *)calloc(pixelsPerFrame, sizeof(UInt16));
	UInt8  * pixBuffer = (UInt8  *)malloc(geometry->size());

	// turn off the sensor
	sensor->seqOnOff(false);

	/* Read frames out of the recorded region and sum their pixels. */
	for(int frame = 0; frame < framesToAverage; frame++) {
		readAcqMem((UInt32 *)pixBuffer, wordAddress, geometry->size());
		for(int row = 0; row < geometry->vRes; row++) {
			for(int col = 0; col < geometry->hRes; col++) {
				int i = row * geometry->hRes + col;
				UInt16 pix = readPixelBuf12(pixBuffer, i);
				fpnBuffer[i] += pix;
			}
		}

		/* Advance to the next frame. */
		wordAddress += getFrameSizeWords(geometry);
	}
	loadFPNCorrection(geometry, fpnBuffer, framesToAverage);

	// restart the sensor
	sensor->seqOnOff(true);

	qDebug() << "About to write file...";
	if(writeToFile)
	{
		quint64 retVal;
		for (int i = 0; i < pixelsPerFrame; i++) {
			fpnBuffer[i] /= framesToAverage;
		}
		retVal = fp.write((const char*)fpnBuffer, sizeof(fpnBuffer[0])*pixelsPerFrame);
		if (retVal != (sizeof(fpnBuffer[0])*pixelsPerFrame)) {
			qDebug("Error writing FPN data to file: %s", fp.errorString().toUtf8().data());
		}
		fp.flush();
		fp.close();
	}

	free(fpnBuffer);
	free(pixBuffer);
}

/* Perform zero-time black cal using the calibration recording region. */
Int32 Camera::fastFPNCorrection(void)
{
	struct timespec tRefresh;
	Int32 retVal;

	io->setShutterGatingEnable(false, FLAG_TEMPORARY);
	setIntegrationTime(0.0, &imagerSettings.geometry, SETTING_FLAG_TEMPORARY);
	retVal = setRecSequencerModeCalLoop();
	if (retVal != SUCCESS) {
		io->setShutterGatingEnable(false, FLAG_USESAVED);
		setIntegrationTime(0.0, &imagerSettings.geometry, SETTING_FLAG_USESAVED);
		return retVal;
	}

	/* Activate the recording sequencer and wait for frames. */
	startSequencer();
	ui->setRecLEDFront(true);
	ui->setRecLEDBack(true);
	tRefresh.tv_sec = 0;
	tRefresh.tv_nsec = ((CAL_REGION_FRAMES + 1) * imagerSettings.period) * 10;
	nanosleep(&tRefresh, NULL);

	/* Terminate the calibration recording. */
	terminateRecord();
	ui->setRecLEDFront(false);
	ui->setRecLEDBack(false);
	io->setShutterGatingEnable(false, FLAG_USESAVED);
	setIntegrationTime(0.0, &imagerSettings.geometry, SETTING_FLAG_USESAVED);

	/* Recalculate the black cal. */
	computeFPNCorrection(&imagerSettings.geometry, CAL_REGION_START, CAL_REGION_FRAMES, false, false);
	return SUCCESS;
}

UInt32 Camera::autoFPNCorrection(UInt32 framesToAverage, bool writeToFile, bool noCap, bool factory)
{
	int count;
	const int countMax = 10;
	int ms = 50;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
//	bool recording;

	if(noCap)
	{
		io->setShutterGatingEnable(false, FLAG_TEMPORARY);
		setIntegrationTime(0.0, &imagerSettings.geometry, SETTING_FLAG_TEMPORARY);
		//nanosleep(&ts, NULL);
	}

	UInt32 retVal;

	qDebug() << "Starting record with a length of" << framesToAverage << "frames";

    CameraRecordModeType oldMode = imagerSettings.mode;
    imagerSettings.mode = RECORD_MODE_FPN;

    retVal = setRecSequencerModeSingleBlock(framesToAverage+1);
    if(SUCCESS != retVal)
	return retVal;

	retVal = startRecording();
	if(SUCCESS != retVal)
		return retVal;

	for(count = 0; (count < countMax) && recording; count++) {nanosleep(&ts, NULL);}

	//Return to set exposure
	if(noCap)
	{
		io->setShutterGatingEnable(false, FLAG_USESAVED);
		setIntegrationTime(0.0, &imagerSettings.geometry, SETTING_FLAG_USESAVED);
	}

	if(count == countMax)	//If after the timeout recording hasn't finished
	{
		qDebug() << "Error: Record failed to stop within timeout period.";

		retVal = stopRecording();
		if(SUCCESS != retVal)
			qDebug() << "Error: Stop Record failed";

		return CAMERA_FPN_CORRECTION_ERROR;
	}

	qDebug() << "Record done, doing normal FPN correction";

    imagerSettings.mode = oldMode;

	computeFPNCorrection(&imagerSettings.geometry, REC_REGION_START, framesToAverage, writeToFile, factory);

	qDebug() << "FPN correction done";
	recordingData.hasBeenSaved = true;

	return SUCCESS;

}

Int32 Camera::loadFPNFromFile(void)
{
	QString filename;
	QFile fp;
	UInt32 retVal = SUCCESS;
	
	//Generate the filename for this particular resolution and offset
	filename.sprintf("fpn:fpn_%dx%doff%dx%d", getImagerSettings().geometry.hRes, getImagerSettings().geometry.vRes, getImagerSettings().geometry.hOffset, getImagerSettings().geometry.vOffset);
	
	std::string fn;
	fn = sensor->getFilename("", ".raw");
	filename.append(fn.c_str());
	QFileInfo fpnResFile(filename);
	if (fpnResFile.exists() && fpnResFile.isFile())
		fn = fpnResFile.absoluteFilePath().toLocal8Bit().constData();
	else {
		qDebug("loadFPNFromFile: File not found %s", filename.toLocal8Bit().constData());
		return CAMERA_FILE_NOT_FOUND;
	}

	qDebug() << "Found FPN file" << fn.c_str();

	fp.setFileName(filename);
	fp.open(QIODevice::ReadOnly);
	if(!fp.isOpen())
	{
		qDebug() << "Error: File couldn't be opened";
		return CAMERA_FILE_ERROR;
	}

	UInt32 pixelsPerFrame = imagerSettings.geometry.pixels();
	UInt16 * buffer = new UInt16[pixelsPerFrame];

	//Read in the active region of the FPN data file.
	if (fp.read((char*) buffer, pixelsPerFrame * sizeof(buffer[0])) < (pixelsPerFrame * sizeof(buffer[0]))) {
		retVal = CAMERA_FILE_ERROR;
		goto loadFPNFromFileCleanup;
	}
	loadFPNCorrection(&imagerSettings.geometry, buffer, 1);
	fp.close();

loadFPNFromFileCleanup:
	delete buffer;

	return retVal;
}

Int32 Camera::computeColGainCorrection(UInt32 framesToAverage, bool writeToFile)
{
	UInt32 retVal = SUCCESS;
	UInt32 pixelsPerFrame = recordingData.is.geometry.pixels();
	UInt32 bytesPerFrame = recordingData.is.geometry.size();

	UInt16 * buffer = new UInt16[pixelsPerFrame];
	UInt16 * fpnBuffer = new UInt16[pixelsPerFrame];
	UInt32 * rawBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawBuffer = (UInt8 *)rawBuffer32;

	QString filename;
	QFile fp;

	int i;
	double minVal = 0;
	double valueSum[16] = {0.0};
	double gainCorrection[16];
	
	// If writing to file - generate the filename and open for writing
	if(writeToFile)
	{
		//Generate the filename for this particular resolution and offset
		if(recordingData.is.gain >= LUX1310_GAIN_4)
			filename.append("cal/dcgH.bin");
		else
			filename.append("cal/dcgL.bin");
		
		qDebug("Writing colGain to file %s", filename.toUtf8().data());

		fp.setFileName(filename);
		fp.open(QIODevice::WriteOnly);
		if(!fp.isOpen())
		{
			qDebug("Error: File couldn't be opened");
			retVal = CAMERA_FILE_ERROR;
			goto computeColGainCorrectionCleanup;
		}
	}

	recordFrames(1);

	//Zero buffer
	memset(buffer, 0, sizeof(buffer)*sizeof(buffer[0]));

	//Read the FPN frame into a buffer
	readAcqMem(rawBuffer32,
			   FPN_ADDRESS,
			   bytesPerFrame);

	//Retrieve pixels from the raw buffer and sum them
	for(i = 0; i < pixelsPerFrame; i++)
	{
		fpnBuffer[i] = readPixelBuf12(rawBuffer, i);
	}

	//Sum pixel values across frames
	for(int frame = 0; frame < framesToAverage; frame++)
	{
		//Get one frame into the raw buffer
		readAcqMem(rawBuffer32,
				   REC_REGION_START + frame * getFrameSizeWords(&recordingData.is.geometry),
				   bytesPerFrame);

		//Retrieve pixels from the raw buffer and sum them
		for(i = 0; i < pixelsPerFrame; i++)
		{
			buffer[i] += readPixelBuf12(rawBuffer, i) - fpnBuffer[i];
		}
	}

	if(isColor)
	{
		//Divide by number summed to get average and write to FPN area
		for(i = 0; i < recordingData.is.geometry.hRes; i++)
		{
			bool isGreen = sensor->getFilterColor(i, recordingData.is.geometry.vRes / 2) == FILTER_COLOR_GREEN;
			UInt32 y = recordingData.is.geometry.vRes / 2 + (isGreen ? 0 : 1);	//Zig zag to we only look at green pixels
			valueSum[i % 16] += (double)buffer[i+recordingData.is.geometry.hRes * y] / (double)framesToAverage;
		}
	}
	else
	{
		//Divide by number summed to get average and write to FPN area
		for(i = 0; i < recordingData.is.geometry.hRes; i++)
		{
			valueSum[i % 16] += (double)buffer[i + recordingData.is.geometry.pixels() / 2] / (double)framesToAverage;
		}
	}

	for(i = 0; i < 16; i++)
	{
		//divide by number of values summed to get pixel value
		valueSum[i] /= ((double)recordingData.is.geometry.hRes/16.0);

		//Find min value
		if(valueSum[i] > minVal)
			minVal = valueSum[i];
	}

	qDebug() << "Gain correction values:";
	for(i = 0; i < 16; i++)
	{
		gainCorrection[i] =  minVal / valueSum[i];


		qDebug() << gainCorrection[i];

	}

	//Check that values are within a sane range
	for(i = 0; i < 16; i++)
	{
		if(gainCorrection[i] < LUX1310_GAIN_CORRECTION_MIN || gainCorrection[i] > LUX1310_GAIN_CORRECTION_MAX) {
			retVal = CAMERA_GAIN_CORRECTION_ERROR;
			goto computeColGainCorrectionCleanup;
		}
	}

	if(writeToFile)
	{
		quint64 retVal64;
		retVal64 = fp.write((const char*)gainCorrection, sizeof(gainCorrection[0])*LUX1310_HRES_INCREMENT);
		if (retVal64 != (sizeof(gainCorrection[0])*LUX1310_HRES_INCREMENT)) {
			qDebug("Error writing colGain data to file: %s (%d vs %d)", fp.errorString().toUtf8().data(), (UInt32) retVal64, sizeof(gainCorrection[0])*LUX1310_HRES_INCREMENT);
		}
		fp.flush();
		fp.close();
	}

	for(i = 0; i < recordingData.is.geometry.hRes; i++)
		gpmc->write16(COL_GAIN_MEM_START_ADDR+(2*i), gainCorrection[i % 16]*4096.0);

computeColGainCorrectionCleanup:
	delete[] buffer;
	delete[] fpnBuffer;
	delete[] rawBuffer32;
	return retVal;
}

/*===============================================================
 * checkForDeadPixels
 *
 * This records a set of frames at full resolution over a few
 * exposure levels then averages them to find the per-pixel
 * deviation.
 *
 * Pass/Fail is returned
 */
#define BAD_PIXEL_RING_SIZE          6
#define DEAD_PIXEL_THRESHHOLD        512
#define MAX_DEAD_PIXELS              0
Int32 Camera::checkForDeadPixels(int* resultCount, int* resultMax) {
	Int32 retVal;

	int exposureSet;
	UInt32 nomExp;

	int i;
	int x, y;

	int averageQuad[4];
	int quad[4];
	int dividerValue;

	int totalFailedPixels = 0;

	ImagerSettings_t _is;

	int frame;
	int stride;
	int lx, ly;
	int endX, endY;
	int yOffset;

	int maxOffset = 0;
	int averageOffset = 0;
	int pixelsInFrame = 0;

	double exposures[] = {0.001,
						  0.125,
						  0.25,
						  0.5,
						  1};

	qDebug("===========================================================================");
	qDebug("Starting dead pixel detection");

	_is.geometry = sensor->getMaxGeometry();
	_is.geometry.vDarkRows = 0;	//inactive dark rows
	_is.exposure = 400000;		//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments
	_is.gain = LUX1310_GAIN_1;
	_is.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&_is.geometry);
    _is.disableRingBuffer = 0;
    _is.mode = RECORD_MODE_NORMAL;
    _is.prerecordFrames = 1;
    _is.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
    _is.segments = 1;
    _is.temporary = 1;

	retVal = setImagerSettings(_is);
	if(SUCCESS != retVal) {
		qDebug("error during setImagerSettings");
		return retVal; // this happens before buffers are made
	}

	UInt32 pixelsPerFrame = imagerSettings.geometry.pixels();
	UInt32 bytesPerFrame = imagerSettings.geometry.size();

	UInt16* buffer = new UInt16[pixelsPerFrame];
	UInt16* fpnBuffer = new UInt16[pixelsPerFrame];
	UInt32* rawBuffer32 = new UInt32[(bytesPerFrame+3) >> 2];
	UInt8* rawBuffer = (UInt8*)rawBuffer32;


	memset(fpnBuffer, 0, sizeof(fpnBuffer));
	//Read the FPN frame into a buffer
	readAcqMem(rawBuffer32,
			   FPN_ADDRESS,
			   bytesPerFrame);

	//Retrieve pixels from the raw buffer and sum them
	for(int i = 0; i < pixelsPerFrame; i++) {
		fpnBuffer[i] = readPixelBuf12(rawBuffer, i);
	}

	retVal = adjustExposureToValue(CAMERA_MAX_EXPOSURE_TARGET, 100, false);
	if(SUCCESS != retVal) {
		qDebug("error during adjustExposureToValue");
		goto checkForDeadPixelsCleanup;
	}

	nomExp = imagerSettings.exposure;

	//For each exposure value
	for(exposureSet = 0; exposureSet < (sizeof(exposures)/sizeof(exposures[0])); exposureSet++) {
		//Set exposure
		_is.exposure = (UInt32)((double)nomExp * exposures[exposureSet]);
		averageOffset = 0;

		retVal = setImagerSettings(_is);
		if(SUCCESS != retVal) {
			qDebug("error during setImagerSettings");
			goto checkForDeadPixelsCleanup;
		}

		qDebug("Recording frames for exposure 1/%ds", 100000000 / _is.exposure);
		//Record frames
		retVal = recordFrames(16);
		if(SUCCESS != retVal) {
			qDebug("error during recordFrames");
			goto checkForDeadPixelsCleanup;
		}

		//Zero buffer
		memset(buffer, 0, sizeof(buffer));



		// Average pixels across frame
		for(frame = 0; frame < 16; frame++) {
			//Get one frame into the raw buffer
			readAcqMem(rawBuffer32,
					   REC_REGION_START + frame * getFrameSizeWords(&recordingData.is.geometry),
					   bytesPerFrame);

			//Retrieve pixels from the raw buffer and sum them
			for(i = 0; i < pixelsPerFrame; i++) {
				buffer[i] += readPixelBuf12(rawBuffer, i) - fpnBuffer[i];
			}
		}
		for(i = 0; i < pixelsPerFrame; i++) {
			buffer[i] >>= 4;
		}

		// take average quad
		averageQuad[0] = averageQuad[1] = averageQuad[2] = averageQuad[3] = 0;
		for (y = 0; y < recordingData.is.geometry.vRes-2; y += 2) {
			stride = _is.geometry.hRes;
			yOffset = y * stride;
			for (x = 0; x < recordingData.is.geometry.hRes-2; x += 2) {
				averageQuad[0] += buffer[x   + yOffset       ];
				averageQuad[1] += buffer[x+1 + yOffset       ];
				averageQuad[2] += buffer[x   + yOffset+stride];
				averageQuad[3] += buffer[x+1 + yOffset+stride];
			}
		}
		averageQuad[0] /= (pixelsPerFrame>>2);
		averageQuad[1] /= (pixelsPerFrame>>2);
		averageQuad[2] /= (pixelsPerFrame>>2);
		averageQuad[3] /= (pixelsPerFrame>>2);

		qDebug("bad pixel detection - average for frame: 0x%04X, 0x%04X, 0x%04X, 0x%04X", averageQuad[0], averageQuad[1], averageQuad[2], averageQuad[3]);
		// note that the average isn't actually used after this point - the variables are reused later

		//dividerValue = (1<<16) / (((BAD_PIXEL_RING_SIZE*2) * (BAD_PIXEL_RING_SIZE*2))/2);

		// Check if pixel is valid
		for (y = 0; y < recordingData.is.geometry.vRes-2; y += 2) {
			for (x = 0; x < recordingData.is.geometry.hRes-2; x += 2) {
				quad[0] = quad[1] = quad[2] = quad[3] = 0;
				averageQuad[0] = averageQuad[1] = averageQuad[2] = averageQuad[3] = 0;

				dividerValue = 0;
				for (ly = -BAD_PIXEL_RING_SIZE; ly <= BAD_PIXEL_RING_SIZE+1; ly+=2) {
					stride = _is.geometry.hRes;
					endY = y + ly;
					if (endY >= 0 && endY < recordingData.is.geometry.vRes-2)
						yOffset = endY * _is.geometry.hRes;
					else
						yOffset = y * _is.geometry.hRes;

					for (lx = -BAD_PIXEL_RING_SIZE; lx <= BAD_PIXEL_RING_SIZE+1; lx+=2) {
						endX = x + lx;
						if (endX >= 0 && endX < recordingData.is.geometry.hRes-2) {
							averageQuad[0] += buffer[endX   + yOffset       ];
							averageQuad[1] += buffer[endX+1 + yOffset       ];
							averageQuad[2] += buffer[endX   + yOffset+stride];
							averageQuad[3] += buffer[endX+1 + yOffset+stride];
							dividerValue++;
						}
					}
				}
				dividerValue = (1<<16) / dividerValue;
				// now divide the outcome by the number of pixels
				//
				averageQuad[0] = (averageQuad[0] * dividerValue);
				averageQuad[0] >>= 16;

				averageQuad[1] = (averageQuad[1] * dividerValue);
				averageQuad[1] >>= 16;

				averageQuad[2] = (averageQuad[2] * dividerValue);
				averageQuad[2] >>= 16;

				averageQuad[3] = (averageQuad[3] * dividerValue);
				averageQuad[3] >>= 16;

				yOffset = y * _is.geometry.hRes;

				quad[0] = averageQuad[0] - buffer[x      + yOffset       ];
				quad[1] = averageQuad[1] - buffer[x   +1 + yOffset       ];
				quad[2] = averageQuad[2] - buffer[x      + yOffset+stride];
				quad[3] = averageQuad[3] - buffer[x   +1 + yOffset+stride];

				if (quad[0] > maxOffset) maxOffset = quad[0];
				if (quad[1] > maxOffset) maxOffset = quad[1];
				if (quad[2] > maxOffset) maxOffset = quad[2];
				if (quad[3] > maxOffset) maxOffset = quad[3];
				if (quad[0] < -maxOffset) maxOffset = -quad[0];
				if (quad[1] < -maxOffset) maxOffset = -quad[1];
				if (quad[2] < -maxOffset) maxOffset = -quad[2];
				if (quad[3] < -maxOffset) maxOffset = -quad[3];

				pixelsInFrame++;
				averageOffset += quad[0] + quad[1] + quad[2] + quad[3];

				if (quad[0] > DEAD_PIXEL_THRESHHOLD || quad[0] < -DEAD_PIXEL_THRESHHOLD) {
					//qDebug("Bad pixel found: %dx%d (0x%04X vs 0x%04X in the local area)", x  , y  , buffer[x      + yOffset       ], averageQuad[0]);
					totalFailedPixels++;
				}
				if (quad[1] > DEAD_PIXEL_THRESHHOLD || quad[1] < -DEAD_PIXEL_THRESHHOLD) {
					//qDebug("Bad pixel found: %dx%d (0x%04X vs 0x%04X in the local area)", x+1, y  , buffer[x   +1 + yOffset       ], averageQuad[1]);
					totalFailedPixels++;
				}
				if (quad[2] > DEAD_PIXEL_THRESHHOLD || quad[2] < -DEAD_PIXEL_THRESHHOLD) {
					//qDebug("Bad pixel found: %dx%d (0x%04X vs 0x%04X in the local area)", x  , y+1, buffer[x      + yOffset+stride], averageQuad[2]);
					totalFailedPixels++;
				}
				if (quad[3] > DEAD_PIXEL_THRESHHOLD || quad[3] < -DEAD_PIXEL_THRESHHOLD) {
					//qDebug("Bad pixel found: %dx%d (0x%04X vs 0x%04X in the local area)", x+1, y+1, buffer[x   +1 + yOffset+stride], averageQuad[3]);
					totalFailedPixels++;
				}
			}
		}

		averageOffset /= pixelsInFrame;
		qDebug("===========================================================================");
		qDebug("Average offset for exposure 1/%ds: %d", 100000000 / _is.exposure, averageOffset);
		qDebug("===========================================================================");
		pixelsInFrame = 0;
		averageOffset = 0;
	}
	qDebug("Total dead pixels found: %d", totalFailedPixels);
	if (totalFailedPixels > MAX_DEAD_PIXELS) {
		retVal = CAMERA_DEAD_PIXEL_FAILED;
		goto checkForDeadPixelsCleanup;
	}
	retVal = SUCCESS;

checkForDeadPixelsCleanup:
	delete buffer;
	delete fpnBuffer;
	delete rawBuffer32;
	qDebug("===========================================================================");
	if (resultMax != NULL) *resultMax = maxOffset;
	if (resultCount != NULL) *resultCount = totalFailedPixels;
	return retVal;
}

void Camera::offsetCorrectionIteration(FrameGeometry *geometry, UInt32 wordAddress, UInt32 framesToAverage)
{
	UInt32 numRows = geometry->vDarkRows ? geometry->vDarkRows : 1;
	UInt32 rowSize = (geometry->hRes * BITS_PER_PIXEL) / 8;
	UInt32 samples = (numRows * framesToAverage * geometry->hRes / LUX1310_HRES_INCREMENT);
	UInt32 adcAverage[LUX1310_HRES_INCREMENT];
	UInt32 adcStdDev[LUX1310_HRES_INCREMENT];

	UInt32 *pxbuffer = (UInt32 *)malloc(rowSize * numRows * framesToAverage);
	Int16 *offsets = sensor->offsetsA;

	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
		adcAverage[i] = 0;
		adcStdDev[i] = 0;
	}
	/* Read out the black regions from all frames. */
	for (int i = 0; i < framesToAverage; i++) {
		UInt32 *rowbuffer = pxbuffer + (rowSize * numRows * i) / sizeof(UInt32);
		readAcqMem(rowbuffer, wordAddress, rowSize * numRows);
		wordAddress += getFrameSizeWords(geometry);
	}

	/* Find the per-ADC averages and standard deviation */
	for (int row = 0; row < (numRows * framesToAverage); row++) {
		for (int col = 0; col < geometry->hRes; col++) {
			adcAverage[col % LUX1310_HRES_INCREMENT] += readPixelBuf12((UInt8 *)pxbuffer, row * geometry->hRes + col);
		}
	}
	for (int row = 0; row < (numRows * framesToAverage); row++) {
		for (int col = 0; col < geometry->hRes; col++) {
			UInt16 pix = readPixelBuf12((UInt8 *)pxbuffer, row * geometry->hRes + col);
			UInt16 avg = adcAverage[col % LUX1310_HRES_INCREMENT] / samples;
			adcStdDev[col % LUX1310_HRES_INCREMENT] += (pix - avg) * (pix - avg);
		}
	}
	for(int col = 0; col < LUX1310_HRES_INCREMENT; col++) {
		adcStdDev[col] = sqrt(adcStdDev[col % LUX1310_HRES_INCREMENT] / (samples - 1));
		adcAverage[col] /= samples;
	}
	free(pxbuffer);

	/* Train the ADC for a target of: Average = Footroom + StandardDeviation */
	for(int col = 0; col < LUX1310_HRES_INCREMENT; col++) {
		UInt16 avg = adcAverage[col % LUX1310_HRES_INCREMENT];
		UInt16 dev = adcStdDev[col % LUX1310_HRES_INCREMENT];

		offsets[col] = offsets[col] - (avg - dev - COL_OFFSET_FOOTROOM) / 2;
		offsets[col] = within(offsets[col], -1023, 1023);
		sensor->setADCOffset(col, offsets[col]);
	}
}

void Camera::computeFPNColumns(FrameGeometry *geometry, UInt32 wordAddress, UInt32 framesToAverage)
{
	UInt32 rowSize = (geometry->hRes * BITS_PER_PIXEL) / 8;
	UInt32 scale = (geometry->vRes * framesToAverage);
	UInt32 *pxBuffer = (UInt32 *)malloc(rowSize * geometry->vRes);
	UInt32 *fpnColumns = (UInt32 *)calloc(geometry->hRes, sizeof(UInt32));

	/* Read and sum the dark columns */
	for (int i = 0; i < framesToAverage; i++) {
		readAcqMem(pxBuffer, wordAddress, rowSize * geometry->vRes);
		for (int row = 0; row < geometry->vRes; row++) {
			for(int col = 0; col < geometry->hRes; col++) {
				fpnColumns[col] += readPixelBuf12((UInt8 *)pxBuffer, row * geometry->hRes + col);
			}
		}
		wordAddress += getFrameSizeWords(geometry);
	}
	/* Write the average value for each column */
	for (int col = 0; col < geometry->hRes; col++) {
		UInt16 gain = gpmc->read16(COL_GAIN_MEM_START_ADDR + (2 * col));
		Int32 offset = ((UInt64)fpnColumns[col] * gain) / (scale << COL_GAIN_FRAC_BITS);
		fprintf(stderr, "FPN Column %d: fpn=%d, gain=%f correction=%d\n", col, fpnColumns[col], (double)gain / (1 << COL_GAIN_FRAC_BITS), -offset);
		gpmc->write16(COL_OFFSET_MEM_START_ADDR + (2 * col), -offset & 0xffff);
	}
	/* Clear out the per-pixel FPN (for now). */
	memset(pxBuffer, 0, rowSize * geometry->vRes);
	writeAcqMem(pxBuffer, FPN_ADDRESS, rowSize * geometry->vRes);
	free(pxBuffer);
	free(fpnColumns);
}

void Camera::computeGainColumns(FrameGeometry *geometry, UInt32 wordAddress, const struct timespec *interval)
{
	UInt32 numRows = 64;
	UInt32 scale = numRows * geometry->hRes / LUX1310_HRES_INCREMENT;
	UInt32 pixFullScale = (1 << BITS_PER_PIXEL);
	UInt32 rowStart = ((geometry->vRes - numRows) / 2) & ~0x1f;
	UInt32 rowSize = (geometry->hRes * BITS_PER_PIXEL) / 8;
	UInt32 *pxBuffer = (UInt32 *)malloc(numRows * rowSize);
	UInt32 highColumns[LUX1310_HRES_INCREMENT] = {0};
	UInt32 midColumns[LUX1310_HRES_INCREMENT] = {0};
	UInt32 lowColumns[LUX1310_HRES_INCREMENT] = {0};
	UInt16 colGain[LUX1310_HRES_INCREMENT];
	Int16 colCurve[LUX1310_HRES_INCREMENT];
	int col;
	UInt32 maxColumn, minColumn;
	unsigned int vhigh, vlow, vmid;

	/* Sample rows from somewhere around the middle of the frame. */
	wordAddress += (rowSize * rowStart) / BYTES_PER_WORD;

	/* Search for a dummy voltage high reference point. */
	for (vhigh = 31; vhigh > 0; vhigh--) {
		sensor->SCIWrite(0x63, 0xC008 | (vhigh << 5));
		nanosleep(interval, NULL);

		/* Get the average pixel value. */
		readAcqMem(pxBuffer, wordAddress, rowSize * numRows);
		memset(highColumns, 0, sizeof(highColumns));
		for (int row = 0; row < numRows; row++) {
			for(int col = 0; col < geometry->hRes; col++) {
				highColumns[col % LUX1310_HRES_INCREMENT] += readPixelBuf12((UInt8 *)pxBuffer, row * geometry->hRes + col);
			}
		}
		maxColumn = 0;
		for (col = 0; col < LUX1310_HRES_INCREMENT; col++) {
			if (highColumns[col] > maxColumn) maxColumn = highColumns[col];
		}
		maxColumn /= scale;

		/* High voltage should be less than 3/4 of full scale */
		if (maxColumn <= (pixFullScale - (pixFullScale / 8))) {
			break;
		}
	}

	/* Search for a dummy voltage low reference point. */
	for (vlow = 0; vlow < vhigh; vlow++) {
		sensor->SCIWrite(0x63, 0xC008 | (vlow << 5));
		nanosleep(interval, NULL);

		/* Get the average pixel value. */
		readAcqMem(pxBuffer, wordAddress, rowSize * numRows);
		memset(lowColumns, 0, sizeof(lowColumns));
		for (int row = 0; row < numRows; row++) {
			for(col = 0; col < geometry->hRes; col++) {
				lowColumns[col % LUX1310_HRES_INCREMENT] += readPixelBuf12((UInt8 *)pxBuffer, row * geometry->hRes + col);
			}
		}
		minColumn = UINT32_MAX;
		for (col = 0; col < LUX1310_HRES_INCREMENT; col++) {
			if (lowColumns[col] < minColumn) minColumn = lowColumns[col];
		}
		minColumn /= scale;

		/* Find the minimum voltage that does not clip. */
		if (minColumn >= COL_OFFSET_FOOTROOM) {
			break;
		}
	}

	/* Sample the midpoint, which should be around quarter scale. */
	vmid = (vhigh + 3*vlow) / 4;
	sensor->SCIWrite(0x63, 0xC008 | (vmid << 5));
	nanosleep(interval, NULL);
	fprintf(stderr, "ADC Calibration voltages: vlow=%d vmid=%d vhigh=%d\n", vlow, vmid, vhigh);

	/* Get the average pixel value. */
	readAcqMem(pxBuffer, wordAddress, rowSize * numRows);
	memset(midColumns, 0, sizeof(midColumns));
	for (int row = 0; row < numRows; row++) {
		for(int col = 0; col < geometry->hRes; col++) {
			midColumns[col % LUX1310_HRES_INCREMENT] += readPixelBuf12((UInt8 *)pxBuffer, row * geometry->hRes + col);
		}
	}
	free(pxBuffer);

	/* Determine which column has the highest response, and sanity check the gain measurements. */
	maxColumn = 0;
	for (col = 0; col < LUX1310_HRES_INCREMENT; col++) {
		UInt32 minrange = (pixFullScale * scale / 16);
		UInt32 diff = highColumns[col] - lowColumns[col];
		if (highColumns[col] <= (midColumns[col] + minrange)) break;
		if (midColumns[col] <= (lowColumns[col] + minrange)) break;
		if (diff > maxColumn) maxColumn = diff;
	}
	if (col != LUX1310_HRES_INCREMENT) {
		qWarning("Warning! ADC Auto calibration range error.");
		for (int col = 0; col < geometry->hRes; col++) {\
			gpmc->write16(COL_GAIN_MEM_START_ADDR + (2 * col), (1 << COL_GAIN_FRAC_BITS));
			gpmc->write16(COL_CURVE_MEM_START_ADDR + (2 * col), 0);
		}
		return;
	}

	/* Compute the 3-point gain calibration coefficients. */
	for (int col = 0; col < LUX1310_HRES_INCREMENT; col++) {
		/* Compute the 2-point calibration coefficients. */
		UInt32 diff = highColumns[col] - lowColumns[col];
		UInt32 gain2pt = ((unsigned long long)maxColumn << COL_GAIN_FRAC_BITS) / diff;
#if 1
		/* Predict the ADC to be linear with dummy voltage and find the error. */
		UInt32 predict = lowColumns[col] + (diff * (vmid - vlow)) / (vhigh - vlow);
		Int32 err2pt = (int)(midColumns[col] - predict);

		/*
		 * Add a parabola to compensate for the curvature. This parabola should have
		 * zeros at the high and low measurement points, and a curvature to compensate
		 * for the error at the middle range. Such a parabola is therefore defined by:
		 *
		 *	f(X) = a*(X - Xlow)*(X - Xhigh), and
		 *  f(Xmid) = -error
		 *
		 * Solving for the curvature gives:
		 *
		 *  a = error / ((Xmid - Xlow) * (Xhigh - Xmid))
		 *
		 * The resulting 3-point calibration function is therefore:
		 *
		 *  Y = a*X^2 + (b - a*Xhigh - a*Xlow)*X + c
		 *  a = three-point curvature correction.
		 *  b = two-point gain correction.
		 *  c = some constant (black level).
		 */
		Int64 divisor = (UInt64)(midColumns[col] - lowColumns[col]) * (UInt64)(highColumns[col] - midColumns[col]) / scale;
		Int32 curve3pt = ((Int64)err2pt << COL_CURVE_FRAC_BITS) / divisor;
		Int32 gainadj = ((Int64)curve3pt * (highColumns[col] + lowColumns[col])) / scale;
		colGain[col] = gain2pt - (gainadj >> (COL_CURVE_FRAC_BITS - COL_GAIN_FRAC_BITS));
		colCurve[col] = curve3pt;

#if 0
		fprintf(stderr, "ADC Column %d 3-point values: 0x%03x, 0x%03x, 0x%03x\n",
						col, lowColumns[col], midColumns[col], highColumns[col]);
		fprintf(stderr, "ADC Column %d 2-point cal: gain2pt=%f predict=0x%03x err2pt=%d\n", col,
						(double)gain2pt / (1 << COL_GAIN_FRAC_BITS), predict, err2pt);
		fprintf(stderr, "ADC Column %d 3-point cal: gain3pt=%f curve3pt=%.9f\n", col,
						(double)colGain[col] / (1 << COL_GAIN_FRAC_BITS),
						(double)colCurve[col] / (1 << COL_CURVE_FRAC_BITS));
		fprintf(stderr, "ADC Column %d 3-point registers: gainadj=%d gain=0x%04x curve=0x%04x\n", col, gainadj, colGain[col], (unsigned)colCurve[col] & 0xffff);
#endif
#else
		/* Apply 2-point calibration */
		colGain[col] = gain2pt;
		colCurve[col] = 0;
#endif
	}

	/* Enable 3-point calibration and load the column gains */
	gpmc->write16(DISPLAY_GAIN_CONTROL_ADDR, DISPLAY_GAIN_CONTROL_3POINT);
	for (int col = 0; col < geometry->hRes; col++) {
		gpmc->write16(COL_GAIN_MEM_START_ADDR + (2 * col), colGain[col % LUX1310_HRES_INCREMENT]);
		gpmc->write16(COL_CURVE_MEM_START_ADDR + (2 * col), colCurve[col % LUX1310_HRES_INCREMENT]);
	}
}

Int32 Camera::liveColumnCalibration(unsigned int iterations)
{
	ImagerSettings_t isPrev = imagerSettings;
	ImagerSettings_t isDark;
	struct timespec tRefresh;
	Int32 retVal;

	/* Swap the black rows into the top of the frame. */
	memcpy(&isDark, &isPrev, sizeof(isDark));
	if (!isDark.geometry.vDarkRows) {
		isDark.geometry.vDarkRows = LUX1310_MAX_V_DARK / 2;
		if ((isDark.geometry.vRes - isDark.geometry.vDarkRows) > sensor->getMinVRes()) {
			isDark.geometry.vRes -= isDark.geometry.vDarkRows;
			isDark.geometry.vOffset += isDark.geometry.vDarkRows;
		}
	}
	isDark.recRegionSizeFrames = CAL_REGION_FRAMES;
	isDark.disableRingBuffer = 0;
	isDark.mode = RECORD_MODE_NORMAL;
	isDark.prerecordFrames = 1;
	isDark.segmentLengthFrames = CAL_REGION_FRAMES;
	isDark.segments = 1;
	isDark.temporary = 1;
	retVal = setImagerSettings(isDark);
	if(SUCCESS != retVal) {
		return retVal;
	}

	retVal = setRecSequencerModeCalLoop();
	if (SUCCESS != retVal) {
		setImagerSettings(isPrev);
		return retVal;
	}

	/* Activate the recording sequencer. */
	startSequencer();
	ui->setRecLEDFront(true);
	ui->setRecLEDBack(true);

	/* Compute the live display worst-case frame refresh time. */
	tRefresh.tv_sec = 0;
	tRefresh.tv_nsec = (CAL_REGION_FRAMES+1) * isDark.period * 10;

	/* Clear out the ADC Offsets. */
	for (int i = 0; i < LUX1310_HRES_INCREMENT; i++) {
		sensor->setADCOffset(i, 0);
	}

	/* Tune the ADC offset calibration. */
	for (int i = 0; i < iterations; i++) {
		nanosleep(&tRefresh, NULL);
		offsetCorrectionIteration(&isDark.geometry, CAL_REGION_START, CAL_REGION_FRAMES);
	}

	sensor->seqOnOff(false);
	delayms(10);
	sensor->updateWavetableSetting(true);
	sensor->seqOnOff(true);
	computeGainColumns(&isDark.geometry, CAL_REGION_START, &tRefresh);

	terminateRecord();
	ui->setRecLEDFront(false);
	ui->setRecLEDBack(false);

	/* Restore the sensor settings. */
	return setImagerSettings(isPrev);
}

Int32 Camera::autoColGainCorrection(void)
{
	Int32 retVal;
	ImagerSettings_t _is;

	_is.geometry = sensor->getMaxGeometry();
	_is.geometry.vDarkRows = 0;	//inactive dark rows on top
	_is.exposure = 400000;		//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments
	_is.gain = LUX1310_GAIN_1;
	_is.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&_is.geometry);
    _is.disableRingBuffer = 0;
    _is.mode = RECORD_MODE_NORMAL;
    _is.prerecordFrames = 1;
    _is.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
    _is.segments = 1;
    _is.temporary = 1;

	retVal = setImagerSettings(_is);
	if(SUCCESS != retVal) {
		qDebug("autoColGainCorrection: Error during setImagerSettings %d", retVal);
		return retVal;
	}

	retVal = adjustExposureToValue(CAMERA_MAX_EXPOSURE_TARGET, 100, false);
	if(SUCCESS != retVal) {
		qDebug("autoColGainCorrection: Error during adjustExposureToValue %d", retVal);
		return retVal;
	}

	retVal = computeColGainCorrection(1, true);
	if(SUCCESS != retVal)
		return retVal;

	_is.gain = LUX1310_GAIN_4;
	retVal = setImagerSettings(_is);
	if(SUCCESS != retVal) {
		qDebug("autoColGainCorrection: Error during setImagerSettings(2) %d", retVal);
		return retVal;
	}

	retVal = adjustExposureToValue(CAMERA_MAX_EXPOSURE_TARGET, 100, false);
	if(SUCCESS != retVal) {
		qDebug("autoColGainCorrection: Error during adjustExposureToValue(2) %d", retVal);
		return retVal;
	}

	retVal = computeColGainCorrection(1, true);
	if(SUCCESS != retVal)
		return retVal;

	return SUCCESS;
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
	//gpmc->write32(GPMC_PAGE_OFFSET_ADDR, REC_REGION_START);	//Read from the beginning of the record region

	UInt32 quadStartX = getImagerSettings().geometry.hRes / 2 & 0xFFFFFFFE;
	UInt32 quadStartY = getImagerSettings().geometry.vRes / 2 & 0xFFFFFFFE;
	//3=G B
	//  R G

	UInt16 bRaw = readPixel12((quadStartY + 1) * imagerSettings.geometry.hRes + quadStartX, REC_REGION_START * BYTES_PER_WORD);
	UInt16 gRaw = readPixel12(quadStartY * imagerSettings.geometry.hRes + quadStartX, REC_REGION_START * BYTES_PER_WORD);
	UInt16 rRaw = readPixel12(quadStartY * imagerSettings.geometry.hRes + quadStartX + 1, REC_REGION_START * BYTES_PER_WORD);

	if(includeFPNCorrection)
	{
		bRaw -= readPixel12((quadStartY + 1) * imagerSettings.geometry.hRes + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
		gRaw -= readPixel12(quadStartY * imagerSettings.geometry.hRes + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
		rRaw -= readPixel12(quadStartY * imagerSettings.geometry.hRes + quadStartX + 1, FPN_ADDRESS * BYTES_PER_WORD);

		//If the result underflowed, clip it to zero
		if(bRaw >= (1 << SENSOR_DATA_WIDTH)) bRaw = 0;
		if(gRaw >= (1 << SENSOR_DATA_WIDTH)) gRaw = 0;
		if(rRaw >= (1 << SENSOR_DATA_WIDTH)) rRaw = 0;
	}

	qDebug() << "RGB values read:" << rRaw << gRaw << bRaw;
	int maxVal = max(rRaw, max(gRaw, bRaw));

	//gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
	return maxVal;
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::readFrame(UInt32 frame, UInt16 * frameBuffer)
{
	const char * filename;
	double gainCorrection[16];

	UInt32 pixelsPerFrame = recordingData.is.geometry.pixels();
	UInt32 bytesPerFrame = recordingData.is.geometry.size();
	UInt32 wordsPerFrame = getFrameSizeWords(&recordingData.is.geometry);

	UInt16 * buffer = new UInt16[pixelsPerFrame];
	UInt32 * rawFrameBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawFrameBuffer = (UInt8 *)rawFrameBuffer32;
	UInt32 * fpnBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * fpnBuffer = (UInt8 *)fpnBuffer32;

	//Read in column gain values
	if(recordingData.is.gain >= LUX1310_GAIN_4)
		filename = "cal/dcgH.bin";
	else
		filename = "cal/dcgL.bin";

	size_t count;

	FILE * fp;
	fp = fopen(filename, "rb");
	if(!fp)
		return CAMERA_FILE_ERROR;

	count = fread(gainCorrection, sizeof(gainCorrection[0]), LUX1310_HRES_INCREMENT, fp);
	fclose(fp);

	//If the file wasn't fully read in, set all gain corrections to 1.0
	if(LUX1310_HRES_INCREMENT != count)
		return CAMERA_FILE_ERROR;

	//Read in the frame and FPN data
	//Get one frame into the raw buffer
	readAcqMem(rawFrameBuffer32, REC_REGION_START + frame * wordsPerFrame, bytesPerFrame);

	//Get one frame into the raw buffer
	readAcqMem(fpnBuffer32, FPN_ADDRESS, bytesPerFrame);

	//Subtract the FPN data from the buffer
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		frameBuffer[i] = readPixelBuf12(rawFrameBuffer, i) - readPixelBuf12(fpnBuffer, i);

		//If the result underflowed, clip it to zero
		if(frameBuffer[i] >= (1 << SENSOR_DATA_WIDTH))
			frameBuffer[i] = 0;

		frameBuffer[i] = (UInt16)((double)frameBuffer[i] * gainCorrection[i % LUX1310_HRES_INCREMENT]);

		//If over max value, clip to max
		if(frameBuffer[i] > ((1 << SENSOR_DATA_WIDTH) - 1))
			frameBuffer[i] = ((1 << SENSOR_DATA_WIDTH) - 1);

	}

	delete buffer;
	delete fpnBuffer32;
	delete rawFrameBuffer32;

	return SUCCESS;
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::getRawCorrectedFrame(UInt32 frame, UInt16 * frameBuffer)
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

	retVal = readCorrectedFrame(frame, frameBuffer, fpnUnpacked, gainCorrection);
	if(SUCCESS != retVal)
	{
		delete fpnUnpacked;
		return retVal;
	}

	delete fpnUnpacked;

	return SUCCESS;
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
	size_t count = 0;
	QString filename;
	QFile fp;
	
	//Generate the filename for this particular resolution and offset
	if(imagerSettings.gain >= LUX1310_GAIN_4)
		filename.sprintf("cal:dcgH.bin");
	else
		filename.sprintf("cal:dcgL.bin");
	
	QFileInfo colGainFile(filename);
	if (colGainFile.exists() && colGainFile.isFile()) {
		qDebug("Found colGain file, %s", colGainFile.absoluteFilePath().toLocal8Bit().constData());
	
		fp.setFileName(filename);
		fp.open(QIODevice::ReadOnly);
		if(!fp.isOpen()) {
			qDebug("Error: File couldn't be opened");
		}
	
		//If the column gain file exists, read it in
		count = fp.read((char*) gainCorrection, sizeof(gainCorrection[0]) * LUX1310_HRES_INCREMENT);
		fp.close();
		if (count < sizeof(gainCorrection[0]) * LUX1310_HRES_INCREMENT) 
			return CAMERA_FILE_ERROR;
	}
	else {
		return CAMERA_FILE_ERROR;
	}

	//If the file wasn't fully read in, fail
	if(LUX1310_HRES_INCREMENT != count)
		return CAMERA_FILE_ERROR;

	return SUCCESS;
}

Int32 Camera::readFPN(UInt16 * fpnUnpacked)
{
	UInt32 pixelsPerFrame = recordingData.is.geometry.pixels();
	UInt32 bytesPerFrame = recordingData.is.geometry.size();
	UInt32 * fpnBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * fpnBuffer = (UInt8 *)fpnBuffer32;

	if(NULL == fpnBuffer32)
		return CAMERA_MEM_ERROR;

	//Read the FPN data in
	readAcqMem(fpnBuffer32,
			   FPN_ADDRESS,
			   bytesPerFrame);

	//Unpack the FPN data
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		fpnUnpacked[i] = readPixelBuf12(fpnBuffer, i);
	}

	delete fpnBuffer32;

	return SUCCESS;
}

Int32 Camera::readCorrectedFrame(UInt32 frame, UInt16 * frameBuffer, UInt16 * fpnInput, double * gainCorrection)
{
	UInt32 pixelsPerFrame = recordingData.is.geometry.pixels();
	UInt32 bytesPerFrame = recordingData.is.geometry.size();

	UInt32 * rawFrameBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawFrameBuffer = (UInt8 *)rawFrameBuffer32;

	if(NULL == rawFrameBuffer32)
		return CAMERA_MEM_ERROR;

	//Read in the frame and FPN data
	//Get one frame into the raw buffer
	readAcqMem(rawFrameBuffer32,
			   REC_REGION_START + frame * getFrameSizeWords(&recordingData.is.geometry),
			   bytesPerFrame);

	//Subtract the FPN data from the buffer
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		frameBuffer[i] = readPixelBuf12(rawFrameBuffer, i) - fpnInput[i];

		//If the result underflowed, clip it to zero
		if(frameBuffer[i] >= (1 << SENSOR_DATA_WIDTH))
			frameBuffer[i] = 0;

		frameBuffer[i] = (UInt16)((double)frameBuffer[i] * gainCorrection[i % LUX1310_HRES_INCREMENT]);

		//If over max value, clip to max
		if(frameBuffer[i] > ((1 << SENSOR_DATA_WIDTH) - 1))
			frameBuffer[i] = ((1 << SENSOR_DATA_WIDTH) - 1);
	}

	delete rawFrameBuffer32;

	return SUCCESS;
}

void setADCOffset(UInt8 channel, Int16 offset);
Int16 getADCOffset(UInt8 channel);

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

	gpmc->write16(CCM_11_ADDR, ccm[0]);
	gpmc->write16(CCM_12_ADDR, ccm[1]);
	gpmc->write16(CCM_13_ADDR, ccm[2]);

	gpmc->write16(CCM_21_ADDR, ccm[3]);
	gpmc->write16(CCM_22_ADDR, ccm[4]);
	gpmc->write16(CCM_23_ADDR, ccm[5]);

	gpmc->write16(CCM_31_ADDR, ccm[6]);
	gpmc->write16(CCM_32_ADDR, ccm[7]);
	gpmc->write16(CCM_33_ADDR, ccm[8]);
}

void Camera::setWhiteBalance(const double *rgb)
{
	double r = rgb[0] * imgGain;
	double g = rgb[1] * imgGain;
	double b = rgb[2] * imgGain;

	/* If imgGain white balance to clip, then scale it back. */
	if ((r > 8.0) || (g > 8.0) || (b > 8.0)) {
		double wbMax = rgb[0];
		if (wbMax < rgb[1]) wbMax = rgb[1];
		if (wbMax < rgb[2]) wbMax = rgb[2];

		qWarning("White Balance clipped due to imgGain of %g", imgGain);
		r = rgb[0] * 8.0 / wbMax;
		g = rgb[1] * 8.0 / wbMax;
		b = rgb[2] * 8.0 / wbMax;
	}

	qDebug("Setting WB Matrix: %06f %06f %06f", r, g, b);

	gpmc->write16(WBAL_RED_ADDR,   (int)(4096.0 * r));
	gpmc->write16(WBAL_GREEN_ADDR, (int)(4096.0 * g));
	gpmc->write16(WBAL_BLUE_ADDR,  (int)(4096.0 * b));
}

Int32 Camera::autoWhiteBalance(UInt32 x, UInt32 y)
{
	const UInt32 min_sum = 100 * (LUX1310_HRES_INCREMENT * LUX1310_HRES_INCREMENT) / 2;

	UInt32 offset = (y & 0xFFFFFFF0) * imagerSettings.geometry.hRes + (x & 0xFFFFFFF0);
	double gain[LUX1310_HRES_INCREMENT];
	UInt32 r_sum = 0;
	UInt32 g_sum = 0;
	UInt32 b_sum = 0;
	double scale;
	int i, j;

	/* Attempt to get the current column gain data, or default to 1.0 (no gain). */
	if (readDCG(gain) != SUCCESS) {
		for (i = 0; i < LUX1310_HRES_INCREMENT; i++) gain[i] = 1.0;
	}

	for (i = 0; i < LUX1310_HRES_INCREMENT; i += 2) {
		/* Even Rows - Green/Red Pixels */
		for (j = 0; j < LUX1310_HRES_INCREMENT; j++) {
			UInt16 fpn = readPixel12(offset + j, FPN_ADDRESS * BYTES_PER_WORD);
			UInt16 pix = readPixel12(offset + j, LIVE_REGION_START * BYTES_PER_WORD);
			if (pix >= (4096 - 128)) {
				return CAMERA_CLIPPED_ERROR;
			}
			if (j & 1) {
				r_sum += gain[j] * (pix - fpn) * 2;
			} else {
				g_sum += gain[j] * (pix - fpn);
			}
		}
		offset += imagerSettings.geometry.hRes;

		/* Odd Rows - Blue/Green Pixels */
		for (j = 0; j < LUX1310_HRES_INCREMENT; j++) {
			UInt16 fpn = readPixel12(offset + j, FPN_ADDRESS * BYTES_PER_WORD);
			UInt16 pix = readPixel12(offset + j, LIVE_REGION_START * BYTES_PER_WORD);
			if (pix >= (4096 - 128)) {
				return CAMERA_CLIPPED_ERROR;
			}
			if (j & 1) {
				g_sum += (pix - fpn) * gain[j];
			} else {
				b_sum += (pix - fpn) * gain[j] * 2;
			}
		}
		offset += imagerSettings.geometry.hRes;
	}

	if ((r_sum < min_sum) || (g_sum < min_sum) || (b_sum < min_sum))
        	return CAMERA_LOW_SIGNAL_ERROR;

	fprintf(stderr, "WhiteBalance RGB average: %d %d %d\n",
			(2*r_sum) / (LUX1310_HRES_INCREMENT * LUX1310_HRES_INCREMENT),
			(2*g_sum) / (LUX1310_HRES_INCREMENT * LUX1310_HRES_INCREMENT),
			(2*b_sum) / (LUX1310_HRES_INCREMENT * LUX1310_HRES_INCREMENT));

	/* Find the highest channel (probably green) */
	scale = g_sum;
	if (scale < r_sum) scale = r_sum;
	if (scale < b_sum) scale = b_sum;

	whiteBalMatrix[0] = scale / r_sum;
	whiteBalMatrix[1] = scale / g_sum;
	whiteBalMatrix[2] = scale / b_sum;

	setWhiteBalance(whiteBalMatrix);
	return SUCCESS;
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
	ImagerSettings_t settings;
	FrameGeometry	maxSize = sensor->getMaxGeometry();

	//Populate the common resolution combo box from the list of resolutions
	QFile fp;
	UInt32 retVal = SUCCESS;
	QString filename;
	QString line;

	vinst->setRunning(false);

	filename.append("camApp:resolutions");
	QFileInfo resolutionsFile(filename);
	if (resolutionsFile.exists() && resolutionsFile.isFile()) {
		fp.setFileName(filename);
		fp.open(QIODevice::ReadOnly);
		if(!fp.isOpen()) {
			qDebug("Error: resolutions file couldn't be opened");
			return CAMERA_FILE_ERROR;
		}
	}
	else {
		qDebug("Error: resolutions file isn't present");
		return CAMERA_FILE_ERROR;
	}

	
	int g;

	//For each gain
	for(g = LUX1310_GAIN_1; g <= LUX1310_GAIN_16; g++)
	{
		if (!fp.reset()) {
			return CAMERA_FILE_ERROR;
		}

		line.sprintf("%ux%u", maxSize.hRes, maxSize.vRes);
		do {
			// Split the resolution string on 'x' into horizontal and vertical sizes.
			QStringList strlist = line.split('x');
			settings.geometry.hRes = strlist[0].toInt(); //pixels
			settings.geometry.vRes = strlist[1].toInt(); //pixels
			settings.geometry.vDarkRows = 0;	//dark rows
			settings.geometry.bitDepth = maxSize.bitDepth;
			settings.geometry.hOffset = round((maxSize.hRes - settings.geometry.hRes) / 2, sensor->getHResIncrement());
			settings.geometry.vOffset = round((maxSize.vRes - settings.geometry.vRes) / 2, sensor->getVResIncrement());
			settings.gain = g;
			settings.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&settings.geometry);
			settings.period = sensor->getMinFramePeriod(&settings.geometry);
			settings.exposure = sensor->getMaxExposure(settings.period);
			settings.disableRingBuffer = 0;
			settings.mode = RECORD_MODE_NORMAL;
			settings.prerecordFrames = 1;
			settings.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
			settings.segments = 1;
			settings.temporary = 0;
			if (sensor->isValidResolution(&settings.geometry)) {
				retVal = setImagerSettings(settings);
				if(SUCCESS != retVal)
					return retVal;

				qDebug("Doing FPN correction for %ux%u...", settings.geometry.hRes, settings.geometry.vRes);
				retVal = liveColumnCalibration();
				if (SUCCESS != retVal)
					return retVal;

				retVal = autoFPNCorrection(16, true, false, factory);	//Factory mode
				if(SUCCESS != retVal)
					return retVal;

				qDebug() << "Done.";
			}

			line = fp.readLine(30);
		} while (!line.isEmpty() && !line.isNull());
	}
	fp.close();

	memcpy(&settings.geometry, &maxSize, sizeof(maxSize));
	settings.geometry.vDarkRows = 0; //Inactive dark rows on top
	settings.gain = LUX1310_GAIN_1;
	settings.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&settings.geometry);
	settings.period = sensor->getMinFramePeriod(&settings.geometry);
	settings.exposure = sensor->getMaxExposure(settings.period);

	retVal = setImagerSettings(settings);
	if(SUCCESS != retVal) {
		qDebug("blackCalAllStdRes: Error during setImagerSettings %d", retVal);
		return retVal;
	}

	retVal = setDisplaySettings(false, MAX_LIVE_FRAMERATE);
	if(SUCCESS != retVal) {
		qDebug("blackCalAllStdRes: Error during setDisplaySettings %d", retVal);
		return retVal;
	}

	retVal = loadFPNFromFile();
	if(SUCCESS != retVal) {
		qDebug("blackCalAllStdRes: Error during loadFPNFromFile %d", retVal);
		return retVal;
	}

	vinst->setRunning(true);

	return SUCCESS;
}




Int32 Camera::takeWhiteReferences(void)
{
	Int32 retVal = SUCCESS;
	ImagerSettings_t _is;
	UInt32 g;
	QFile fp;
	QString filename;
	const char * gName;
	double exposures[] = {0.000244141,
						  0.000488281,
						  0.000976563,
						  0.001953125,
						  0.00390625,
						  0.0078125,
						  0.015625,
						  0.03125,
						  0.0625,
						  0.125,
						  0.25,
						  0.5,
						  1};

	_is.geometry = sensor->getMaxGeometry();
	_is.geometry.vDarkRows = 0;	//Inactive dark rows on top
	_is.exposure = 400000;		//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments
	_is.recRegionSizeFrames = getMaxRecordRegionSizeFrames(&_is.geometry);
	_is.disableRingBuffer = 0;
	_is.mode = RECORD_MODE_NORMAL;
	_is.prerecordFrames = 1;
	_is.segmentLengthFrames = imagerSettings.recRegionSizeFrames;
	_is.segments = 1;
	_is.temporary = 0;

	UInt32 pixelsPerFrame = _is.geometry.pixels();

	UInt16 * frameBuffer = new UInt16[pixelsPerFrame];
	if(NULL == frameBuffer)
		return CAMERA_MEM_ERROR;


	//For each gain
	for(g = LUX1310_GAIN_1; g <= LUX1310_GAIN_16; g++)
	{
		UInt32 nomExp;

		//Set gain then adjust expsure to get initial exposure value
		_is.gain = g;


		retVal = setImagerSettings(_is);
		if(SUCCESS != retVal)
			goto cleanupTakeWhiteReferences;

		retVal = adjustExposureToValue(4096*3/4);
		if(SUCCESS != retVal)
			goto cleanupTakeWhiteReferences;

		nomExp = imagerSettings.exposure;

		//Get the filename
		switch(g)
		{
			case LUX1310_GAIN_1:		gName = LUX1310_GAIN_1_FN;		break;
			case LUX1310_GAIN_2:		gName = LUX1310_GAIN_2_FN;		break;
			case LUX1310_GAIN_4:		gName = LUX1310_GAIN_4_FN;		break;
			case LUX1310_GAIN_8:		gName = LUX1310_GAIN_8_FN;		break;
			case LUX1310_GAIN_16:		gName = LUX1310_GAIN_16_FN;		break;
		default:						gName = "";						break;
		}

		//For each exposure value
		for(int i = 0; i < (sizeof(exposures)/sizeof(exposures[0])); i++)
		{
			//Set exposure
			_is.exposure = (UInt32)((double)nomExp * exposures[i]);

			retVal = setImagerSettings(_is);
			if(SUCCESS != retVal)
			{
				delete frameBuffer;
				return retVal;
			}

			qDebug() << "Recording frames for gain" << gName << "exposure" << i;
			//Record frames
			retVal = recordFrames(16);
			if(SUCCESS != retVal)
				goto cleanupTakeWhiteReferences;

			//Get the frames averaged and save to file
			qDebug() << "Doing getRawCorrectedFramesAveraged()";
			retVal = getRawCorrectedFramesAveraged(0, 16, frameBuffer);
			if(SUCCESS != retVal)
				goto cleanupTakeWhiteReferences;


			//Generate the filename for this particular resolution and offset
			filename.sprintf("userFPN/wref_%s_LV%d.raw", gName, i+1);

			qDebug("Writing WhiteReference to file %s", filename.toUtf8().data());

			fp.setFileName(filename);
			fp.open(QIODevice::WriteOnly);
			if(!fp.isOpen()) {
				qDebug() << "Error: File couldn't be opened";
				retVal = CAMERA_FILE_ERROR;
				goto cleanupTakeWhiteReferences;
			}

			retVal = fp.write((const char*)frameBuffer, sizeof(frameBuffer[0])*pixelsPerFrame);
			if (retVal != (sizeof(frameBuffer[0])*pixelsPerFrame)) {
				qDebug("Error writing WhiteReference data to file: %s", fp.errorString().toUtf8().data());
			}
			fp.flush();
			fp.close();
		}
	}

cleanupTakeWhiteReferences:
	delete frameBuffer;
	return retVal;
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
	appSettings.setValue("camera/focusPeak", en);
	vinst->setDisplayOptions(getZebraEnable(), en ? (FocusPeakColors)getFocusPeakColor() : FOCUS_PEAK_DISABLE);
}

int Camera::getFocusPeakColor(void)
{
	QSettings appSettings;
	return appSettings.value("camera/focusPeakColor", FOCUS_PEAK_CYAN).toInt();
}

void Camera::setFocusPeakColor(int value)
{
	QSettings appSettings;
	appSettings.setValue("camera/focusPeakColor", value);
	vinst->setDisplayOptions(getZebraEnable(), getFocusPeakEnable() ? (FocusPeakColors)value : FOCUS_PEAK_DISABLE);
}

bool Camera::getZebraEnable(void)
{
	QSettings appSettings;
	return appSettings.value("camera/zebra", true).toBool();
}

void Camera::setZebraEnable(bool en)
{
	QSettings appSettings;
	appSettings.setValue("camera/zebra", en);
	vinst->setDisplayOptions(en, getFocusPeakEnable() ? (FocusPeakColors)getFocusPeakColor() : FOCUS_PEAK_DISABLE);
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


void Camera::set_shippingMode(bool state) {
	QSettings appSettings;
	shippingMode = state;
	char buf[256];

	if(state == TRUE){
		powerDataSocket.write("SET_SHIPPING_MODE_ENABLED");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			qDebug() << buf;
			if(!strncmp(buf,"shipping mode enabled",21)){
				appSettings.setValue("camera/shippingMode", TRUE);
			} else {
				appSettings.setValue("camera/shippingMode", FALSE);
			}
		}
	} else {
		powerDataSocket.write("SET_SHIPPING_MODE_DISABLED");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			qDebug() << buf;
			if(!strncmp(buf,"shipping mode disabled",22)){
				appSettings.setValue("camera/shippingMode", FALSE);
			} else {
				appSettings.setValue("camera/shippingMode", TRUE);
			}
		}
	}

}

bool Camera::get_shippingMode() {
	QSettings appSettings;
	return appSettings.value("camera/shippingMode", false).toBool();
}

void Camera::set_autoPowerMode(int mode) {
	QSettings appSettings;
	char buf[256];

	autoPowerMode = mode;

	switch (mode)
	{
	case AUTO_POWER_DISABLED:
		powerDataSocket.write("SET_POWERUP_MODE_0");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			if(!strncmp(buf,"pwrmode0",8)){
				appSettings.setValue("camera/autoPowerMode", AUTO_POWER_DISABLED);
			}
		}
		break;

	case AUTO_POWER_RESTORE_ONLY: //Turn the camera on when the AC adapter is plugged in
		powerDataSocket.write("SET_POWERUP_MODE_1");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			if(!strncmp(buf,"pwrmode1",8)){
				appSettings.setValue("camera/autoPowerMode", AUTO_POWER_RESTORE_ONLY);
			}
		}
		break;

	case AUTO_POWER_REMOVE_ONLY: //Turn the camera off when the AC adapter is removed
		powerDataSocket.write("SET_POWERUP_MODE_2");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			if(!strncmp(buf,"pwrmode2",8)){
				appSettings.setValue("camera/autoPowerMode", AUTO_POWER_REMOVE_ONLY);
			}
		}
		break;

	case AUTO_POWER_BOTH:
		powerDataSocket.write("SET_POWERUP_MODE_3");
		if(powerDataSocket.waitForReadyRead()){
			powerDataSocket.read(buf, sizeof(buf));
			if(!strncmp(buf,"pwrmode3",8)){
				appSettings.setValue("camera/autoPowerMode", AUTO_POWER_BOTH);
			}
		}
		break;

	default:
		appSettings.setValue("camera/autoPowerMode", AUTO_POWER_DISABLED);
		break;
	}
}

int Camera::get_autoPowerMode() {
	QSettings appSettings;
	return appSettings.value("camera/autoPowerMode", 0).toInt();
}


int Camera::get_batteryData(char *buf, size_t bufSize) {
	int len = 0;
	powerDataSocket.write("GET_BATTERY_DATA");
	if(powerDataSocket.waitForReadyRead()){
		len = powerDataSocket.read(buf, bufSize);
		return len;
	} else {
		return 0;
	}
}


void* recDataThread(void *arg)
{
    Camera * cInst = (Camera *)arg;
	int ms = 100;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	bool recording;

	while(!cInst->terminateRecDataThread)
    {
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
