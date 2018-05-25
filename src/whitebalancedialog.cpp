#include "whitebalancedialog.h"
#include "ui_whitebalancedialog.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>

#define RED   camera->sceneWhiteBalMatrix[0]
#define GREEN camera->sceneWhiteBalMatrix[1]
#define BLUE  camera->sceneWhiteBalMatrix[2]

whiteBalanceDialog::whiteBalanceDialog(QWidget *parent, Camera * cameraInst) :
	QDialog(parent),
	ui(new Ui::whiteBalanceDialog)
{
	windowInitComplete = false;
	ui->setupUi(this);
	camera = cameraInst;
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(camera->ButtonsOnLeft? 0:600, 0);
	sw = new StatusWindow;

	QSettings appSettings;
	addPreset(appSettings.value("whiteBalance/customR", 1.0).toDouble(),
			appSettings.value("whiteBalance/customG", 1.0).toDouble(),
			appSettings.value("whiteBalance/customB", 1.0).toDouble(),
			"Custom");
	customWhiteBalOld[0] = sceneWhiteBalPresets[0][0];
	customWhiteBalOld[1] = sceneWhiteBalPresets[0][1];
	customWhiteBalOld[2] = sceneWhiteBalPresets[0][2];
	addPreset(1.53, 1.00, 1.35, "8000K(Cloudy Sky)");
	addPreset(1.42, 1.00, 1.46, "6500K(Noon Daylight)");
	addPreset(1.35, 1.00, 1.584,"5600K(Avg Daylight)");
	addPreset(1.30, 1.00, 1.61, "5250K(Flash)");
	addPreset(1.22, 1.00, 1.74, "4600K(Flourescent)");
	windowInitComplete = true;
	ui->comboWB->setCurrentIndex(camera->getWBIndex());
}

whiteBalanceDialog::~whiteBalanceDialog()
{
	delete ui;
}

void whiteBalanceDialog::addPreset(double r, double g, double b, QString s){
	ui->comboWB->addItem(s);
	UInt8 workingPreset = ui->comboWB->count()-1;
	sceneWhiteBalPresets[workingPreset][0] = r;
	sceneWhiteBalPresets[workingPreset][1] = g;
	sceneWhiteBalPresets[workingPreset][2] = b;
}

void whiteBalanceDialog::on_comboWB_currentIndexChanged(int index)
{
	if(!windowInitComplete) return;
	camera->setWBIndex(index);
	RED =   sceneWhiteBalPresets[index][0];
	GREEN = sceneWhiteBalPresets[index][1];
	BLUE =  sceneWhiteBalPresets[index][2];
	QSettings appSettings;
	appSettings.setValue("whiteBalance/currentR", RED);
	appSettings.setValue("whiteBalance/currentG", GREEN);
	appSettings.setValue("whiteBalance/currentB", BLUE);
	qDebug() <<" colors: " << RED << GREEN << BLUE;
	camera->setCCMatrix();
}

void whiteBalanceDialog::on_cmdSetCustomWB_clicked()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Set white balance?", "Will set white balance. Continue?", QMessageBox::Yes|QMessageBox::No);
	if(QMessageBox::Yes != reply)
		return;

	ui->comboWB->setCurrentIndex(0);
	ui->cmdResetCustomWB->setEnabled(true);

	Int32 ret = camera->setWhiteBalance(camera->getImagerSettings().hRes / 2 & 0xFFFFFFFE,
								 camera->getImagerSettings().vRes / 2 & 0xFFFFFFFE);	//Sample from middle but make sure position is a multiple of 2
	if(ret == CAMERA_CLIPPED_ERROR)
	{
		sw->setText("Clipping. Reduce exposure and try white balance again");
		sw->setTimeout(3000);
		sw->show();
		return;
	}
	else if(ret == CAMERA_LOW_SIGNAL_ERROR)
	{
		sw->setText("Too dark. Increase exposure and try white balance again");
		sw->setTimeout(3000);
		sw->show();
		return;
	}
	camera->setCCMatrix();
	QSettings appSettings;
	appSettings.setValue("whiteBalance/customR", RED);
	appSettings.setValue("whiteBalance/customG", GREEN);
	appSettings.setValue("whiteBalance/customB", BLUE);
	sceneWhiteBalPresets[0][0] = RED;
	sceneWhiteBalPresets[0][1] = GREEN;
	sceneWhiteBalPresets[0][2] = BLUE;
}

void whiteBalanceDialog::on_cmdClose_clicked()
{
	delete sw;
	this->close();
}

void whiteBalanceDialog::on_cmdResetCustomWB_clicked()
{
    RED   = sceneWhiteBalPresets[0][0] = customWhiteBalOld[0];
    GREEN = sceneWhiteBalPresets[0][1] = customWhiteBalOld[1];
    BLUE  = sceneWhiteBalPresets[0][2] = customWhiteBalOld[2];
    QSettings appSettings;
    appSettings.setValue("whiteBalance/currentR", RED);
    appSettings.setValue("whiteBalance/currentG", GREEN);
    appSettings.setValue("whiteBalance/currentB", BLUE);
    if(ui->comboWB->currentIndex() == 0)	camera->setCCMatrix();
    qDebug() <<" colors: " << RED << GREEN << BLUE;
}
