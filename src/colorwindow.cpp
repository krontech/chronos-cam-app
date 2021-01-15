
#include "camera.h"
#include "colorwindow.h"
#include "ui_colorwindow.h"

ColorWindow::ColorWindow(QWidget *parent, Camera *cameraInst, const double *matrix) :
	QDialog(parent),
	ui(new Ui::ColorWindow)
{
	camera = cameraInst;
	ui->setupUi(this);

	this->setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	move(camera->ButtonsOnLeft ? (800 - width()) : 0, 0);

	colorMatrixChanged();
	whiteBalanceChanged();
}

ColorWindow::~ColorWindow()
{
	double rgb[3] = { ui->wbRed->value(), ui->wbGreen->value(), ui->wbBlue->value() };
	camera->cinst->setArray("wbMatrix", 3, rgb);
	camera->setCCMatrix(camera->colorCalMatrix);
	delete ui;
}

void ColorWindow::colorMatrixChanged(void)
{
	setMatrix(camera->colorCalMatrix);
}

void ColorWindow::whiteBalanceChanged(void)
{
	double wbMatrix[3];
	camera->cinst->getArray("wbColor", 3, wbMatrix);

	ui->wbRed->setValue(wbMatrix[0]);
	ui->wbGreen->setValue(wbMatrix[1]);
	ui->wbBlue->setValue(wbMatrix[2]);
}

void ColorWindow::getWhiteBalance(double *rgb)
{
	rgb[0] = ui->wbRed->value();
	rgb[1] = ui->wbGreen->value();
	rgb[2] = ui->wbBlue->value();
}

void ColorWindow::getColorMatrix(double *matrix)
{
	matrix[0] = ui->ccm11->value();
	matrix[1] = ui->ccm12->value();
	matrix[2] = ui->ccm13->value();
	matrix[3] = ui->ccm21->value();
	matrix[4] = ui->ccm22->value();
	matrix[5] = ui->ccm23->value();
	matrix[6] = ui->ccm31->value();
	matrix[7] = ui->ccm32->value();
	matrix[8] = ui->ccm33->value();
}

void ColorWindow::setMatrix(const double *matrix)
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
    QString modelName;
    // Check if the camera is a 2.1 or not
    camera->cinst->getString("cameraModel", &modelName);
    if(modelName.startsWith("CR21"))
        setMatrix(camera->ccmPresets21[0].matrix); // default for 2.1 camera
    else
        setMatrix(camera->ccmPresets14[0].matrix); // default for 1.4 camera
}

void ColorWindow::on_ccmIdentity_clicked(void)
{
	double identity[9] = {
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0
	};
	setMatrix(identity);
}

void ColorWindow::on_ccmApply_clicked(void)
{
	emit applyColorMatrix();
}

void ColorWindow::on_wbApply_clicked(void)
{
	emit applyWhiteBalance();
}

/* Live updates to the white balance matrix */
void ColorWindow::on_wbRed_valueChanged(double arg)
{
	double wb[3] = { arg, ui->wbGreen->value(), ui->wbBlue->value() };
	camera->cinst->setArray("wbColor", 3, wb);
}

void ColorWindow::on_wbGreen_valueChanged(double arg)
{
	double wb[3] = {ui->wbRed->value(), arg, ui->wbBlue->value() };
	camera->cinst->setArray("wbColor", 3, wb);
}

void ColorWindow::on_wbBlue_valueChanged(double arg)
{
	double wb[3] = {ui->wbRed->value(), ui->wbGreen->value(), arg };
	camera->cinst->setArray("wbColor", 3, wb);
}

void ColorWindow::on_ccm11_valueChanged(double arg)
{
	camera->colorCalMatrix[0] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm12_valueChanged(double arg)
{
	camera->colorCalMatrix[1] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm13_valueChanged(double arg)
{
	camera->colorCalMatrix[2] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm21_valueChanged(double arg)
{
	camera->colorCalMatrix[3] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm22_valueChanged(double arg)
{
	camera->colorCalMatrix[4] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm23_valueChanged(double arg)
{
	camera->colorCalMatrix[5] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm31_valueChanged(double arg)
{
	camera->colorCalMatrix[6] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm32_valueChanged(double arg)
{
	camera->colorCalMatrix[7] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}

void ColorWindow::on_ccm33_valueChanged(double arg)
{
	camera->colorCalMatrix[8] = arg;
	camera->setCCMatrix(camera->colorCalMatrix);
}
