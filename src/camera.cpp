#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <QDebug>
#include <semaphore.h>

#include "font.h"
#include "camera.h"
#include "gpmc.h"
#include "gpmcRegs.h"
#include "cameraRegisters.h"
#include "videoRecord.h"
#include "util.h"
#include "types.h"
#include "lux1310.h"
#include "ecp5Config.h"
#include "defines.h"

void* recDataThread(void *arg);
void frameCallback(void * arg);
void recordEosCallback(void * arg);

Camera::Camera()
{
		recDataPos = 0;
		terminateRecDataThread = false;
		lastRecording = false;
		playbackMode = false;
		recording = false;
		playbackSpeed = 0;
		playbackForward = true;
		playDivisorCount = 0;
		endOfRecCallback = NULL;
		imgGain = 1.0;
		recordingData.hasBeenSaved = true;		//Nothing in RAM at power up so there's nothing to lose
		autoSave = false;
		strcpy(serialNumber, "Not_Set");

/*
		//WPPLS
		ccMatrix[0] = 1.7701;	ccMatrix[1] = -0.3927;	ccMatrix[2] = -0.1725;
		ccMatrix[3] = -0.3323;	ccMatrix[4] = 1.4063;	ccMatrix[5] = -0.1257;
		ccMatrix[6] = -0.1747;	ccMatrix[7] = 0.2080;	ccMatrix[8] = 0.8756;

		//LS
		ccMatrix[0] = 1.7356;	ccMatrix[1] = -0.3398;	ccMatrix[2] = -0.1910;
		ccMatrix[3] = -0.3422;	ccMatrix[4] = 1.4605;	ccMatrix[5] = -0.1701;
		ccMatrix[6] = -0.1165;	ccMatrix[7] = -0.0475;	ccMatrix[8] = 1.0728;

		wbMatrix[0] = 1.0309;	wbMatrix[1] = 1.0;	wbMatrix[2] = 1.4406;

		//LED3000K
		ccMatrix[0] = 1.4689;	ccMatrix[1] = -0.1712;	ccMatrix[2] = -0.0926;
		ccMatrix[3] = -0.3208;	ccMatrix[4] = 1.3082;	ccMatrix[5] = -0.0392;
		ccMatrix[6] = -0.0395;	ccMatrix[7] = -0.1952;	ccMatrix[8] = 1.1435;

		wbMatrix[0] = 0.7424;	wbMatrix[1] = 1.0;	wbMatrix[2] = 2.2724;

*/

/*
 *LUX1310 response
 *
 *		D50
		//WPPLS
		ccMatrix[0] = 1.4996;	ccMatrix[1] = 0.5791;	ccMatrix[2] = -0.8738;
		ccMatrix[3] = -0.4962;	ccMatrix[4] = 1.3805;	ccMatrix[5] = 0.0640;
		ccMatrix[6] = -0.0610;	ccMatrix[7] = -0.6490;	ccMatrix[8] = 1.6188;

		//LS
		ccMatrix[0] = 1.5584;	ccMatrix[1] = 0.4102;	ccMatrix[2] = -0.7083;
		ccMatrix[3] = -0.5440;	ccMatrix[4] = 1.5178;	ccMatrix[5] = -0.0706;
		ccMatrix[6] = -0.0130;	ccMatrix[7] = -0.7868;	ccMatrix[8] = 1.7540;

		//CIECAM02
		ccMatrix[0] = 1.6410;	ccMatrix[1] = -0.0255;	ccMatrix[2] = -0.4398;
		ccMatrix[3] = -0.3992;	ccMatrix[4] = 1.4600;	ccMatrix[5] = -0.0851;
		ccMatrix[6] = -0.0338;	ccMatrix[7] = -0.6913;	ccMatrix[8] = 1.4470;

		wbMatrix[0] = 1.1405;	wbMatrix[1] = 1.0;	wbMatrix[2] = 1.1563;
		*/
		//LED3000K LS
//		ccMatrix[0] = 1.4444;	ccMatrix[1] = 0.7958;	ccMatrix[2] = -1.1809;
//		ccMatrix[3] = -0.5068;	ccMatrix[4] = 1.4137;	ccMatrix[5] = 0.0310;
//		ccMatrix[6] = -0.0675;	ccMatrix[7] = -0.6846;	ccMatrix[8] = 1.7197;


		//LED3000K WPPLS
		ccMatrix[0] = 1.4508;	ccMatrix[1] = 0.6010;	ccMatrix[2] = -0.8470;
		ccMatrix[3] = -0.5063;	ccMatrix[4] = 1.3998;	ccMatrix[5] = 0.0549;
		ccMatrix[6] = -0.0701;	ccMatrix[7] = -0.6060;	ccMatrix[8] = 1.5849;

		wbMatrix[0] = 0.8748;	wbMatrix[1] = 1.0;	wbMatrix[2] = 1.6607;

/*
		//LED3000K WPPLS without white balancing
		ccMatrix[0] = 1.9103;	ccMatrix[1] = 1.0115;	ccMatrix[2] = -1.7170;
		ccMatrix[3] = -0.6258;	ccMatrix[4] = 1.3750;	ccMatrix[5] = 0.1991;
		ccMatrix[6] = -0.3871;	ccMatrix[7] = -0.9426;	ccMatrix[8] = 2.2385;

		wbMatrix[0] = 1.0;	wbMatrix[1] = 1.0;	wbMatrix[2] = 1.0;

		*/


		sem_init(&playMutex, 0, 1);

}

Camera::~Camera()
{
	terminateRecDataThread = true;
	pthread_join(recDataThreadID, NULL);

	sem_destroy(&playMutex);
}

