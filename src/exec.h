#ifndef EXEC_H
#define EXEC_H

void getCamJson(QString parameter, QString *jsonString);
void setCamJson(QString jsonString);
void parseJsonArray(QString parameter, QString jsonString, uint32_t size, double *values);
void buildJsonCalibration(QString *jsonString, QString calType);
void buildJsonArray(QString parameter, QString *jsonString, UInt32 size, double *values);
void startRecordingCamJson( QString *jsonString);
void parseJsonTiming(QString jsonString, FrameGeometry *geometry, FrameTiming *timing);
void startCalibrationCamJson( QString *jsonOutString, QString *jsonInString);

#endif // EXEC_H

