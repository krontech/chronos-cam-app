#ifndef CAMERA_H
#define CAMERA_H

#include <pthread.h>
#include <semaphore.h>

#include "errorCodes.h"
#include "defines.h"

#include "gpmc.h"
#include "video.h"
//#include "lupa1300.h"
#include "lux1310.h"
#include "userInterface.h"
#include "io.h"
#include "videoRecord.h"
#include "string.h"
#include "types.h"

#define RECORD_DATA_LENGTH		2048		//Number of record data entries for the record sequencer data
#define FPN_ADDRESS				0x0
#define MAX_FRAME_LENGTH		0xF000
#define	LIVE_FRAME_0_ADDRESS	MAX_FRAME_LENGTH
#define	LIVE_FRAME_1_ADDRESS	(MAX_FRAME_LENGTH*2)
#define	LIVE_FRAME_2_ADDRESS	(MAX_FRAME_LENGTH*3)
#define REC_REGION_START		(MAX_FRAME_LENGTH*4)
#define REC_REGION_LEN			ramSize
#define FRAME_ALIGN_WORDS		8			//Align to 256 byte boundaries (8 32-byte words)

#define MAX_FRAME_SIZE_H		1280
#define MAX_FRAME_SIZE_V		1024
#define MAX_STRIDE				1280
#define BITS_PER_PIXEL			12
#define BYTES_PER_WORD			32

#define FPN_AVERAGE_FRAMES		16	//Number of frames to average to get FPN correction data

#define SENSOR_DATA_WIDTH		12
#define COLOR_MATRIX_INT_BITS	3

#define IMAGE_GAIN_FUDGE_FACTOR 1.0		//Multiplier to make sure clipped ADC value actually clips image

/*
typedef enum CameraErrortype
{
	CAMERA_ERROR_NONE = 0,
	CAMERA_THREAD_ERROR,
	CAMERA_ALREADY_RECORDING,
	CAMERA_NOT_RECORDING,
	CAMERA_NO_RECORDING_PRESENT,
	CAMERA_IN_PLAYBACK_MODE,
	CAMERA_ERROR_SENSOR,
	CAMERA_INVALID_IMAGER_SETTINGS,
	CAMERA_FILE_NOT_FOUND,
	CAMERA_FILE_ERROR,
	CAMERA_ERROR_IO,
	CAMERA_INVALID_SETTINGS
} CameraErrortype;
*/
typedef struct {
	UInt32 blockStart;
	UInt32 blockEnd;
	UInt32 blockLast;
} RecData;

typedef union SeqPrmMemWord_t
{
	struct settings_t
	{
	unsigned termRecTrig	: 1;
	unsigned termRecMem		: 1;
	unsigned termRecBlkEnd	: 1;
	unsigned termBlkFull	: 1;
	unsigned termBlkLow		: 1;
	unsigned termBlkHigh	: 1;
	unsigned termBlkFalling	: 1;
	unsigned termBlkRising	: 1;
	unsigned next			: 4;
	unsigned blkSize		: 32;
	unsigned pad			: 20;
	} __attribute__ ((__packed__)) settings;

	struct data_t
	{
		UInt32 low;
		UInt32 high;
	} data;
} SeqPgmMemWord;

typedef struct {
	UInt32 hRes;		//pixels
	UInt32 vRes;		//pixels
	UInt32 stride;		//Number of pixels per line (allows for dark pixels in the last column)
	UInt32 hOffset;		//active area offset from left
	UInt32 vOffset;		//Active area offset from top
	UInt32 exposure;	//10ns increments
	UInt32 period;		//Frame period in 10ns increments
	UInt32 gain;
	UInt32 frameSizeWords;
	UInt32 recRegionSizeFrames;
} ImagerSettings_t;


typedef struct {
	ImagerSettings_t is;
	RecData recData[RECORD_DATA_LENGTH];
	UInt32 numRecRegions;
	UInt32 totalFrames;
	bool valid;
	bool hasBeenSaved;

} RecordSettings_t;

typedef struct {
	UInt32 discardCount;
	UInt32 inputXRes;
	UInt32 inputYRes;
	UInt32 outputXRes;
	UInt32 outputYRes;
	UInt32 xScale;
	UInt32 yScale;
	UInt32 leftOffset;
	UInt32 topFracOffset;
} ScalerSettings_t;



class Camera
{
public:
	Camera();
	~Camera();
	CameraErrortype init(GPMC * gpmcInst, Video * vinstInst, LUX1310 * sensorInst, UserInterface * userInterface, UInt32 ramSizeVal, bool color);
	Int32 startRecording(void);
	Int32 setRecSequencerModeNormal();
	Int32 setRecSequencerModeSingleBlock(UInt32 blockLength);
	Int32 stopRecording(void);
	bool getIsRecording(void);
	void (*endOfRecCallback)(void *);
	void * endOfRecCallbackArg;
	UInt32 recDataPos;
	RecData recData [RECORD_DATA_LENGTH];
	UInt32 recDataLength;
	GPMC * gpmc;
	Video * vinst;
	LUX1310 * sensor;
	UserInterface * ui;
	VideoRecord * recorder;
	IO * io;