CameraErrortype Camera::init(GPMC * gpmcInst, Video * vinstInst, LUX1310 * sensorInst, UserInterface * userInterface, UInt32 ramSizeVal, bool color)
{
	int FRAME_SIZE = 1280*1024*12/8;
	CameraErrortype retVal;
	UInt32 ramSizeGBSlot0, ramSizeGBSlot1;

	//Get the memory size
	retVal = (CameraErrortype)getRamSizeGB(&ramSizeGBSlot0, &ramSizeGBSlot1);

	if(retVal != CAMERA_SUCCESS)
		return retVal;

	//Configure FPGA
	Ecp5Config * config;
	const char * configFileName;
/*
	//Get the file name based on the RAM installed
	if(8 == ramSizeGBSlot0)
		configFileName = "FPGA_8GB.bit";
	else if(16 == ramSizeGBSlot0)
		configFileName = "FPGA_16GB.bit";
	else
		return CAMERA_MEM_ERROR;
*/
	configFileName = "FPGA.bit";

	config = new Ecp5Config();
	config->init(IMAGE_SENSOR_SPI, FPGA_PROGRAMN_PATH, FPGA_INITN_PATH, FPGA_DONE_PATH, FPGA_SN_PATH, FPGA_HOLDN_PATH, 33000000);
	retVal = (CameraErrortype)config->configure(configFileName);
	delete config;

	if(retVal != CAMERA_SUCCESS)
		return retVal;


	//Read serial number in
	retVal = (CameraErrortype)readSerialNumber(serialNumber);
	if(retVal != CAMERA_SUCCESS)
		return retVal;

	gpmc = gpmcInst;
	vinst = vinstInst;
	sensor = sensorInst;
	ui = userInterface;
	ramSize = ramSizeGBSlot0*1024/32*1024*1024;
	isColor = readIsColor();
	int err;

	//dummy read
	if(getRecording())
		qDebug("rec true at init");
	int v = 0;


//	while(v < 2*1000000)
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

/*
	LUX1310 * lux1310 = new LUX1310();
	retVal = lux1310->init(gpmc);
*/

	//Set video timing
	/*
	//Old video timing that works with monitors
	gpmc->write16(DISPLAY_H_PERIOD_ADDR, 1296+48+112+248-1);
	gpmc->write16(DISPLAY_V_PERIOD_ADDR, 1024+1+3+38-1);
	gpmc->write16(DISPLAY_H_SYNC_LEN_ADDR, 112);
	gpmc->write16(DISPLAY_V_SYNC_LEN_ADDR, 3);
	gpmc->write16(DISPLAY_H_BACK_PORCH_ADDR, 248);
	gpmc->write16(DISPLAY_V_BACK_PORCH_ADDR, 38);
	*/
	//LUPA1300
	gpmc->write16(DISPLAY_H_PERIOD_ADDR, 1296+50+50+166-1);
	gpmc->write16(DISPLAY_V_PERIOD_ADDR, 1024+2+3+38-1);
	gpmc->write16(DISPLAY_H_SYNC_LEN_ADDR, 50);
	gpmc->write16(DISPLAY_V_SYNC_LEN_ADDR, 3);
	gpmc->write16(DISPLAY_H_BACK_PORCH_ADDR, 166);
	gpmc->write16(DISPLAY_V_BACK_PORCH_ADDR, 38);
/*	gpmc->write16(DISPLAY_H_PERIOD_ADDR, 1296+48+112+248-1);
	gpmc->write16(DISPLAY_V_PERIOD_ADDR, 1024+1+3+38-1);
	gpmc->write16(DISPLAY_H_SYNC_LEN_ADDR, 112);
	gpmc->write16(DISPLAY_V_SYNC_LEN_ADDR, 3);
	gpmc->write16(DISPLAY_H_BACK_PORCH_ADDR, 248);
	gpmc->write16(DISPLAY_V_BACK_PORCH_ADDR, 38);*/

	gpmc->write16(IMAGE_SENSOR_FIFO_STOP_W_THRESH_ADDR, 32);

	gpmc->write32(SEQ_LIVE_ADDR_0_ADDR, LIVE_FRAME_0_ADDRESS);
	gpmc->write32(SEQ_LIVE_ADDR_1_ADDR, LIVE_FRAME_1_ADDRESS);
	gpmc->write32(SEQ_LIVE_ADDR_2_ADDR, LIVE_FRAME_2_ADDRESS);
	setRecRegionStartWords(REC_REGION_START);

/*
	//Test RAM
	int len = FRAME_SIZE;
	int errorCount = 0;
	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);	//Set GPMC offset
	srand(1);
	for(int i = 0; i < len; i = i + 2)
	{
		int randVal = rand();
		gpmc->writeRam16(i, randVal);
	}
	int firstErrorAddr;
	int firstCorrectAddr = -1;
	srand(1);
	for(int i = 0; i < len; i = i + 2)
	{
		int readData, randValue;
		randValue = rand() & 0xFFFF;
		readData = gpmc->readRam16(i);
		if(randValue != readData)
		{
			if(errorCount == 0)
				firstErrorAddr = i;
			errorCount++;
		}
		else
		{
			if(firstCorrectAddr == -1)		//Store the frst correct address read back
				firstCorrectAddr = i;
		}

	}
	if(0 == errorCount)
	{
		qDebug() << "Memory test passed.";
	}
	else
	{
		qDebug() << "Memory test failed, " << errorCount << "errors, first at " << firstErrorAddr << ", first correct at " << firstCorrectAddr;
	}
*/

	//enable video readout
	gpmc->write32(DISPLAY_CTL_ADDR, (gpmc->read32(DISPLAY_CTL_ADDR) & ~DISPLAY_CTL_READOUT_INH_MASK) | (isColor ? DISPLAY_CTL_COLOR_MODE_MASK : 0));

	printf("Starting rec data thread\n");
	terminateRecDataThread = false;

	err = pthread_create(&recDataThreadID, NULL, &recDataThread, this);
	if(err)
		return CAMERA_THREAD_ERROR;


	retVal = sensor->init(gpmc);
//mem problem before this
	if(retVal != CAMERA_SUCCESS)
	{
		return retVal;
	}

	imagerSettings.hRes = MAX_FRAME_SIZE_H;
	imagerSettings.vRes = MAX_FRAME_SIZE_V;
	imagerSettings.stride = MAX_STRIDE;
	//frameSizeWords must always be even. Set it larger than needed if required.
	imagerSettings.frameSizeWords = ROUND_UP_MULT((imagerSettings.stride * (imagerSettings.vRes+0) * BITS_PER_PIXEL / 8 + (BYTES_PER_WORD-1)) / BYTES_PER_WORD, FRAME_ALIGN_WORDS);
	imagerSettings.recRegionSizeFrames = (ramSize - REC_REGION_START) / imagerSettings.frameSizeWords;
	imagerSettings.period = sensor->getMinFramePeriod(MAX_STRIDE, MAX_FRAME_SIZE_V);
	imagerSettings.exposure = sensor->getMaxExposure(imagerSettings.period);

	vinst->frameCallbackArg = (void *)this;
	vinst->frameCallback = frameCallback;
	vinst->init();

	//Set to full resolution
	ImagerSettings_t settings;

	settings.hRes = 1280;		//pixels
	settings.vRes = 1024;		//pixels
	settings.stride = 1280;		//Number of pixels per line (allows for dark pixels in the last column)
	settings.hOffset = 0;	//Active area offset from left
	settings.vOffset = 0;		//Active area offset from top
	settings.period = sensor->getMinMasterFramePeriod(settings.stride, settings.vRes) * 100000000.0;
	//settings.period = 1.0 / 500 * 100000000.0;
	settings.exposure = sensor->getMaxIntegrationTime((double)settings.period / 100000000.0, settings.stride, settings.vRes) * 100000000.0 ;
	settings.gain = LUX1310_GAIN_1;
	setImagerSettings(settings);
	setDisplaySettings(false);

	vinst->setRunning(true);

	recorder = new VideoRecord();

	recorder->eosCallback = recordEosCallback;
	recorder->eosCallbackArg = (void *)this;

	//If the FPN file exists, read it in, otherwise write the FPN area with zeroes
	if(CAMERA_FILE_NOT_FOUND == loadFPNFromFile(FPN_FILENAME))
	{
		for(int i = 0; i < FRAME_SIZE; i = i + 4)
			{
				gpmc->writeRam32(i, 0);
			}
	}
	/*
	//If the FPN file exists, read it in
	if( access( "fpn.raw", R_OK ) != -1 )
	{
		FILE * fp;
		fp = fopen("fpn.raw", "rb");
		UInt16 * buffer = new UInt16[MAX_STRIDE*MAX_FRAME_SIZE_V];
		fread(buffer, sizeof(buffer[0]), MAX_STRIDE*MAX_FRAME_SIZE_V, fp);
		fclose(fp);

		UInt32 * packedBuf = new UInt32[FRAME_SIZE*BYTES_PER_WORD/4];
		memset(packedBuf, 0, FRAME_SIZE*BYTES_PER_WORD);

		//Generate packed buffer
		for(int i = 0; i < MAX_STRIDE*MAX_FRAME_SIZE_V; i++)
		{
			//writePixel(i, 0, buffer[i]);
			writePixelBuf12((UInt8 *)packedBuf, i, buffer[i]);
		}

		//Write packed buffer to RAM
		writeAcqMem(packedBuf, FPN_ADDRESS, FRAME_SIZE);

		delete buffer;
		delete packedBuf;
	}
	else //if no file exists, zero out the FPN area
	{
	for(int i = 0; i < FRAME_SIZE; i = i + 4)
		{
			gpmc->writeRam32(i, 0);
		}
	}
	*/
	loadColGainFromFile("cal/dcgL.bin");

	//For mono version, set color matrix to just pass straight through
	if(!isColor)
	{
		ccMatrix[0] = 1.0;	ccMatrix[1] = 0.0;	ccMatrix[2] = 0.0;
		ccMatrix[3] = 0.0;	ccMatrix[4] = 1.0;	ccMatrix[5] = 0.0;
		ccMatrix[6] = 0.0;	ccMatrix[7] = 0.0;	ccMatrix[8] = 1.0;
		wbMatrix[0] = 1.0;	wbMatrix[1] = 1.0;	wbMatrix[2] = 1.0;
	}

	qDebug() << gpmc->read16(CCM_11_ADDR) << gpmc->read16(CCM_12_ADDR) << gpmc->read16(CCM_13_ADDR);
	qDebug() << gpmc->read16(CCM_21_ADDR) << gpmc->read16(CCM_22_ADDR) << gpmc->read16(CCM_23_ADDR);
	qDebug() << gpmc->read16(CCM_31_ADDR) << gpmc->read16(CCM_32_ADDR) << gpmc->read16(CCM_33_ADDR);


	wbMat[0] = wbMat[1] = wbMat[2] = 1.0;
	setCCMatrix(wbMat);

	qDebug() << gpmc->read16(CCM_11_ADDR) << gpmc->read16(CCM_12_ADDR) << gpmc->read16(CCM_13_ADDR);
	qDebug() << gpmc->read16(CCM_21_ADDR) << gpmc->read16(CCM_22_ADDR) << gpmc->read16(CCM_23_ADDR);
	qDebug() << gpmc->read16(CCM_31_ADDR) << gpmc->read16(CCM_32_ADDR) << gpmc->read16(CCM_33_ADDR);

	setFocusPeakEnable(false);
	setZebraEnable(true);

	printf("Video init done\n");

	io = new IO(gpmc);
	retVal = io->init();
	if(retVal != CAMERA_SUCCESS)
		return retVal;

	//Set trigger for normally open switch on IO1
	io->setTriggerInvert(1);
	io->setTriggerEnable(1);
	io->setTriggerDebounceEn(1);
	io->setOutLevel((1 << 1));	//Enable strong pullup

	return CAMERA_SUCCESS;

}

