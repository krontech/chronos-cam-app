#ifndef COLORWINDOW_H
#define COLORWINDOW_H

#include <QDialog>
#include "camera.h"

namespace Ui {
class ColorWindow;
}

class ColorWindow : public QDialog
{
	Q_OBJECT

public:
	explicit ColorWindow(QWidget *parent = 0, Camera * cameraInst = NULL, const double *matrix = NULL);
	~ColorWindow();

	void whiteBalanceChanged(void);
	void colorMatrixChanged(void);
	void getWhiteBalance(double *);
	void getColorMatrix(double *);

signals:
	void applyColorMatrix();
	void applyWhiteBalance();

private slots:
	void on_wbRed_valueChanged(double arg);
	void on_wbGreen_valueChanged(double arg);
	void on_wbBlue_valueChanged(double arg);

	void on_ccm11_valueChanged(double arg);
	void on_ccm12_valueChanged(double arg);
	void on_ccm13_valueChanged(double arg);
	void on_ccm21_valueChanged(double arg);
	void on_ccm22_valueChanged(double arg);
	void on_ccm23_valueChanged(double arg);
	void on_ccm31_valueChanged(double arg);
	void on_ccm32_valueChanged(double arg);
	void on_ccm33_valueChanged(double arg);

	void on_ccmDefault_clicked();
	void on_ccmIdentity_clicked();
	void on_ccmApply_clicked();
	void on_wbApply_clicked();

private:
	Ui::ColorWindow *ui;
	Camera * camera;

	void setMatrix(const double *matrix);
};

#endif // COLORWINDOW_H
