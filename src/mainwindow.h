#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "camera.h"
#include "types.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
	Camera * camera;

private slots:

	void on_cmdWrite_clicked();

	void on_cmdAdvPhase_clicked();

	void on_cmdSeqOn_clicked();

	void on_cmdSeqOff_clicked();

	void on_cmdSetOffset_clicked();

	void on_cmdTrigger_clicked();

	void on_cmdRam_clicked();

	void on_cmdReset_clicked();

	void on_cmdFPN_clicked();

	void on_cmdFrameNumbers_clicked();

	void on_cmdDecPhase_clicked();

	void on_cmdGC_clicked();

	void on_cmdOffsetCorrection_clicked();

	void on_cmdSaveOC_clicked();

	void on_cmdAutoBlack_clicked();

	void on_cmdSaveFrame_clicked();

private:
    Ui::MainWindow *ui;
	friend void endOfRecCallback(void * arg);
	UInt16 readPixel(UInt32 pixel, UInt32 offset);
	void writePixel(UInt32 pixel, UInt32 offset, UInt16 value);
	void writePixel12(UInt32 pixel, UInt32 offset, UInt16 value);
};

#endif // MAINWINDOW_H