UInt32 Camera::setImagerSettings(ImagerSettings_t settings)
{
	if(!sensor->isValidResolution(settings.stride, settings.vRes, settings.hOffset, settings.vOffset))
		return CAMERA_INVALID_IMAGER_SETTINGS;

	//sensor->setSlaveExposure(0);	//Disable integration
	gpmc->write32(IMAGER_INT_TIME_ADDR, 0);
	delayms(10);
	qDebug() << "Settings.period is" << settings.period;
	//sensor->seqOnOff(false);
	sensor->setResolution(settings.hOffset, settings.stride, settings.vOffset, settings.vOffset + settings.vRes - 1);
	sensor->setFramePeriod((double)settings.period/100000000.0, settings.stride, settings.vRes);
	//sensor->setSlavePeriod(settings.period*100);
	sensor->setGain(settings.gain);
	sensor->updateWavetableSetting();
	gpmc->write16(SENSOR_LINE_PERIOD_ADDR, max((sensor->currentHRes / LUX1310_HRES_INCREMENT)+2, (sensor->wavetableSize + 3)) - 1);	//Set the timing generator to handle the line period
	delayms(10);
	qDebug() << "About to sensor->setSlaveExposure"; sensor->setSlaveExposure(settings.exposure);
	//sensor->seqOnOff(true);

	imagerSettings.hRes = settings.hRes;
	imagerSettings.stride = settings.stride;
	imagerSettings.vRes = settings.vRes;
	imagerSettings.hOffset = settings.hOffset;
	imagerSettings.vOffset = settings.vOffset;
	imagerSettings.period = settings.period;
	imagerSettings.exposure = settings.exposure;
	imagerSettings.gain = settings.gain;

	imagerSettings.frameSizeWords = ROUND_UP_MULT((settings.stride * (settings.vRes+0) * 12 / 8 + (BYTES_PER_WORD - 1)) / BYTES_PER_WORD, FRAME_ALIGN_WORDS);	//Enough words to fit the frame, but make it even
	imagerSettings.recRegionSizeFrames = (ramSize - REC_REGION_START) / imagerSettings.frameSizeWords;
	setFrameSizeWords(imagerSettings.frameSizeWords);

	qDebug() << "About to sensor->loadADCOffsetsFromFile"; sensor->loadADCOffsetsFromFile("a");

	if(imagerSettings.gain >= LUX1310_GAIN_4)
		loadColGainFromFile("cal/dcgH.bin");
	else
		loadColGainFromFile("cal/dcgL.bin");

	qDebug()	<< "\nSet imager settings:\nhRes" << imagerSettings.hRes
				<< "vRes" << imagerSettings.vRes
				<< "stride" << imagerSettings.stride
				<< "hOffset" << imagerSettings.hOffset
				<< "vOffset" << imagerSettings.vOffset
				<< "exposure" << imagerSettings.exposure
				<< "period" << imagerSettings.period
				<< "frameSizeWords" << imagerSettings.frameSizeWords
				<< "recRegionSizeFrames" << imagerSettings.recRegionSizeFrames;

	return CAMERA_SUCCESS;

}

UInt32 Camera::setDisplaySettings(bool encoderSafe)
{
	if(!sensor->isValidResolution(imagerSettings.stride, imagerSettings.vRes, imagerSettings.hOffset, imagerSettings.vOffset))
		return CAMERA_INVALID_IMAGER_SETTINGS;

	//Stop live video if running
	bool running = vinst->isRunning();
	if(running)
		vinst->setRunning(false);

	//setLiveOutputTiming(imagerSettings.stride, imagerSettings.vRes, encoderSafe ? (imagerSettings.hRes + 15) & 0xFFFFFFF0 : imagerSettings.hRes);	//Multiple of 16 is safe for encoder
	setLiveOutputTiming2(imagerSettings.stride,
						 imagerSettings.vRes,
						 encoderSafe ? recorder->getSafeHRes(imagerSettings.hRes) : imagerSettings.hRes,
						 encoderSafe ? recorder->getSafeVRes(imagerSettings.vRes) : imagerSettings.vRes,
						 encoderSafe);	//Multiple of 16 is safe for encoder

	vinst->setImagerResolution(imagerSettings.hRes, imagerSettings.vRes);

	//Restart live video if it was running
	if(running)
		vinst->setRunning(true);

	return CAMERA_SUCCESS;
}


Int32 Camera::startRecording(void)
{
	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	recDataPos = 0;
	recDataLength = 0;
	recordingData.valid = false;
	recordingData.hasBeenSaved = false;
	startSequencer();
	ui->setRecLEDFront(true);
	ui->setRecLEDBack(true);
	recording = true;

	return CAMERA_SUCCESS;
}

Int32 Camera::setRecSequencerModeNormal()
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	//Set to one plus the last valid address in the record region
	setRecRegionEndWords(REC_REGION_START + imagerSettings.recRegionSizeFrames * imagerSettings.frameSizeWords);

	pgmWord.settings.termRecTrig = 0;
	pgmWord.settings.termRecMem = 0;
	pgmWord.settings.termRecBlkEnd = 1;
	pgmWord.settings.termBlkFull = 0;
	pgmWord.settings.termBlkLow = 0;
	pgmWord.settings.termBlkHigh = 0;
	pgmWord.settings.termBlkFalling = 0;
	pgmWord.settings.termBlkRising = 1;
	pgmWord.settings.next = 0;
	pgmWord.settings.blkSize = imagerSettings.recRegionSizeFrames-1; //Set to number of frames desired minus one
	pgmWord.settings.pad = 0;

	writeSeqPgmMem(pgmWord, 0);

	setFrameSizeWords(imagerSettings.frameSizeWords);

	return CAMERA_SUCCESS;
}

Int32 Camera::setRecSequencerModeSingleBlock(UInt32 blockLength)
{
	SeqPgmMemWord pgmWord;

	if(recording)
		return CAMERA_ALREADY_RECORDING;
	if(playbackMode)
		return CAMERA_IN_PLAYBACK_MODE;

	if(blockLength > imagerSettings.recRegionSizeFrames)
		blockLength = imagerSettings.recRegionSizeFrames;

	//Set to one plus the last valid address in the record region
	setRecRegionEndWords(REC_REGION_START + imagerSettings.recRegionSizeFrames * imagerSettings.frameSizeWords);

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

	setFrameSizeWords(imagerSettings.frameSizeWords);

	return CAMERA_SUCCESS;
}

Int32 Camera::stopRecording(void)
{
	if(!recording)
		return CAMERA_NOT_RECORDING;

	terminateRecord();
	//recording = false;

	return CAMERA_SUCCESS;
}

//Find the earliest fully valid block

void Camera::endOfRec(void)
{
	UInt32 lastRecDataPos = recDataPos == 0 ? RECORD_DATA_LENGTH - 1 : recDataPos - 1;
	UInt32 firstBlock, numBlocks;
	UInt32 blockFrames;
	UInt32 frames;

	qDebug("EndOfRec");

	if(0 == recDataLength)
	{
		recordingData.valid = false;
		recordingData.hasBeenSaved = true;		//We didn't record anything so there's nothing to lose by overwriting
	}
	else
	{
		recordingData.is = imagerSettings;

		frames = 0;
		numBlocks = 0;
		//Iterate through record data records starting from the latest and going back, until we have accounted for all the record memory
		for(int i = recDataLength - 1; i >= 0 && frames < recordingData.is.recRegionSizeFrames; i--)
		{
			blockFrames = getNumFrames(recData[lastRecDataPos].blockStart, recData[lastRecDataPos].blockEnd);
			frames += blockFrames;

			numBlocks++;
			if(0 == lastRecDataPos)
				lastRecDataPos = RECORD_DATA_LENGTH - 1;
			else
				lastRecDataPos--;
		}

		//The iteration above may go beyond the number of frames capable of being stored in memory. If so, go back one block
		if(frames > recordingData.is.recRegionSizeFrames)
		{
			frames -= blockFrames;	//total valid frames
			numBlocks--;
		}

		//We've now iterated through all the valid blocks and are now on the first invalid one
		firstBlock = (lastRecDataPos + 1) % RECORD_DATA_LENGTH;

		//Copy the data for valid blocks into the record data structure
		for(UInt32 i = 0; i < numBlocks; i++)
		{
			//RecData rd; rd.blockStart = REC_REGION_START; rd.blockEnd = recordingData.is.recRegionSizeFrames * recordingData.is.frameSizeWords; rd.blockLast = rd.blockEnd; ///////////////////////////////////////////////////////////////////////////MOD FOR TESTING/////////////////////////////////////
			recordingData.recData[i] = recData[(firstBlock + i) % RECORD_DATA_LENGTH];
			recordingData.numRecRegions = numBlocks;
		}
		playFrame = 0;
		recordingData.is = imagerSettings;
		recordingData.totalFrames = frames;
		//recordingData.totalFrames = recordingData.is.recRegionSizeFrames;// frames; ///////////////////////////////////////////////
		recordingData.valid = true;
		recordingData.hasBeenSaved = false;
		ui->setRecLEDFront(false);
		ui->setRecLEDBack(false);
		recording = false;
	}
}

//Returns the number of frames between the two points taking into account rollover at the end of the record region
UInt32 Camera::getNumFrames(UInt32 start, UInt32 end)
{
	Int32 startNum = (start - REC_REGION_START) / recordingData.is.frameSizeWords;
	Int32 endNum = (end - REC_REGION_START) / recordingData.is.frameSizeWords;

	Int32 frames = endNum - startNum + 1;

	if(frames <= 0)
		frames += recordingData.is.recRegionSizeFrames;

	return frames;
}

//Returns the address of the nth frame in a block
UInt32 Camera::getBlockFrameAddress(UInt32 block, UInt32 frame)
{
	UInt32 startFrame = (recordingData.recData[block].blockStart - REC_REGION_START) / recordingData.is.frameSizeWords;
	UInt32 endFrame = (recordingData.recData[block].blockEnd - REC_REGION_START) / recordingData.is.frameSizeWords;
	UInt32 lastFrame = (recordingData.recData[block].blockLast - REC_REGION_START) / recordingData.is.frameSizeWords;
	UInt32 size = getNumFrames(recordingData.recData[block].blockStart, recordingData.recData[block].blockEnd);
	UInt32 position;

	position = lastFrame + 1 + frame; //Advance to the first frame, then by the number of frames requested

	if(startFrame > endFrame)
	{//block rolls over end of record region

		if(position < recordingData.is.recRegionSizeFrames) //If it doesn't roll over the end of the record region
		{
			return position * recordingData.is.frameSizeWords + REC_REGION_START;
		}
		else//It does roll over the end of the record region
		{
			position -= recordingData.is.recRegionSizeFrames;
			if(position <= endFrame)	//If we don't roll over the end of the block
				return position * recordingData.is.frameSizeWords + REC_REGION_START;
			else //we do roll over the end of the block
				return (position - endFrame + startFrame) * recordingData.is.frameSizeWords + REC_REGION_START;

		}
	}
	else //Block does not roll over end of record region
	{
		if(position > endFrame)
		{
			return (position - size) * recordingData.is.frameSizeWords + REC_REGION_START;
		}
		else
		{
			return position * recordingData.is.frameSizeWords + REC_REGION_START;
		}
	}
}

