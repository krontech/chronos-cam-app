#ifndef WHITEBALANCEDIALOG_H
#define WHITEBALANCEDIALOG_H

#include <QDialog>
#include <QSplashScreen>
#include "camera.h"
#include "statuswindow.h"
#include "colorwindow.h"

namespace Ui {
class whiteBalanceDialog;
}

class whiteBalanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit whiteBalanceDialog(QWidget *parent = 0, Camera * cameraInst = NULL);
	~whiteBalanceDialog();
	void saveColor(void);
	void restoreColor(void);
	double saveMatrix[9];
	int saveWbTemp;



private slots:
	void on_comboWB_currentIndexChanged(int index);
	void on_comboColor_currentIndexChanged(int index);

	void on_cmdSetCustomWB_clicked();

	void on_cmdClose_clicked();
	void on_cmdCancel_clicked();

	void on_cmdResetCustomWB_clicked();

	void on_cmdMatrix_clicked();

	void applyColorMatrix();
	void applyWhiteBalance();
	
private:
	Ui::whiteBalanceDialog *ui;
	Camera * camera;
	bool windowInitComplete;
	StatusWindow * sw;
	ColorWindow * cw;
	QSplashScreen *crosshair;

	/* User Custom Color Matrix */
	ColorMatrix_t ccmCustom = {
		"Custom", {
			1.0, 0.0, 0.0,
			0.0, 1.0, 0.0,
			0.0, 0.0, 1.0
		}
	};

	int customWhiteBalIndex;
	double customWhiteBalOld[3] = {1.0, 1.0, 1.0};
};

#endif // WHITEBALANCEDIALOG_H
