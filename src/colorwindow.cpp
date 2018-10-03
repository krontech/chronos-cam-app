
#include "camera.h"
#include "cameraRegisters.h"
#include "colorwindow.h"
#include "ui_colorwindow.h"

ColorWindow::ColorWindow(QWidget *parent, Camera *cameraInst) :
	QDialog(parent),
	ui(new Ui::ColorWindow)
{
	camera = cameraInst;
	ui->setupUi(this);

	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	move(camera->ButtonsOnLeft ? 200:0, 0);

	/* Load the custom white balance. */
	applyMatrix(camera->colorCalMatrix);
	whiteBalanceChanged();
}

ColorWindow::~ColorWindow()
{
	camera->setWhiteBalance(camera->sceneWhiteBalMatrix[0], camera->sceneWhiteBalMatrix[1], camera->sceneWhiteBalMatrix[2]);
	delete ui;
}

void ColorWindow::whiteBalanceChanged(void)
{
	ui->wbRed->setValue(camera->sceneWhiteBalMatrix[0]);
	ui->wbGreen->setValue(camera->sceneWhiteBalMatrix[1]);
	ui->wbBlue->setValue(camera->sceneWhiteBalMatrix[2]);
}

void ColorWindow::getWhiteBalance(double *rgb)
{
	rgb[0] = ui->wbRed->value();
	rgb[1] = ui->wbGreen->value();
	rgb[2] = ui->wbBlue->value();
}

void ColorWindow::applyMatrix(const double *matrix)
{
	ui->ccm11->setValue(matrix[0]);
	ui->ccm12->setValue(matrix[1]);
	ui->ccm13->setValue(matrix[2]);
	ui->ccm21->setValue(matrix[3]);
	ui->ccm22->setValue(matrix[4]);
	ui->ccm23->setValue(matrix[5]);
	ui->ccm31->setValue(matrix[6]);
	ui->ccm32->setValue(matrix[7]);
	ui->ccm33->setValue(matrix[8]);
}

void ColorWindow::on_ccmDefault_clicked(void)
{
	applyMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccmApply_clicked(void)
{
	/* TODO? Fixme? */
}

/* Live updates to the white balance matrix */
void ColorWindow::on_wbRed_valueChanged(double arg)
{
	camera->setWhiteBalance(arg, ui->wbGreen->value(), ui->wbBlue->value());
}

void ColorWindow::on_wbGreen_valueChanged(double arg)
{
	camera->setWhiteBalance(ui->wbRed->value(), arg, ui->wbBlue->value());
}

void ColorWindow::on_wbBlue_valueChanged(double arg)
{
	camera->setWhiteBalance(ui->wbRed->value(), ui->wbGreen->value(), arg);
}

/* Live updates to the color matrix. */
void ColorWindow::on_ccm11_valueChanged(double arg) { camera->gpmc->write16(CCM_11_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm12_valueChanged(double arg) { camera->gpmc->write16(CCM_12_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm13_valueChanged(double arg) { camera->gpmc->write16(CCM_13_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm21_valueChanged(double arg) { camera->gpmc->write16(CCM_21_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm22_valueChanged(double arg) { camera->gpmc->write16(CCM_22_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm23_valueChanged(double arg) { camera->gpmc->write16(CCM_23_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm31_valueChanged(double arg) { camera->gpmc->write16(CCM_31_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm32_valueChanged(double arg) { camera->gpmc->write16(CCM_32_ADDR, (int)(4096.0 * arg)); }
void ColorWindow::on_ccm33_valueChanged(double arg) { camera->gpmc->write16(CCM_33_ADDR, (int)(4096.0 * arg)); }