UInt16 Camera::getMaxFPNValue(UInt16 * buf, UInt32 count)
{
	UInt16 maximum = 0;
	for(int i = 0; i < count; i++)
		if(buf[i] > maximum)
			maximum = buf[i];
	return maximum;
}

//Return the address of the nth frame in the entire sequence of valid record blocks
UInt32 Camera::getPlayFrameAddr(UInt32 playFrame)
{
	UInt32 frames = 0;
	UInt32 blockFrames;
	if(playFrame >= recordingData.totalFrames || recordingData.valid == false)
		return 0;

	for(int i = 0; i < recordingData.numRecRegions; i++)
	{
		blockFrames = getNumFrames(recordingData.recData[i].blockStart, recordingData.recData[i].blockEnd);

		if((frames + blockFrames) > playFrame)	//If this block exceeds the play frame, we're in the block containing the play frame
			return getBlockFrameAddress(i, playFrame - frames);
		frames += blockFrames;
	}
	/* Should not get here */
	return 0;
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
		setDisplayFrameAddress(getPlayFrameAddr(playFrame));
		setFocusPeakEnableLL(false);	//Disable focus peaking and zebras
		setZebraEnableLL(false);
	}
	else
	{
		setFocusPeakEnableLL(getFocusPeakEnable());	//Enable focus peaking and zebras if they were enabled previously
		setZebraEnableLL(getZebraEnable());
	}

	setDisplayFrameSource(!playMode);

	return CAMERA_SUCCESS;
}

/* setPlaybackRate
 *
 * Sets the plaback rate for recorded video playback.
 *
 * speed:	positive numbers - Playback rate is 2**(speed - 1) (ie speed = 1 results in normal speed)
 *			negative numbers - Playback rate is one frame every |speed| + 1 frames (ie speed = -1 results in half speed)
 *			0				- Playback is stopped
 * forward:	true plays forward, false plays backwards
 *
 * returns: nothing
 **/
void Camera::setPlaybackRate(Int32 speed, bool forward)
{
	playbackSpeed = speed;
	playbackForward = forward;
}

/* processPlay
 *
 * processes playback, adjusting playframe according to the playback rate set in setPlaybackRate
 *
 * returns: nothing
 **/
void Camera::processPlay(void)
{
	UInt32 divisor, multiple;

	if(playbackSpeed != 0)
	{
		if(playbackSpeed < 0)
			divisor = -playbackSpeed + 1;
		else
			divisor = 1;

		if(playbackSpeed > 0)
			multiple = 1 << (playbackSpeed-1);
		else
			multiple = 1;

		if(0 == playDivisorCount)
		{
			if(playbackForward)
				playFrame = (playFrame + multiple) % recordingData.totalFrames; //Roll over past the end
			else
			{
				if(multiple > playFrame)
					playFrame = playFrame - multiple + recordingData.totalFrames; //If going backwards past the end, take care of rolling over
				else
					playFrame -= multiple; //Not rolling over
			}
			playDivisorCount = divisor - 1;
		}
		else
			playDivisorCount--;
	}
	else
	{
		//take care of encoder
		Int32 encMovementLowRes;
		Int32 encMovement = ui->getEncoderValue(&encMovementLowRes);
		if(ui->getEncoderSwitch())
			encMovement *= 10;
		else
			encMovement = encMovementLowRes;

		if(encMovement >= 0)
			playFrame = (playFrame + encMovement) % recordingData.totalFrames; //Roll over past the end
		else
		{
			encMovement = -encMovement;
			if(encMovement > playFrame)
				playFrame = playFrame - encMovement + recordingData.totalFrames; //If going backwards past the end, take care of rolling over
			else
				playFrame -= encMovement; //Not rolling over
		}
	}
}

/* Camera::writeFrameNumbers
 *
 * Burns frame numbers and frame total into the upper left corner of the image.
 *
 * returns: nothing
 **/
void Camera::writeFrameNumbers(void)
{
	char buf[32];

	for(int i = 0; i < recordingData.totalFrames; i++)
	{
		UInt32 addr = getPlayFrameAddr(i);
		sprintf(buf, "%d/%d", i+1, recordingData.totalFrames);

		gpmc->write32(GPMC_PAGE_OFFSET_ADDR, addr);

		for(int c = 0; buf[c]; c++)	//For each character
		{
			for(int y = 0; y < 8; y++)	//for each line of the character
			{
				for(int x = 0; x < 8; x++)		//for each row in the current line of the character
				{
					if(font8x8[buf[c] * 8 + y] & (1<<(8-x)))
						writePixel(x + 8*c + y*recordingData.is.stride, 0, 0x3FF);
					else
						writePixel(x + 8*c + y*recordingData.is.stride, 0, 0);

				}
			}
		}
	}

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
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

void Camera::computeFPNCorrection()
{
	FILE * fp;
	fp = fopen("fpn.raw", "wb");

	UInt16 * buffer = new UInt16[MAX_STRIDE*MAX_FRAME_SIZE_V];
	UInt32 * rawBuffer32 = new UInt32[MAX_FRAME_LENGTH*BYTES_PER_WORD/4];
	UInt8 * rawBuffer = (UInt8 *)rawBuffer32;

	qDebug() << "FPN Address: " << gpmc->read32(DISPLAY_FPN_ADDRESS_ADDR);

	//Zero buffer
	for(int i = 0; i < MAX_STRIDE*MAX_FRAME_SIZE_V; i++)
	{
		buffer[i] = 0;
	}

	//Sum pixel values across frames
	for(int frame = 0; frame < FPN_AVERAGE_FRAMES; frame++)
	{
		//Get one frame into the raw buffer
		readAcqMem(rawBuffer32, REC_REGION_START + frame * MAX_FRAME_LENGTH, MAX_FRAME_LENGTH*BYTES_PER_WORD);
		/*
		gpmc->write32(GPMC_PAGE_OFFSET_ADDR, REC_REGION_START + frame * MAX_FRAME_LENGTH);
		for(int i = 0; i < MAX_FRAME_LENGTH*BYTES_PER_WORD/4; i++)
		{
			rawBuffer32[i] = gpmc->readRam32(4*i);
		}
*/
		//Retrieve pixels from the raw buffer and sum them
		for(int i = 0; i < MAX_STRIDE*MAX_FRAME_SIZE_V; i++)
		{
			buffer[i] += readPixelBuf12(rawBuffer, i);
		}

#if 0
		gpmc->write32(GPMC_PAGE_OFFSET_ADDR, REC_REGION_START + frame * MAX_FRAME_LENGTH);
		//gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
		for(int i = 0; i < MAX_STRIDE*MAX_FRAME_SIZE_V; i++)
		{
			buffer[i] += readPixel(i, /*(REC_REGION_START + frame * MAX_FRAME_LENGTH) * BYTES_PER_WORD*/ 0);
			if(i < 10)
				qDebug() << readPixel(i, /*(REC_REGION_START + frame * MAX_FRAME_LENGTH) * BYTES_PER_WORD*/ 0);
			else if(i == 10)
				qDebug() << " ";
		}
#endif
	}

	//Divide by number summed to get average and write to FPN area
	for(int i = 0; i < MAX_STRIDE*MAX_FRAME_SIZE_V; i++)
	{
		buffer[i] /= FPN_AVERAGE_FRAMES;
		writePixelBuf12(rawBuffer, i, buffer[i]);
	}

	writeAcqMem(rawBuffer32,  FPN_ADDRESS, MAX_FRAME_LENGTH*BYTES_PER_WORD);
	fwrite(buffer, sizeof(buffer[0]), MAX_STRIDE*MAX_FRAME_SIZE_V, fp);

	fclose(fp);

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
	delete buffer;
	delete rawBuffer;
}

void Camera::computeFPNCorrection2(UInt32 framesToAverage, bool writeToFile, bool factory)
{
	char filename[1000];
	const char *formatStr;

	//Generate the filename for this particular resolution and offset
	if(factory)
		formatStr = "cal/factoryFPN/fpn_%dx%doff%dx%d";
	else
		formatStr = "userFPN/fpn_%dx%doff%dx%d";

	sprintf(filename, formatStr, getImagerSettings().hRes, getImagerSettings().vRes, getImagerSettings().hOffset, getImagerSettings().vOffset);

	std::string fn = sensor->getFilename(filename, ".raw");

	FILE * fp;
	if(writeToFile)
	{
		qDebug() << "Writing FPN to file" << fn.c_str();
		fp = fopen(fn.c_str(), "wb");
		if(fp == NULL)
		{
			qDebug() << "Error: File couldn't be opened";
			return;
		}
	}

	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;
	UInt32 bytesPerFrame = recordingData.is.frameSizeWords * BYTES_PER_WORD;

	UInt16 * buffer = new UInt16[pixelsPerFrame];
	UInt32 * rawBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawBuffer = (UInt8 *)rawBuffer32;


	//Zero buffer
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		buffer[i] = 0;
	}

	//Sum pixel values across frames
	for(int frame = 0; frame < framesToAverage; frame++)
	{
		//Get one frame into the raw buffer
		readAcqMem(rawBuffer32,
				   REC_REGION_START + (frame + 1) * recordingData.is.frameSizeWords,
				   bytesPerFrame);

		//Retrieve pixels from the raw buffer and sum them
		for(int i = 0; i < pixelsPerFrame; i++)
		{
			buffer[i] += readPixelBuf12(rawBuffer, i);
		}
	}

	//Divide by number summed to get average and write to FPN area
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		buffer[i] /= framesToAverage;
		writePixelBuf12(rawBuffer, i, buffer[i]);
	}

	writeAcqMem(rawBuffer32, FPN_ADDRESS, bytesPerFrame);

	imgGain = 4096.0 / (double)(4096 - getMaxFPNValue(buffer, pixelsPerFrame)) * IMAGE_GAIN_FUDGE_FACTOR;
	qDebug() << "imgGain set to" << imgGain;
	setCCMatrix(wbMat);

	qDebug() << "About to write file...";
	if(writeToFile)
	{
		fwrite(buffer, sizeof(buffer[0]), pixelsPerFrame, fp);
		fclose(fp);
	}

	delete buffer;
	delete rawBuffer;
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
		sensor->setIntegrationTime(0, imagerSettings.hRes, imagerSettings.vRes);
		//nanosleep(&ts, NULL);
	}

	UInt32 retVal;

	qDebug() << "Starting record with a length of" << framesToAverage << "frames";

	retVal = setRecSequencerModeSingleBlock(framesToAverage+1);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = startRecording();
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	for(count = 0; (count < countMax) && recording; count++) {nanosleep(&ts, NULL);}

	//Return to set exposure
	if(noCap)
		sensor->setIntegrationTime((double)imagerSettings.exposure/100000000.0, imagerSettings.hRes, imagerSettings.vRes);

	if(count == countMax)	//If after the timeout recording hasn't finished
	{
		qDebug() << "Error: Record failed to stop within timeout period. recDataLength =" << recDataLength;

		retVal = stopRecording();
		if(CAMERA_SUCCESS != retVal)
			qDebug() << "Error: Stop Record failed";

		return CAMERA_FPN_CORRECTION_ERROR;
	}

	qDebug() << "Record done, doing normal FPN correction";

	computeFPNCorrection2(framesToAverage, writeToFile, factory);

	qDebug() << "FPN correction done";
	recordingData.hasBeenSaved = true;

	return CAMERA_SUCCESS;

}

