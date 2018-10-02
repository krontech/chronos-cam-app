#include "whitebalancedialog.h"
#include "ui_whitebalancedialog.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QBitmap>

#define RED   camera->sceneWhiteBalMatrix[0]
#define GREEN camera->sceneWhiteBalMatrix[1]
#define BLUE  camera->sceneWhiteBalMatrix[2]
#define COMBO_MAX_INDEX ui->comboWB->count()-1

/* 32x32 Crosshair in PNG format */
const uint8_t crosshair32x32_png[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20,
	0x01, 0x03, 0x00, 0x00, 0x00, 0x49, 0xb4, 0xe8, 0xb7, 0x00, 0x00, 0x00,
	0x06, 0x50, 0x4c, 0x54, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa5,
	0x67, 0xb9, 0xcf, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53, 0x00,
	0x40, 0xe6, 0xd8, 0x66, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73,
	0x00, 0x00, 0x0b, 0x13, 0x00, 0x00, 0x0b, 0x13, 0x01, 0x00, 0x9a, 0x9c,
	0x18, 0x00, 0x00, 0x00, 0x17, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63,
	0x60, 0x60, 0x3c, 0xc0, 0xc0, 0x40, 0x1d, 0xe2, 0x3f, 0x10, 0x20, 0x11,
	0x54, 0x31, 0x14, 0x00, 0x6f, 0x7a, 0x21, 0xd2, 0x4b, 0xd5, 0xd2, 0xcf,
	0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

whiteBalanceDialog::whiteBalanceDialog(QWidget *parent, Camera * cameraInst) :
	QDialog(parent),
	ui(new Ui::whiteBalanceDialog)
{
	QSettings appSettings;

	windowInitComplete = false;
	ui->setupUi(this);
	camera = cameraInst;
	this->setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	this->move(camera->ButtonsOnLeft? 0:600, 0);
	sw = new StatusWindow;

	/* Draw a crosshair to show the white balance position */
	QPixmap crosspx;
	crosspx.loadFromData(crosshair32x32_png, sizeof(crosshair32x32_png));

	crosshair = new QSplashScreen(crosspx);
	crosshair->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SplashScreen | Qt::FramelessWindowHint);
	crosshair->setMask(crosspx.mask());
	crosshair->setGeometry((600 - crosspx.width()) / 2, (480 - crosspx.height()) / 2, crosspx.width(), crosspx.height());
	crosshair->show();
	
	addPreset(1.53, 1.00, 1.35, "8000K(Cloudy Sky)");
	addPreset(1.42, 1.00, 1.46, "6500K(Noon Daylight)");
	addPreset(1.35, 1.00, 1.584,"5600K(Avg Daylight)");
	/* Since "Avg Daylight" is chosen by default if this is the user's first time entering the WB dialog,
	these the default values set near the end of Camera::init() on boot should match up with this,
	or else the white balance will change from the original values to this upon opening the dialog. */
	addPreset(1.30, 1.00, 1.61, "5250K(Flash)");
	addPreset(1.22, 1.00, 1.74, "4600K(Flourescent)");
	addPreset(1.02, 1.00, 1.91, "3200K(Tungsten)");
	
	if(appSettings.value("whiteBalance/customR", 0.0).toDouble() != 0.0){
		//Only if custom values have been loaded, add "Custom" to the list and store old WB values
		
		addCustomPreset();
		
		customWhiteBalOld[0] = sceneWhiteBalPresets[COMBO_MAX_INDEX][0];
		customWhiteBalOld[1] = sceneWhiteBalPresets[COMBO_MAX_INDEX][1];
		customWhiteBalOld[2] = sceneWhiteBalPresets[COMBO_MAX_INDEX][2];
	} else customWhiteBalOld[0] = -1.0;// so that on_cmdSetCustomWB_clicked() knows not to enable the "Reset Custom WB" button
	
	windowInitComplete = true;
	ui->comboWB->setCurrentIndex(camera->getWBIndex());
}

whiteBalanceDialog::~whiteBalanceDialog()
{
	delete ui;
}

void whiteBalanceDialog::addPreset(double r, double g, double b, QString s){
	ui->comboWB->addItem(s);
	UInt8 workingPreset = COMBO_MAX_INDEX;
	sceneWhiteBalPresets[workingPreset][0] = r;
	sceneWhiteBalPresets[workingPreset][1] = g;
	sceneWhiteBalPresets[workingPreset][2] = b;
}

void whiteBalanceDialog::addCustomPreset(){
	QSettings appSettings;
	addPreset(appSettings.value("whiteBalance/customR", 1.0).toDouble(),
			appSettings.value("whiteBalance/customG", 1.0).toDouble(),
			appSettings.value("whiteBalance/customB", 1.0).toDouble(),
			"Custom");
}

void whiteBalanceDialog::on_comboWB_currentIndexChanged(int index)
{
	if(!windowInitComplete) return;
	QSettings appSettings;
	camera->setWBIndex(index);
	
	RED =   sceneWhiteBalPresets[index][0];
	GREEN = sceneWhiteBalPresets[index][1];
	BLUE =  sceneWhiteBalPresets[index][2];
	
	appSettings.setValue("whiteBalance/currentR", RED);
	appSettings.setValue("whiteBalance/currentG", GREEN);
	appSettings.setValue("whiteBalance/currentB", BLUE);
	//qDebug() <<" colors: " << RED << GREEN << BLUE;
	camera->setCCMatrix();
}

void whiteBalanceDialog::on_cmdSetCustomWB_clicked()
{
	Int32 ret = camera->setWhiteBalance(camera->getImagerSettings().hRes / 2, camera->getImagerSettings().vRes / 2);
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
	if(appSettings.value("whiteBalance/customR", 0.0).toDouble() == 0.0)
		ui->comboWB->addItem("Custom"); //Only add "Custom" if the values have not already been set
	appSettings.setValue("whiteBalance/customR", RED);
	appSettings.setValue("whiteBalance/customG", GREEN);
	appSettings.setValue("whiteBalance/customB", BLUE);
	
	sceneWhiteBalPresets[COMBO_MAX_INDEX][0] = RED;
	sceneWhiteBalPresets[COMBO_MAX_INDEX][1] = GREEN;
	sceneWhiteBalPresets[COMBO_MAX_INDEX][2] = BLUE;
	
	ui->comboWB->setCurrentIndex(COMBO_MAX_INDEX);
	if(customWhiteBalOld[0] > 0.0) ui->cmdResetCustomWB->setEnabled(true);
	//qDebug("COMBO_COUNT = %d", COMBO_MAX_INDEX);
}

void whiteBalanceDialog::on_cmdClose_clicked()
{
	delete sw;
	delete crosshair;
	this->close();
}

void whiteBalanceDialog::on_cmdResetCustomWB_clicked()
{
    RED   = sceneWhiteBalPresets[COMBO_MAX_INDEX][0] = customWhiteBalOld[0];
    GREEN = sceneWhiteBalPresets[COMBO_MAX_INDEX][1] = customWhiteBalOld[1];
    BLUE  = sceneWhiteBalPresets[COMBO_MAX_INDEX][2] = customWhiteBalOld[2];
    
    QSettings appSettings;
    appSettings.setValue("whiteBalance/currentR", RED);
    appSettings.setValue("whiteBalance/currentG", GREEN);
    appSettings.setValue("whiteBalance/currentB", BLUE);
    
    if(ui->comboWB->currentIndex() == COMBO_MAX_INDEX)	camera->setCCMatrix();
}