	UInt32 getPlayFrameAddr(UInt32 playFrame);
	RecordSettings_t recordingData;
	ImagerSettings_t getImagerSettings() { return imagerSettings; }
	UInt32 setImagerSettings(ImagerSettings_t settings);
	UInt32 setDisplaySettings(bool encoderSafe);
	UInt32 setPlayMode(bool playMode);
	UInt32 playFrame;
	void setPlaybackRate(Int32 speed, bool forward);
	void writeFrameNumbers();
	UInt16 readPixel(UInt32 pixel, UInt32 offset);
	void writePixel(UInt32 pixel, UInt32 offset, UInt16 value);
	UInt16 readPixel12(UInt32 pixel, UInt32 offset);
	UInt16 readPixelBuf(UInt8 * buf, UInt32 pixel);
	void writePixelBuf(UInt8 * buf, UInt32 pixel, UInt16 value);
	UInt16 readPixelBuf12(UInt8 * buf, UInt32 pixel);
	void writePixelBuf12(UInt8 * buf, UInt32 pixel, UInt16 value);
	void computeFPNCorrection();
	void computeFPNCorrection2(UInt32 framesToAverage, bool writeToFile = false, bool factory = false);
	UInt32 autoFPNCorrection(UInt32 framesToAverage, bool writeToFile = false, bool noCap = false, bool factory = false);
	Int32 loadFPNFromFile(char * filename);
	Int32 computeColGainCorrection(UInt32 framesToAverage, bool writeToFile = false);
	Int32 loadColGainFromFile(char * filename);
	UInt32 adcOffsetCorrection(UInt32 iterations, char * filename = "");
	void offsetCorrectionIteration(UInt32 wordAddress = LIVE_FRAME_0_ADDRESS);
	int autoAdcOffsetCorrection(void);
	Int32 autoColGainCorrection(void);
	Int32 adjustExposureToValue(UInt32 level, UInt32 tolerance = 100, bool includeFPNCorrection = true);
	Int32 recordFrames(UInt32 numframes);
	UInt32 getMiddlePixelValue(bool includeFPNCorrection = true);
	Int32 readFrame(UInt32 frame, UInt16 * frameBuffer);
	Int32 getRawCorrectedFrame(UInt32 frame, UInt16 * frameBuffer);
	Int32 readDCG(double * gainCorrection);
	Int32 readFPN(UInt16 * fpnUnpacked);
	Int32 readCorrectedFrame(UInt32 frame, UInt16 * frameBuffer, UInt16 * fpnInput, double * gainCorrection);
	Int32 getRawCorrectedFramesAveraged(UInt32 frame, UInt32 framesToAverage, UInt16 * frameBuffer);
	Int32 takeWhiteReferences(void);
	Int32 startSave(UInt32 startFrame, UInt32 length);
	void setCCMatrix(double * wbMat);
	int setWhiteBalance(UInt32 x, UInt32 y);
	void setFocusAid(bool enable);
	bool getFocusAid();
	int blackCalAllStdRes(bool factory = false);
	bool getFocusPeakEnable(void);
	void setFocusPeakEnable(bool en);
	bool getZebraEnable(void);
	void setZebraEnable(bool en);
	char * getSerialNumber(void) {return serialNumber;}
	void setSerialNumber(const char * sn) {strcpy(serialNumber, sn);}
	bool getIsColor() {return isColor;}
private:
	void setDisplayFrameSource(bool liveDisplaySource);
public:
	void setDisplaySyncInhibit(bool syncInhibit);
private:
	void setDisplayFrameAddress(UInt32 address);
	void setLiveOutputTiming(UInt32 hRes, UInt32 vRes, unsigned int stride);
	void setLiveOutputTiming2(UInt32 hRes, UInt32 vRes, UInt32 stride, UInt32 vOutRes, bool encoderSafe);
	bool getRecDataFifoIsEmpty(void);
	UInt32 readRecDataFifo(void);
	bool getRecording(void);
	void startSequencer(void);
	void terminateRecord(void);
	void writeSeqPgmMem(SeqPgmMemWord pgmWord, UInt32 address);
	void setFrameSizeWords(UInt16 frameSize);
	void setRecRegionStartWords(UInt32 start);
	void setRecRegionEndWords(UInt32 end);
public:
	void readAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length);
private:
	void writeAcqMem(UInt32 * buf, UInt32 offsetWords, UInt32 length);
	void writeDGCMem(double gain, UInt32 column);
	bool readIsColor(void);
public:
	bool getFocusPeakEnableLL(void);
	void setFocusPeakEnableLL(bool en);
	UInt8 getFocusPeakColor(void);
	void setFocusPeakColor(UInt8 color);
	bool getZebraEnableLL(void);
	void setZebraEnableLL(bool en);
	void setFocusPeakThreshold(UInt32 thresh);
	UInt32 getFocusPeakThreshold(void);
	Int32 getRamSizeGB(UInt32 * stick0SizeGB, UInt32 * stick1SizeGB);
	Int32 readSerialNumber(char * dest);
	Int32 writeSerialNumber(char * src);
	UInt16 getFPGAVersion(void);
private:
	void endOfRec(void);
	UInt32 getNumFrames(UInt32 start, UInt32 end);
	UInt32 getBlockFrameAddress(UInt32 block, UInt32 frame);
	UInt16 getMaxFPNValue(UInt16 * buf, UInt32 count);


	friend void* recDataThread(void *arg);
	friend void frameCallback(void * arg);

	volatile bool recording;
	bool playbackMode;
	Int32 playbackSpeed;
	bool playbackForward;
	UInt32 playDivisorCount;
	void processPlay(void);

	ImagerSettings_t imagerSettings;
	bool isColor;
	double ccMatrix[9];
	double wbMatrix[3];	//White balance multipliers computed by color matrix generator
	double wbMat[3];	//Actual white balance computed during runtime
	double imgGain;
	bool focusPeakEnabled;
	bool zebraEnabled;
	char serialNumber[SERIAL_NUMBER_MAX_LEN+1];

	bool lastRecording;
	bool terminateRecDataThread;
	UInt32 ramSize;
	pthread_t recDataThreadID;
	sem_t playMutex;

};

#endif // CAMERA_H