Int32 Camera::loadFPNFromFile(const char * filename)
{
	char flname[1000];
	std::string fn;
	//Generate the filename for this particular resolution and offset
	sprintf(flname, "userFPN/fpn_%dx%doff%dx%d", getImagerSettings().hRes, getImagerSettings().vRes, getImagerSettings().hOffset, getImagerSettings().vOffset);

	fn = sensor->getFilename(flname, ".raw");

	qDebug() << "Trying to load user FPN from file" << fn.c_str();

	//Check that the file exists, if not, try reading in from the factory cal folder
	if( access( fn.c_str(), R_OK ) == -1 )
	{
		//Generate the filename for this particular resolution and offset
		sprintf(flname, "cal/factoryFPN/fpn_%dx%doff%dx%d", getImagerSettings().hRes, getImagerSettings().vRes, getImagerSettings().hOffset, getImagerSettings().vOffset);

		fn = sensor->getFilename(flname, ".raw");

		qDebug() << "Trying to load factory FPN from file" << fn.c_str();

		//If the FPN file exists, read it in
		if( access( fn.c_str(), R_OK ) == -1 )
			return CAMERA_FILE_NOT_FOUND;
	}

	qDebug() << "Found FPN file" << fn.c_str();

	FILE * fp;
	fp = fopen(fn.c_str(), "rb");
	if(!fp)
		return CAMERA_FILE_ERROR;

	UInt32 pixelsPerFrame = imagerSettings.stride * imagerSettings.vRes;
	UInt32 bytesPerFrame = imagerSettings.frameSizeWords * BYTES_PER_WORD;

	UInt16 * buffer = new UInt16[pixelsPerFrame];
	UInt32 * packedBuf32 = new UInt32[bytesPerFrame / 4];
	UInt8 * packedBuf = (UInt8 *)packedBuf32;

	//Read in the active region of the FPN data file.
	fread(buffer, sizeof(buffer[0]), imagerSettings.stride*imagerSettings.vRes, fp);

	fclose(fp);
	UInt32 mx = getMaxFPNValue(buffer, pixelsPerFrame);
	imgGain = 4096.0 / (double)(4096 - mx) * IMAGE_GAIN_FUDGE_FACTOR;
	qDebug() << "imgGain set to" << imgGain << "Max FPN value found" << mx;
	setCCMatrix(wbMat);

	//zero the buffer
	memset(packedBuf, 0, bytesPerFrame);

	//Generate packed buffer
	for(UInt32 i = 0; i < pixelsPerFrame; i++)
	{
		writePixelBuf12(packedBuf, i, buffer[i]);
	}

	//Write packed buffer to RAM
	writeAcqMem(packedBuf32, FPN_ADDRESS, bytesPerFrame);

	delete buffer;
	delete packedBuf32;

	return CAMERA_SUCCESS;
}

Int32 Camera::computeColGainCorrection(UInt32 framesToAverage, bool writeToFile)
{

	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;
	UInt32 bytesPerFrame = recordingData.is.frameSizeWords * BYTES_PER_WORD;

	UInt16 * buffer = new UInt16[pixelsPerFrame];
	UInt16 * fpnBuffer = new UInt16[pixelsPerFrame];
	UInt32 * rawBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawBuffer = (UInt8 *)rawBuffer32;
	const char * filename;

	recordFrames(1);

	if(recordingData.is.gain >= LUX1310_GAIN_4)
		filename = "cal/dcgH.bin";
	else
		filename = "cal/dcgL.bin";

	FILE * fp;
	if(writeToFile)
		fp = fopen(filename, "wb");

	//Zero buffer
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		buffer[i] = 0;
	}

	//Read the FPN frame into a buffer
	readAcqMem(rawBuffer32,
			   FPN_ADDRESS,
			   bytesPerFrame);

	//Retrieve pixels from the raw buffer and sum them
	for(int i = 0; i < pixelsPerFrame; i++)
	{
		fpnBuffer[i] = readPixelBuf12(rawBuffer, i);
	}

	//Sum pixel values across frames
	for(int frame = 0; frame < framesToAverage; frame++)
	{
		//Get one frame into the raw buffer
		readAcqMem(rawBuffer32,
				   REC_REGION_START + frame * recordingData.is.frameSizeWords,
				   bytesPerFrame);

		//Retrieve pixels from the raw buffer and sum them
		for(int i = 0; i < pixelsPerFrame; i++)
		{
			buffer[i] += readPixelBuf12(rawBuffer, i) - fpnBuffer[i];
		}
	}

	double valueSum[16] = {0.0};
	double gainCorrection[16];

	if(isColor)
	{
		//Divide by number summed to get average and write to FPN area
		for(int i = 0; i < recordingData.is.stride; i++)
		{
			bool isGreen = sensor->getFilterColor(i, recordingData.is.vRes/2) == FILTER_COLOR_GREEN;
			UInt32 y = recordingData.is.vRes/2 + (isGreen ? 0 : 1);	//Zig sag to we only look at green pixels
			valueSum[i % 16] += (double)buffer[i+recordingData.is.stride*y] / (double)framesToAverage;
		}
	}
	else
	{
		//Divide by number summed to get average and write to FPN area
		for(int i = 0; i < recordingData.is.stride; i++)
		{
			valueSum[i % 16] += (double)buffer[i+recordingData.is.stride*recordingData.is.vRes/2] / (double)framesToAverage;
		}
	}

	double minVal = 0;
	for(int i = 0; i < 16; i++)
	{
		//divide by number of values summed to get pixel value
		valueSum[i] /= ((double)recordingData.is.stride/16.0);

		//Find min value
		if(valueSum[i] > minVal)
			minVal = valueSum[i];
	}

	qDebug() << "Gain correction values:";
	for(int i = 0; i < 16; i++)
	{
		gainCorrection[i] =  minVal / valueSum[i];


		qDebug() << gainCorrection[i];

	}

	//Check that values are within a sane range
	for(int i = 0; i < 16; i++)
	{
		if(gainCorrection[i] < LUX1310_GAIN_CORRECTION_MIN || gainCorrection[i] > LUX1310_GAIN_CORRECTION_MAX)
			return CAMERA_GAIN_CORRECTION_ERROR;
	}

	if(writeToFile)
	{
		fwrite(gainCorrection, sizeof(gainCorrection[0]), LUX1310_HRES_INCREMENT, fp);
		fclose(fp);
	}

	for(int i = 0; i < recordingData.is.stride; i++)
		gpmc->write16(DCG_MEM_START_ADDR+2*i, gainCorrection[i % 16]*4096.0);

	delete buffer;
	delete rawBuffer;
	return CAMERA_SUCCESS;
}

