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
    explicit recModeWindow(QWidget *parent = 0, Camera * cameraInst = 0);
    ~recModeWindow();

private slots:
    void on_cmdOK_clicked();

    void on_cmdCancel_clicked();

    void on_radioNormal_clicked();

    void on_radioGated_clicked();

    void on_radioSegmented_clicked();

    void on_cmdMax_clicked();

    void on_spinRecLengthSeconds_valueChanged(double arg1);

    void on_spinRecLengthSeconds_editingFinished();

    void on_spinRecLengthFrames_editingFinished();

private:
    Camera * camera;
    ImagerSettings_t is;
    Ui::recModeWindow *ui;
};

#endif // RECMODEWINDOW_H
