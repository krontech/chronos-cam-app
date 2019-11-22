#ifndef RECMODEWINDOW_H
#define RECMODEWINDOW_H

#include <QWidget>
#include "types.h"
#include "camera.h"

namespace Ui {
class recModeWindow;
}

class recModeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit recModeWindow(QWidget *parent = 0, Camera * cameraInst = 0, ImagerSettings_t * settings = 0);
    ~recModeWindow();

private slots:
    void on_cmdOK_clicked();

    void on_cmdCancel_clicked();

    void on_radioNormal_clicked();

	void on_radioGated_clicked();

	void on_radioLive_clicked();

    void on_radioSegmented_clicked();

    void on_cmdMax_clicked();

    void on_spinRecLengthSeconds_valueChanged(double arg1);

    void on_spinSegmentCount_valueChanged(int arg1);

    void on_spinRecLengthFrames_valueChanged(int arg1);

    void on_spinPrerecordSeconds_valueChanged(double arg1);

    void on_spinPrerecordFrames_valueChanged(int arg1);

    void on_spinPrerecordSeconds_editingFinished();

private:
    Camera * camera;
    ImagerSettings_t * is;
    Ui::recModeWindow *ui;
	SensorInfo_t sensor;

    void updateSegmentSizeText(UInt32 segmentCount);
};

#endif // RECMODEWINDOW_H