Int32 Camera::loadColGainFromFile(const char * filename)
{
	double gainCorrection[16];
	size_t count = 0;
	//If the column gain file exists, read it in
	if( access( filename, R_OK ) != -1 )
	{
		FILE * fp;
		fp = fopen(filename, "rb");
		if(fp)
		{
			count = fread(gainCorrection, sizeof(gainCorrection[0]), LUX1310_HRES_INCREMENT, fp);
			fclose(fp);
		}
	}

	//If the file wasn't fully read in, set all gain corrections to 1.0
	if(LUX1310_HRES_INCREMENT != count)
	{
		for(int i = 0; i < 16; i++)
		{
			gainCorrection[i] = 1.0;
		}
	}
	else
	{
		bool gcOutOfSaneRange = false;
		//If all read in gain corrections are not within a sane range, set them all to 1.0
		for(int i = 0; i < 16; i++)
		{
			if(gainCorrection[i] < 0.5 || gainCorrection[i] > 2.0)
				gcOutOfSaneRange = true;
		}

		if(gcOutOfSaneRange)
		{
			for(int i = 0; i < 16; i++)
			{
				gainCorrection[i] = 1.0;
			}
		}
	}

	//Write the values into the display column gain memory
	for(int i = 0; i < LUX1310_MAX_H_RES; i++)
		writeDGCMem(gainCorrection[i % 16], i);

	return CAMERA_SUCCESS;
}

UInt32 Camera::adcOffsetCorrection(UInt32 iterations, const char * filename)
{
	filename = "a";

	//Zero the offsets
	for(int i = 0; i < LUX1310_HRES_INCREMENT; i++)
		sensor->setADCOffset(i, 0);

	UInt32 retVal;
	for(int i = 0; i < iterations; i++)
	{
		qDebug() << "Starting record for autoOffsetCorrection";

		retVal = recordFrames(1);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		qDebug() << "Record done, doing offset correction iteration";

		offsetCorrectionIteration(REC_REGION_START);
	}
	qDebug() << "Offset correction done";

	if(strlen(filename))
	{
		qDebug() << "Saving to file" << filename;
		sensor->saveADCOffsetsToFile(filename);
	}

	return CAMERA_SUCCESS;
}

void Camera::offsetCorrectionIteration(UInt32 wordAddress)
{
	Int32 buffer[16];
	//static Int16 offsets[16] = {0};
	Int16 * offsets = sensor->offsetsA;
	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, wordAddress);

	//Set to max value prior to finding the minimum
	for(int i = 0; i < 16; i++)
		buffer[i] = 0x7FFFFFFF;

	for(int i = 0; i < recordingData.is.hRes; i++)
	{
		UInt16 pix = readPixel12(i, 0);

		buffer[i % 16] = min(pix, buffer[i % 16]);

	}
	qDebug() << "Min values"
			 << buffer[0] << buffer[1] << buffer[2] << buffer[3]
			 << buffer[4] << buffer[5] << buffer[6] << buffer[7]
			 << buffer[8] << buffer[9] << buffer[10] << buffer[11]
			 << buffer[12] << buffer[13] << buffer[14] << buffer[15];
	qDebug() << "Offsets"
			 << offsets[0] << offsets[1] << offsets[2] << offsets[3]
			 << offsets[4] << offsets[5] << offsets[6] << offsets[7]
			 << offsets[8] << offsets[9] << offsets[10] << offsets[11]
			 << offsets[12] << offsets[12] << offsets[13] << offsets[14];

	for(int i = 0; i < 16; i++)
	{
		//Int16 val = sensor->getADCOffset(i);
		offsets[i] = offsets[i] - 0.5*(buffer[i]-30);
		offsets[i] = within(offsets[i], -1023, 1023);
		sensor->setADCOffset(i, offsets[i]);
	}

	gpmc->write32(GPMC_PAGE_OFFSET_ADDR, 0);
}

Int32 Camera::autoAdcOffsetCorrection(void)
{
	ImagerSettings_t _is;
	Int32 retVal;
	UInt32 gain;
	int FRAME_SIZE = 1280*1024*12/8;

	_is.hRes = 1280;		//pixels
	_is.vRes = 1024;		//pixels
	_is.stride = _is.hRes;		//Number of pixels per line (allows for dark pixels in the last column)
	_is.hOffset = 0;		//active area offset from left
	_is.vOffset = 0;		//Active area offset from top
	_is.exposure = 100000;	//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments
	_is.gain = LUX1310_GAIN_1;

	retVal = setImagerSettings(_is);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	//Zero out the FPN area
	for(int i = 0; i < FRAME_SIZE; i = i + 4)
	{
		gpmc->writeRam32(i, 0);
	}

	//Do correction for 39 clock wavetable
	//sensor->setWavetable(LUX1310_WAVETABLE_39);
	for(gain = LUX1310_GAIN_1; gain <= LUX1310_GAIN_16; gain++)
	{
		qDebug() << "Doing correction for WT39 Gain" << gain;
		_is.gain = gain;
		sensor->wavetableSelect = LUX1310_WAVETABLE_39;
		retVal = setImagerSettings(_is);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		retVal = adcOffsetCorrection(32);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		qDebug() << "Done";
	}

	//Do correction for 80 clock wavetable
	//sensor->setWavetable(LUX1310_WAVETABLE_80);
	for(gain = LUX1310_GAIN_1; gain <= LUX1310_GAIN_16; gain++)
	{
		qDebug() << "Doing correction for WT80 Gain" << gain;
		_is.gain = gain;
		sensor->wavetableSelect = LUX1310_WAVETABLE_80;

		retVal = setImagerSettings(_is);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		retVal = adcOffsetCorrection(32);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		qDebug() << "Done";
	}

	sensor->wavetableSelect = LUX1310_WAVETABLE_AUTO;

	_is.gain = LUX1310_GAIN_1;

	retVal = setImagerSettings(_is);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	return CAMERA_SUCCESS;
}

Int32 Camera::autoColGainCorrection(void)
{
	Int32 retVal;
	ImagerSettings_t _is;

	_is.hRes = 1280;		//pixels
	_is.vRes = 1024;		//pixels
	_is.stride = _is.hRes;		//Number of pixels per line (allows for dark pixels in the last column)
	_is.hOffset = 0;		//active area offset from left
	_is.vOffset = 0;		//Active area offset from top
	_is.exposure = 400000;	//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments
	_is.gain = LUX1310_GAIN_1;

	retVal = setImagerSettings(_is);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = adjustExposureToValue(3584, 100, false);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	computeColGainCorrection(1, true);

	_is.gain = LUX1310_GAIN_4;
	retVal = setImagerSettings(_is);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = adjustExposureToValue(3584, 100, false);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	computeColGainCorrection(1, true);

	return CAMERA_SUCCESS;
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
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		val = getMiddlePixelValue(includeFPNCorrection);
		qDebug() << "Got val of" << val;

		//If the pixel value is above the limit, halve the exposure
		if(val > level)
		{
			qDebug() << "Reducing exposure";
			_is.exposure /= 2;
			retVal = setImagerSettings(_is);
			if(CAMERA_SUCCESS != retVal)
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
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		//Check the value was correct
		retVal = recordFrames(1);
		if(CAMERA_SUCCESS != retVal)
			return retVal;

		val = getMiddlePixelValue(includeFPNCorrection);
		iterationCount++;
	} while(abs(val - level) > tolerance && iterationCount <= iterationCountMax);
	qDebug() << "Final exposure set to" << ((double)_is.exposure / 100.0) << "us, final pixel value" << val << "expected pixel value" << level;

	if(iterationCount > iterationCountMax)
		return CAMERA_ITERATION_LIMIT_EXCEEDED;

	return CAMERA_SUCCESS;
}

Int32 Camera::recordFrames(UInt32 numframes)
{
	Int32 retVal;
	UInt32 count;
	const int countMax = 500;
	int ms = 1;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };

	qDebug() << "Starting record of one frame";

	retVal = setRecSequencerModeSingleBlock(numframes + 1);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = startRecording();
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	for(count = 0; (count < countMax) && recording; count++) {nanosleep(&ts, NULL);}

	if(count == countMax)	//If after the timeout recording hasn't finished
	{
		qDebug() << "Error: Record failed to stop within timeout period. recDataLength =" << recDataLength;

		retVal = stopRecording();
		if(CAMERA_SUCCESS != retVal)
			qDebug() << "Error: Stop Record failed";

		return CAMERA_RECORD_FRAME_ERROR;
	}

	qDebug() << "Record done";

	return CAMERA_SUCCESS;
}

