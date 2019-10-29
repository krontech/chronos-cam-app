#ifndef TRIGGERDELAYWINDOW_H
#define TRIGGERDELAYWINDOW_H

#include <QWidget>
#include "camera.h"

namespace Ui {
class triggerDelayWindow;
}

class triggerDelayWindow : public QWidget
{
    Q_OBJECT

public:
	explicit triggerDelayWindow(QWidget *parent = 0, Camera * cameraInst = 0, ImagerSettings_t *is = 0);
    ~triggerDelayWindow();

private slots:
    void on_cmdOK_clicked();

    void on_cmdCancel_clicked();

    void on_horizontalSlider_sliderMoved(int position);

    void on_cmdZeroPercent_clicked();

    void on_cmdFiftyPercent_clicked();

    void on_cmdHundredPercent_clicked();

    void on_spinPreSeconds_valueChanged(double arg1);

    void on_spinPreFrames_valueChanged(int arg1);

    void on_spinPostSeconds_valueChanged(double arg1);

    void on_spinPostFrames_valueChanged(int arg1);

    void on_spinPreRecSeconds_valueChanged(double arg1);

    void on_spinPreRecFrames_valueChanged(int arg1);

    void on_cmdResetToDefaults_clicked();

    void on_cmdMorePreRecTime_clicked();

    void on_horizontalSlider_sliderReleased();
    
private:
    void updateControls(UInt32 postTriggerFrames);
    Ui::triggerDelayWindow *ui;

	Camera * camera;
	ImagerSettings_t *is;
    UInt32 recLenFrames;
    double period;

	UInt32 triggerPostFrames;
};

#endif // TRIGGERDELAYWINDOW_H
