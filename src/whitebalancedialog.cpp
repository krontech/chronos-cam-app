#include "whitebalancedialog.h"
#include "ui_whitebalancedialog.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QBitmap>
#include <QListView>

/* 32x32 Crosshair in PNG format */
static const uint8_t crosshair32x32_png[] = {
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
	QString ccmName = appSettings.value("colorMatrix/current", "").toString();


	windowInitComplete = false;
	ui->setupUi(this);
	camera = cameraInst;
	saveColor();

	setWindowFlags(Qt::Dialog /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	move(camera->ButtonsOnLeft ? 0:600, 0);
	sw = new StatusWindow;

	/* Draw a crosshair to show the white balance position */
	QPixmap crosspx;
	crosspx.loadFromData(crosshair32x32_png, sizeof(crosshair32x32_png));

	crosshair = new QSplashScreen(crosspx);
	crosshair->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SplashScreen | Qt::FramelessWindowHint);
	crosshair->setMask(crosspx.mask());
	crosshair->setGeometry((camera->ButtonsOnLeft ? 200:0) + (600 - crosspx.width()) / 2, (480 - crosspx.height()) / 2, crosspx.width(), crosspx.height());
	crosshair->show();

	/* Hide the color matrix dialog for now */
	cw = NULL;
	ui->cmdMatrix->setText(camera->ButtonsOnLeft ? "Matrix >>" : "<< Matrix");
	
	/* Load white balance presets. */
	ui->comboWB->addItem("8000K(Cloudy Sky)",    8000);
	ui->comboWB->addItem("6500K(Noon Daylight)", 6500);
	ui->comboWB->addItem("5600K(Avg Daylight)",  5600);
	/* Since "Avg Daylight" is chosen by default if this is the user's first time entering the WB dialog,
	these the default values set near the end of Camera::init() on boot should match up with this,
	or else the white balance will change from the original values to this upon opening the dialog. */
	ui->comboWB->addItem("5250K(Flash)",         5250);
	ui->comboWB->addItem("4600K(Flourescent)",   4600);
	ui->comboWB->addItem("3200K(Tungsten)",      3200);
	
	/* If the custom white balance is sane, load it too. */
	camera->cinst->getArray("wbCustomColor", 3, (double *)&customWhiteBalOld);
	if (customWhiteBalOld[0] && customWhiteBalOld[1] && customWhiteBalOld[2]) {
		ui->comboWB->addItem("Custom", 0);
		customWhiteBalIndex = ui->comboWB->count() - 1;
	}
	else {
		// so that on_cmdSetCustomWB_clicked() knows not to enable the "Reset Custom WB" button
		customWhiteBalIndex = -1;
		customWhiteBalOld[0] = -1.0;
		customWhiteBalOld[1] = -1.0;
		customWhiteBalOld[2] = -1.0;
	}

	/* Load Color Matrix presets */
	for (int i = 0; i < sizeof(camera->ccmPresets)/sizeof(camera->ccmPresets[0]); i++) {
		ui->comboColor->addItem(camera->ccmPresets[i].name);
		if (ccmName == camera->ccmPresets[i].name) {
			ui->comboColor->setCurrentIndex(i);
		}
	}

	/* Load Custom Matrix */
	if (appSettings.beginReadArray("colorMatrix") >= 9) {
		for (int i = 0; i < 9; i++) {
			appSettings.setArrayIndex(i);
			ccmCustom.matrix[i] = appSettings.value("ccmValue").toDouble();
		}
		ui->comboColor->addItem(ccmCustom.name);
		if (ccmName == ccmCustom.name) {
			ui->comboColor->setCurrentIndex(sizeof(camera->ccmPresets)/sizeof(camera->ccmPresets[0]));
		}
	}
	appSettings.endArray();

    //Set QListView to change items in the combo box with qss
    ui->comboColor->setView(new QListView);
    ui->comboColor->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	/* Get the wbTemperature and select the nearest preset. */
	int diff = INT_MAX;
	int wbIndex = 0;
	for (int i = 0; i < ui->comboWB->count(); i++) {
		int x = ui->comboWB->itemData(i).toInt() - saveWbTemp;
		if (x < 0) x = -x; /* take the absolute value */
		if (x < diff) {
			wbIndex = i;   /* find which preset is closest */
			diff = x;
		}
	}

	windowInitComplete = true;
	ui->comboWB->setCurrentIndex(wbIndex);

    //Set QListView to change items in the combo box with qss
    ui->comboWB->setView(new QListView);
    ui->comboWB->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

whiteBalanceDialog::~whiteBalanceDialog()
{
	delete ui;
}

void whiteBalanceDialog::on_comboWB_currentIndexChanged(int index)
{
	if(!windowInitComplete) return;

	camera->cinst->setProperty("wbTemperature", ui->comboWB->itemData(index));

	if (cw) cw->whiteBalanceChanged();
}

void whiteBalanceDialog::on_comboColor_currentIndexChanged(int index)
{
	if(!windowInitComplete) return;
	QSettings appSettings;
	const ColorMatrix_t *choice;

	if (index < sizeof(camera->ccmPresets)/sizeof(camera->ccmPresets[0])) {
		choice = &camera->ccmPresets[index];
	} else {
		choice = &ccmCustom;
	}

	memcpy(camera->colorCalMatrix, choice->matrix, sizeof(choice->matrix));
	camera->setCCMatrix(camera->colorCalMatrix);
	if (cw) cw->colorMatrixChanged();

	appSettings.setValue("colorMatrix/current", choice->name);
}

void whiteBalanceDialog::applyColorMatrix(void)
{
	if (!cw) return;
	QSettings appSettings;
	int index;

	/* Retreive the entered color matrix. */
	cw->getColorMatrix(ccmCustom.matrix);
	cw->getColorMatrix(camera->colorCalMatrix);
	index = ui->comboColor->findText(ccmCustom.name);
	if (index < 0) {
		ui->comboColor->addItem(ccmCustom.name);
		ui->comboColor->setCurrentIndex(ui->comboColor->count() - 1);
	} else {
		ui->comboColor->setCurrentIndex(index);
	}

	/* Save the matrix in the settings. */
	appSettings.setValue("colorMatrix/current", "Custom");
	appSettings.beginWriteArray("colorMatrix", 9);
	for (int i = 0; i < 9; i++) {
		appSettings.setArrayIndex(i);
		appSettings.setValue("ccmValue", camera->colorCalMatrix[i]);
	}
	appSettings.endArray();
}

void whiteBalanceDialog::applyWhiteBalance(void)
{
	/* Get the white balance from the colour matrix window, if open. */
	if (cw) {
		double wbMatrix[3];

		cw->getWhiteBalance(wbMatrix);
		camera->cinst->setArray("wbCustomColor", 3, wbMatrix);
		camera->cinst->setInt("wbTemperature", 0);
	}
	else {
		Int32 ret;

		ret = camera->cinst->startWhiteBalance();
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
	}

	/* Enable the reset "custom" button. */
	if (customWhiteBalOld[0] > 0.0) {
		ui->cmdResetCustomWB->setEnabled(true);
	}
	/* Add the custom white balance to the dropdown. */
	if (customWhiteBalIndex < 0) {
		ui->comboWB->addItem("Custom", 0);
		customWhiteBalIndex = ui->comboWB->count() - 1;
	}

	/* Select the custom white balance. */
	ui->comboWB->setCurrentIndex(customWhiteBalIndex);
}

void whiteBalanceDialog::on_cmdSetCustomWB_clicked()
{
	applyWhiteBalance();
}

void whiteBalanceDialog::on_cmdClose_clicked()
{
	if (cw) {
		delete cw;
		cw = NULL;
	}
	delete sw;
	delete crosshair;
	this->close();
}

void whiteBalanceDialog::on_cmdCancel_clicked()
{
	if (cw) {
		delete cw;
		cw = NULL;
	}
	delete sw;
	delete crosshair;
	restoreColor();
	camera->setCCMatrix(camera->colorCalMatrix);
	this->close();
}

void whiteBalanceDialog::on_cmdResetCustomWB_clicked()
{
	camera->cinst->setArray("wbCustomColor", 3, customWhiteBalOld);
	camera->cinst->setInt("wbTemperature", 0);
	ui->comboWB->setCurrentIndex(customWhiteBalIndex);
}

void whiteBalanceDialog::on_cmdMatrix_clicked()
{
	/* Close the color matrix window and re-enable the crosshair */
	if (cw) {
		delete cw;
		cw = NULL;
		crosshair->show();
		ui->cmdMatrix->setText(camera->ButtonsOnLeft ? "Matrix >>" : "<< Matrix");
	}
	/* Disable the crosshair and show the colour matrix window. */
	else {
		cw = new ColorWindow(this, camera);
		cw->show();
		crosshair->hide();
		ui->cmdMatrix->setText(camera->ButtonsOnLeft ? "Matrix <<" : ">> Matrix");

		connect(cw, SIGNAL(applyColorMatrix()), this, SLOT(applyColorMatrix()));
		connect(cw, SIGNAL(applyWhiteBalance()), this, SLOT(applyWhiteBalance()));
	}
}

void whiteBalanceDialog::saveColor(void)
{
	int i;
	for (i = 0; i<9; i++)
	{
		saveMatrix[i] = camera->colorCalMatrix[i];
	}
	saveWbTemp = camera->cinst->getProperty("wbTemperature", 5600).toUInt();
}

void whiteBalanceDialog::restoreColor(void)
{
	int i;
	for (i = 0; i<9; i++)
	{
		camera->colorCalMatrix[i] = saveMatrix[i];
	}
	camera->cinst->setArray("wbCustomColor", 3, customWhiteBalOld);
	camera->cinst->setInt("wbTemperature", saveWbTemp);
}