UInt32 Camera::getMiddlePixelValue(bool includeFPNCorrection)
{
	//gpmc->write32(GPMC_PAGE_OFFSET_ADDR, REC_REGION_START);	//Read from the beginning of the record region

	UInt32 quadStartX = getImagerSettings().hRes / 2 & 0xFFFFFFFE;
	UInt32 quadStartY = getImagerSettings().vRes / 2 & 0xFFFFFFFE;
	//3=G B
	//  R G

	UInt16 bRaw = readPixel12((quadStartY + 1) * imagerSettings.stride + quadStartX, REC_REGION_START * BYTES_PER_WORD);
	UInt16 gRaw = readPixel12(quadStartY * imagerSettings.stride + quadStartX, REC_REGION_START * BYTES_PER_WORD);
	UInt16 rRaw = readPixel12(quadStartY * imagerSettings.stride + quadStartX + 1, REC_REGION_START * BYTES_PER_WORD);

	if(includeFPNCorrection)
	{
		bRaw -= readPixel12((quadStartY + 1) * imagerSettings.stride + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
		gRaw -= readPixel12(quadStartY * imagerSettings.stride + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
		rRaw -= readPixel12(quadStartY * imagerSettings.stride + quadStartX + 1, FPN_ADDRESS * BYTES_PER_WORD);

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

	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;
	UInt32 bytesPerFrame = recordingData.is.frameSizeWords * BYTES_PER_WORD;

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
	readAcqMem(rawFrameBuffer32,
			   REC_REGION_START + frame * recordingData.is.frameSizeWords,
			   bytesPerFrame);

	//Get one frame into the raw buffer
	readAcqMem(fpnBuffer32,
			   FPN_ADDRESS,
			   bytesPerFrame);

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

	return CAMERA_SUCCESS;
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::getRawCorrectedFrame(UInt32 frame, UInt16 * frameBuffer)
{
	Int32 retVal;
	double gainCorrection[16];

	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;


	retVal = readDCG(gainCorrection);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	UInt16 * fpnUnpacked = new UInt16[pixelsPerFrame];
	if(NULL == fpnUnpacked)
		return CAMERA_MEM_ERROR;

	retVal = readFPN(fpnUnpacked);
	if(CAMERA_SUCCESS != retVal)
	{
		delete fpnUnpacked;
		return retVal;
	}

	retVal = readCorrectedFrame(frame, frameBuffer, fpnUnpacked, gainCorrection);
	if(CAMERA_SUCCESS != retVal)
	{
		delete fpnUnpacked;
		return retVal;
	}

	delete fpnUnpacked;

	return CAMERA_SUCCESS;
}

//frameBuffer must be a UInt16 array with enough elements to to hold all pixels at the current recording resolution
Int32 Camera::getRawCorrectedFramesAveraged(UInt32 frame, UInt32 framesToAverage, UInt16 * frameBuffer)
{
	Int32 retVal;
	double gainCorrection[16];

	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;


	retVal = readDCG(gainCorrection);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	UInt16 * fpnUnpacked = new UInt16[pixelsPerFrame];
	if(NULL == fpnUnpacked)
		return CAMERA_MEM_ERROR;

	retVal = readFPN(fpnUnpacked);
	if(CAMERA_SUCCESS != retVal)
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
		if(CAMERA_SUCCESS != retVal)
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

	return CAMERA_SUCCESS;
}

Int32 Camera::readDCG(double * gainCorrection)
{
	const char * filename;

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

	//If the file wasn't fully read in, fail
	if(LUX1310_HRES_INCREMENT != count)
		return CAMERA_FILE_ERROR;

	return CAMERA_SUCCESS;
}

Int32 Camera::readFPN(UInt16 * fpnUnpacked)
{
	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;
	UInt32 bytesPerFrame = recordingData.is.frameSizeWords * BYTES_PER_WORD;
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

	return CAMERA_SUCCESS;
}


Int32 Camera::readCorrectedFrame(UInt32 frame, UInt16 * frameBuffer, UInt16 * fpnInput, double * gainCorrection)
{
	UInt32 pixelsPerFrame = recordingData.is.stride * recordingData.is.vRes;
	UInt32 bytesPerFrame = recordingData.is.frameSizeWords * BYTES_PER_WORD;

	UInt32 * rawFrameBuffer32 = new UInt32[bytesPerFrame / 4];
	UInt8 * rawFrameBuffer = (UInt8 *)rawFrameBuffer32;

	if(NULL == rawFrameBuffer32)
		return CAMERA_MEM_ERROR;

	//Read in the frame and FPN data
	//Get one frame into the raw buffer
	readAcqMem(rawFrameBuffer32,
			   REC_REGION_START + frame * recordingData.is.frameSizeWords,
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

	return CAMERA_SUCCESS;
}


void setADCOffset(UInt8 channel, Int16 offset);
Int16 getADCOffset(UInt8 channel);

Int32 Camera::startSave(UInt32 startFrame, UInt32 length)
{
	Int32 retVal;

	if(startFrame + length > recordingData.totalFrames)
		return CAMERA_INVALID_SETTINGS;

	vinst->setRunning(false);
	delayms(100); //Gstreamer crashes with no delay here, OMX needs time to deinit stuff?
	setDisplaySettings(true);	//Set to encoder safe mode (width multiple of 16)

	//Set the start frame. This will advance by one before recording starts.
	if(0 == startFrame)
		playFrame = recordingData.totalFrames-1;	//Setting it to this will cause playback to advance to the first frame on record
	else
		playFrame = startFrame - 1;		//Will advance 1 frame before start of record

	setDisplaySyncInhibit(true);

	retVal = recorder->start((recordingData.is.hRes + 15) & 0xFFFFFFF0, recordingData.is.vRes, length+2);

	if(retVal != CAMERA_SUCCESS)
	{
		setDisplaySyncInhibit(false);
		vinst->setRunning(true);
		if(RECORD_DIRECTORY_NOT_WRITABLE == retVal)
			return RECORD_DIRECTORY_NOT_WRITABLE;
		else if(RECORD_FILE_EXISTS == retVal)
			return RECORD_FILE_EXISTS;
		else
			return RECORD_ERROR;
	}

	delayms(100);	//A few frames are skipped without this delay

	sem_wait(&playMutex);
	setDisplaySyncInhibit(false);
	setPlaybackRate(1, true);
	sem_post(&playMutex);

	fflush(stdout);

	recordingData.hasBeenSaved = true;

	return CAMERA_SUCCESS;
}

#define COLOR_MATRIX_MAXVAL	((1 << SENSOR_DATA_WIDTH) * (1 << COLOR_MATRIX_INT_BITS))

void Camera::setCCMatrix(double * wbMat)
{
	gpmc->write16(CCM_11_ADDR, within((int)(4096.0 * ccMatrix[0] * wbMatrix[0] * imgGain * wbMat[0]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_12_ADDR, within((int)(4096.0 * ccMatrix[1] * wbMatrix[0] * imgGain * wbMat[0]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_13_ADDR, within((int)(4096.0 * ccMatrix[2] * wbMatrix[0] * imgGain * wbMat[0]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));

	gpmc->write16(CCM_21_ADDR, within((int)(4096.0 * ccMatrix[3] * wbMatrix[1] * imgGain * wbMat[1]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_22_ADDR, within((int)(4096.0 * ccMatrix[4] * wbMatrix[1] * imgGain * wbMat[1]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_23_ADDR, within((int)(4096.0 * ccMatrix[5] * wbMatrix[1] * imgGain * wbMat[1]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));

	gpmc->write16(CCM_31_ADDR, within((int)(4096.0 * ccMatrix[6] * wbMatrix[2] * imgGain * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_32_ADDR, within((int)(4096.0 * ccMatrix[7] * wbMatrix[2] * imgGain * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));
	gpmc->write16(CCM_33_ADDR, within((int)(4096.0 * ccMatrix[8] * wbMatrix[2] * imgGain * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1));

	qDebug() << "Blue matrix" << within((int)(4096.0 * ccMatrix[6] * wbMatrix[2] * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1)
			<< within((int)(4096.0 * ccMatrix[7] * wbMatrix[2] * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1)
			<< within((int)(4096.0 * ccMatrix[8] * wbMatrix[2] * wbMat[2]), -COLOR_MATRIX_MAXVAL, COLOR_MATRIX_MAXVAL-1);
}

Int32 Camera::setWhiteBalance(UInt32 x, UInt32 y)
{
	UInt32 quadStartX = x & 0xFFFFFFFE;
	UInt32 quadStartY = y & 0xFFFFFFFE;
	//3=G B
	//  R G

	int bRaw = readPixel12((quadStartY + 1) * imagerSettings.stride + quadStartX, LIVE_FRAME_0_ADDRESS * BYTES_PER_WORD);
	int gRaw = readPixel12(quadStartY * imagerSettings.stride + quadStartX, LIVE_FRAME_0_ADDRESS * BYTES_PER_WORD);
	int rRaw = readPixel12(quadStartY * imagerSettings.stride + quadStartX + 1, LIVE_FRAME_0_ADDRESS * BYTES_PER_WORD);

	//Read pixels from first live display buffer and subtract FPN data
	double b = bRaw -
				readPixel12((quadStartY + 1) * imagerSettings.stride + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
	double g = gRaw -
				readPixel12(quadStartY * imagerSettings.stride + quadStartX, FPN_ADDRESS * BYTES_PER_WORD);
	double r =  rRaw-
				readPixel12(quadStartY * imagerSettings.stride + quadStartX + 1, FPN_ADDRESS * BYTES_PER_WORD);
	qDebug() << "RGB values read:" << r << g << b;
	//Perform color correction
	double rc =		within(
			r * ccMatrix[0] * wbMatrix[0] +
			g * ccMatrix[1] * wbMatrix[0] +
			b * ccMatrix[2] * wbMatrix[0],
			0.0, 4095.0);

	double gc =		within(
			r * ccMatrix[3] * wbMatrix[1] +
			g * ccMatrix[4] * wbMatrix[1] +
			b * ccMatrix[5] * wbMatrix[1],
			0.0, 4095.0);

	double bc =		within(
			r * ccMatrix[6] * wbMatrix[2] +
			g * ccMatrix[7] * wbMatrix[2] +
			b * ccMatrix[8] * wbMatrix[2],
			0.0, 4095.0);
	qDebug() << "Corrected values:" << rc << gc << bc;

	//Fail if the pixel values is clipped or too low
	if(rRaw == 4095 || gRaw == 4095 || bRaw == 4095 || rc >= 4094.0 || gc >= 4094.0 || bc >= 4094.0)
		return CAMERA_CLIPPED_ERROR;

	if(r < 384 || g < 384 || b < 384)
		return CAMERA_LOW_SIGNAL_ERROR;


	//Find the max value, generate white balance matrix that scales the other colors up to match the brightest color
	double mx = max(rc, max(gc, bc));

	wbMat[0] = (double)mx / (double)rc;
	wbMat[1] = (double)mx / (double)gc;
	wbMat[2] = (double)mx / (double)bc;

	qDebug() << "Setting WB matrix to " << wbMat[0] << wbMat[1] << wbMat[2];

	setCCMatrix(wbMat);
	return CAMERA_SUCCESS;

}

void Camera::setFocusAid(bool enable)
{
	UInt32 startX, startY, cropX, cropY;

	if(enable)
	{
		//Set crop window to native resolution of LCD or unchanged if we're scaling up
		if(imagerSettings.hRes > 600 || imagerSettings.vRes > 480)
		{
			//Depending on aspect ratio, set the display window appropriately
			if((imagerSettings.vRes * MAX_FRAME_SIZE_H) > (imagerSettings.hRes * MAX_FRAME_SIZE_V))	//If it's taller than the display aspect
			{
				cropY = 480;
				cropX = cropY * imagerSettings.hRes / imagerSettings.vRes;
				if(cropX & 1)	//If it's odd, round it up to be even
					cropX++;
				startX = (imagerSettings.hRes - cropX) / 2;
				startY = (imagerSettings.vRes - cropY) / 2;

			}
			else
			{
				cropX = 600;
				cropY = cropX * imagerSettings.vRes / imagerSettings.hRes;
				if(cropY & 1)	//If it's odd, round it up to be even
					cropY++;
				startX = (imagerSettings.hRes - cropX) / 2;
				startY = (imagerSettings.vRes - cropY) / 2;

			}
			qDebug() << "Setting startX" << startX << "startY" << startY << "cropX" << cropX << "cropY" << cropY;
			vinst->setScaling(startX & 0xFFFF8, startY, cropX, cropY);	//StartX must be a multiple of 8
		}

	}
	else
	{
		vinst->setScaling(0, 0, imagerSettings.hRes, imagerSettings.vRes);
	}
}

bool Camera::getFocusAid()
{
	/* FIXME: Not implemented */
	return false;
}

Int32 Camera::blackCalAllStdRes(bool factory)
{
	ImagerSettings_t settings;
	Int32 retVal;
	//Populate the common resolution combo box from the list of resolutions
	FILE * fp;
	char line[30];

	vinst->setRunning(false);

	fp = fopen("resolutions", "r");
	if (fp == NULL)
		return CAMERA_FILE_ERROR;

	int g;

	//For each gain
	for(g = LUX1310_GAIN_1; g <= LUX1310_GAIN_16; g++)
	{
		retVal = fseek(fp, 0, SEEK_SET);
		if (retVal != 0) {
			return CAMERA_FILE_ERROR;
		}
		while(fgets(line, 30, fp) != NULL)
		{	//For each resolution in the resolution list
			//Get the resolution and compute the maximum frame rate to be appended after the resolution
			int hRes, vRes;

			sscanf(line, "%dx%d", &hRes, &vRes);

			settings.hRes = hRes;		//pixels
			settings.vRes = vRes;		//pixels
			settings.stride = hRes;		//Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
			settings.hOffset = (sensor->getMaxHRes() - hRes) / 2 & 0xFFFFFFFE;	//Active area offset from left
			settings.vOffset = (sensor->getMaxVRes() - vRes) / 2 & 0xFFFFFFFE;		//Active area offset from top
			settings.gain = g;
			settings.period = sensor->getMinFramePeriod(hRes, vRes);
			settings.exposure = sensor->getMaxExposure(settings.period);

			retVal = setImagerSettings(settings);
			if(CAMERA_SUCCESS != retVal)
				return retVal;

			qDebug() << "Doing FPN correction for " << hRes << "x" << vRes << "...";
			retVal = autoFPNCorrection(16, true, false, factory);	//Factory mode
			if(CAMERA_SUCCESS != retVal)
				return retVal;

			qDebug() << "Done.";
		}
	}


	fclose(fp);

	settings.hRes = LUX1310_MAX_H_RES;		//pixels
	settings.vRes = LUX1310_MAX_V_RES;		//pixels
	settings.stride = LUX1310_MAX_H_RES;		//Number of pixels per line (allows for dark pixels in the last column), always multiple of 16
	settings.hOffset = 0;	//Active area offset from left
	settings.vOffset = 0;		//Active area offset from top
	settings.gain = LUX1310_GAIN_1;
	settings.period = sensor->getMinFramePeriod(LUX1310_MAX_H_RES, LUX1310_MAX_V_RES);
	settings.exposure = sensor->getMaxExposure(settings.period);

	retVal = setImagerSettings(settings);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = setDisplaySettings(false);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	retVal = loadFPNFromFile(FPN_FILENAME);
	if(CAMERA_SUCCESS != retVal)
		return retVal;

	vinst->setRunning(true);

	return CAMERA_SUCCESS;
}




Int32 Camera::takeWhiteReferences(void)
{
	Int32 retVal;
	ImagerSettings_t _is;
	UInt32 g;
	FILE * fp;
	char filename[1000];
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

	_is.hRes = 1280;		//pixels
	_is.vRes = 1024;		//pixels
	_is.stride = _is.hRes;		//Number of pixels per line (allows for dark pixels in the last column)
	_is.hOffset = 0;		//active area offset from left
	_is.vOffset = 0;		//Active area offset from top
	_is.exposure = 400000;	//10ns increments
	_is.period = 500000;		//Frame period in 10ns increments

	UInt32 pixelsPerFrame = _is.stride * _is.vRes;

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
		if(CAMERA_SUCCESS != retVal)
		{
			delete frameBuffer;
			return retVal;
		}

		retVal = adjustExposureToValue(4096*3/4);
		if(CAMERA_SUCCESS != retVal)
		{
			delete frameBuffer;
			return retVal;
		}

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
			if(CAMERA_SUCCESS != retVal)
			{
				delete frameBuffer;
				return retVal;
			}

			qDebug() << "Recording frames for gain" << gName << "exposure" << i;
			//Record frames
			retVal = recordFrames(16);
			if(CAMERA_SUCCESS != retVal)
			{
				delete frameBuffer;
				return retVal;
			}

			//Get the frames averaged and save to file
			qDebug() << "Doing getRawCorrectedFramesAveraged()";
			retVal = getRawCorrectedFramesAveraged(0, 16, frameBuffer);
			if(CAMERA_SUCCESS != retVal)
			{
				delete frameBuffer;
				return retVal;
			}


			//Generate the filename for this particular resolution and offset
			sprintf(filename, "userFPN/wref_%s_LV%d.raw", gName, i+1);

			fp = fopen(filename, "wb");
			if(NULL == fp)
			{
				delete frameBuffer;
				return CAMERA_FILE_ERROR;
			}
			fwrite(frameBuffer, sizeof(frameBuffer[0]), pixelsPerFrame, fp);
			fclose(fp);
		}
	}

	delete frameBuffer;
	return CAMERA_SUCCESS;
}

bool Camera::getFocusPeakEnable(void)
{
	return focusPeakEnabled;
}
void Camera::setFocusPeakEnable(bool en)
{
	setFocusPeakEnableLL(en);
	focusPeakEnabled = en;
}
bool Camera::getZebraEnable(void)
{
	return zebraEnabled;
}
void Camera::setZebraEnable(bool en)
{
	setZebraEnableLL(en);
	zebraEnabled = en;
}

void* recDataThread(void *arg)
{
	Camera * cInst = (Camera *)arg;
	int ms = 100;
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	bool recording;

	//Dummy
	//while(cInst->getRecording());

	while(!cInst->terminateRecDataThread)
	{
		if(!cInst->getRecDataFifoIsEmpty())
		{
			cInst->recData[cInst->recDataPos].blockStart = cInst->readRecDataFifo();
			cInst->recData[cInst->recDataPos].blockEnd = cInst->readRecDataFifo();
			cInst->recData[cInst->recDataPos].blockLast = cInst->readRecDataFifo();
			qDebug() << "Read something from recDataFifo";

			//Keep track of number of records
			if(cInst->recDataLength < RECORD_DATA_LENGTH)
				cInst->recDataLength++;

			//Track which record is the latest
			cInst->recDataPos++;
			if(cInst->recDataPos >= RECORD_DATA_LENGTH)
				cInst->recDataPos = 0;
		}

		//On the falling edge of recording, call the user callback
		recording = cInst->getRecording();

		if(!recording && (cInst->lastRecording || cInst->recording))	//Take care of situtation where recording goes low->high-low between two interrutps by checking the cInst->recording flag
		{
			recording = false;
			cInst->endOfRec();

			if(cInst->endOfRecCallback)
				(*cInst->endOfRecCallback)(cInst->endOfRecCallbackArg);	//Call user callback

		}
		cInst->lastRecording = recording;


		nanosleep(&ts, NULL);
	}

	pthread_exit(NULL);
}

void frameCallback(void * arg)
{
	Camera * cInst = (Camera *)arg;
	if(cInst->playbackMode)
	{
		sem_wait(&cInst->playMutex);

		cInst->processPlay();

		cInst->setDisplayFrameAddress(cInst->getPlayFrameAddr(cInst->playFrame));

		sem_post(&cInst->playMutex);
	}
}

void recordEosCallback(void * arg)
{
	Camera * camera = (Camera *)arg;
	camera->setPlaybackRate(0, true);
	camera->recorder->stop();
	camera->setDisplaySettings(false);
	camera->vinst->setRunning(true);
	fflush(stdout);
}
