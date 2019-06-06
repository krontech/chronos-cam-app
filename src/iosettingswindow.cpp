/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include <qdebug.h>
#include "iosettingswindow.h"
#include "ui_iosettingswindow.h"

extern bool pych;

IOSettingsWindow::IOSettingsWindow(QWidget *parent, Camera * cameraInst) :
	QWidget(parent),
	ui(new Ui::IOSettingsWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	this->move(0,0);

	connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;

	ui->spinIO1Thresh->setValue(camera->io->getThreshold(1));

	ui->spinIO2Thresh->setValue(camera->io->getThreshold(2));

	ui->radioIO1None->setChecked(true);
	ui->radioIO2None->setChecked(true);

	ui->chkIO1InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 0));
	ui->chkIO2InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 1));
	ui->chkIO3InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 2));

	ui->radioIO1TrigIn->setChecked(camera->io->getTriggerEnable() & (1 << 0));
	ui->radioIO2TrigIn->setChecked(camera->io->getTriggerEnable() & (1 << 1));
	ui->chkIO3TriggerInEn->setChecked(camera->io->getTriggerEnable() & (1 << 2));
	ui->radioIO1TriggeredShutter->setChecked(camera->io->getTriggeredExpEnable()  && (camera->io->getExtShutterSrcEn() & (1 << 0)));
	ui->radioIO1ShutterGating->   setChecked(camera->io->getShutterGatingEnable() && (camera->io->getExtShutterSrcEn() & (1 << 0)));
	ui->radioIO2TriggeredShutter->setChecked(camera->io->getTriggeredExpEnable()  && (camera->io->getExtShutterSrcEn() & (1 << 1)));
	ui->radioIO2ShutterGating->   setChecked(camera->io->getShutterGatingEnable() && (camera->io->getExtShutterSrcEn() & (1 << 1)));

	ui->chkIO1Debounce->setChecked(camera->io->getTriggerDebounceEn() & (1 << 0));
	ui->chkIO2Debounce->setChecked(camera->io->getTriggerDebounceEn() & (1 << 1));
	ui->chkIO3Debounce->setChecked(camera->io->getTriggerDebounceEn() & (1 << 2));

	ui->radioIO1FSOut->setChecked(camera->io->getOutSource() & (1 << 1));
	ui->radioIO2FSOut->setChecked(camera->io->getOutSource() & (1 << 2));

	ui->chkIO1Pull->setChecked(camera->io->getOutLevel() & (1 << 1));
	ui->chkIO1WeakPull->setChecked(camera->io->getOutLevel() & (1 << 0));
	ui->chkIO2Pull->setChecked(camera->io->getOutLevel() & (1 << 2));

	ui->chkIO1InvertOut->setChecked(camera->io->getOutInvert() & (1 << 1));
	ui->chkIO2InvertOut->setChecked(camera->io->getOutInvert() & (1 << 2));

	lastIn = camera->io->getIn();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_updateTimer()));
	timer->start(30);

	updateIO();


}

IOSettingsWindow::~IOSettingsWindow()
{
	delete ui;
}

void IOSettingsWindow::on_updateTimer()
{
	if(camera->io->getIn() != lastIn)
	{
		updateIO();
		lastIn = camera->io->getIn();
	}
}

void IOSettingsWindow::updateIO()
{
	char str[100];
	UInt32 input = camera->io->getIn();

	sprintf(str, "IO1: %s\r\nIO2: %s\r\nIn3: %s", input & (1 << 0) ? "Hi" : "Lo",
												input & (1 << 1) ? "Hi" : "Lo",
												input & (1 << 2) ? "Hi" : "Lo");

	ui->lblStatus->setText(str);
}

void IOSettingsWindow::on_cmdOK_clicked()
{
	on_cmdApply_clicked();
	close();
}

void IOSettingsWindow::getIoConfig1(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO1Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO1InvertIn->isChecked()));
	config.insert("source", QVariant("io1"));
}

void IOSettingsWindow::getIoConfig2(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO2Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO2InvertIn->isChecked()));
	config.insert("source", QVariant("io2"));
}

void IOSettingsWindow::getIoConfig3(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO3Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO3InvertIn->isChecked()));
	config.insert("source", QVariant("io3"));
}

/* Generate the IO configuration and apply it via the API. */
void IOSettingsWindow::setIoSettings()
{
	QVariantMap values;

	QVariantMap orConfig[3];
	QVariantMap andConfig;
	QVariantMap xorConfig;

	QVariantMap stopConfig;
	QVariantMap shutterConfig;

	QVariantMap io1config;
	QVariantMap io2config;
	QVariantMap io1thresh;
	QVariantMap io2thresh;

	int io1pull = 0;
	int io2pull = 0;

	int nRecTrig = 0;
	int nExpTrig = 0;
	int nShGate = 0;

	/* Setup the input flags for IO1. */
	if (ui->chkIO1WeakPull->isChecked()) io1pull += 1;
	if (ui->chkIO1Pull->isChecked()) io1pull += 2;

	/* If multiple inputs are mapped to the same thing, we need to use a combinatorial block. */
	if (ui->radioIO1TrigIn->isChecked()) nRecTrig++;
	if (ui->radioIO2TrigIn->isChecked()) nRecTrig++;
	if (ui->chkIO3TriggerInEn->isChecked()) nRecTrig++;
	if (ui->radioIO1TriggeredShutter->isChecked()) nExpTrig++;
	if (ui->radioIO2TriggeredShutter->isChecked()) nExpTrig++;
	if (ui->radioIO1ShutterGating->isChecked()) nShGate++;
	if (ui->radioIO2ShutterGating->isChecked()) nShGate++;

	/* Prepare the combinatorial block. */
	orConfig[0].insert("source", QVariant("none"));
	orConfig[1].insert("source", QVariant("none"));
	orConfig[2].insert("source", QVariant("none"));
	andConfig.insert("source", QVariant("alwaysHigh"));
	xorConfig.insert("source", QVariant("none"));

	stopConfig.insert("source", QVariant("none"));
	shutterConfig.insert("shutter", QVariant("none"));

	/* Prepare the combinatorial block for recording end trigger. */
	if (nRecTrig > 1) {
		if (ui->radioIO1TrigIn->isChecked()) getIoConfig1(orConfig[0]);
		if (ui->radioIO2TrigIn->isChecked()) getIoConfig2(orConfig[1]);
		if (ui->chkIO3TriggerInEn->isChecked()) getIoConfig3(orConfig[2]);
		stopConfig.insert("source", QVariant("comb"));
	}
	/* Otherwise directly connect the recording end trigger. */
	else if (ui->radioIO1TrigIn->isChecked()) getIoConfig1(stopConfig);
	else if (ui->radioIO2TrigIn->isChecked()) getIoConfig2(stopConfig);
	else if (ui->chkIO3TriggerInEn->isChecked()) getIoConfig3(stopConfig);

	/* Prepare the combinatorial block for exposure trigger and/or shutter gating. */
	if ((nExpTrig + nShGate) > 1) {
		if (ui->radioIO1TriggeredShutter->isChecked()) getIoConfig1(orConfig[0]);
		else if (ui->radioIO1ShutterGating->isChecked()) getIoConfig1(orConfig[0]);
		if (ui->radioIO2TriggeredShutter->isChecked()) getIoConfig2(orConfig[1]);
		else if (ui->radioIO2ShutterGating->isChecked()) getIoConfig2(orConfig[1]);
		shutterConfig.insert("source", QVariant("comb"));
	}
	/* Otherwise directly connect the frame trigger. */
	else if (ui->radioIO1TriggeredShutter->isChecked()) getIoConfig1(shutterConfig);
	else if (ui->radioIO2TriggeredShutter->isChecked()) getIoConfig2(shutterConfig);

	/* HACK: This feels like the wrong place to be selecting the exposure program. */
	if (nShGate) {
		values.insert("exposureMode", QVariant("shutterGating"));
	} else if (nExpTrig) {
		values.insert("exposureMode", QVariant("frameTrigger"));
	} else {
		values.insert("exposureMode", QVariant("normal"));
	}

	/* Configure the IO output drivers. */
	io1config.insert("invert", QVariant(ui->chkIO1InvertOut->isChecked()));
	io1config.insert("driveStrength", QVariant(io1pull));
	io1config.insert("source", QVariant(ui->radioIO1FSOut->isChecked() ? "timingIo" : "none"));
	io1thresh.insert("threshold", QVariant(ui->spinIO1Thresh->value()));
	io2config.insert("invert", QVariant(ui->chkIO2InvertOut->isChecked()));
	io2config.insert("driveStrength", QVariant(io2pull));
	io2config.insert("source", QVariant(ui->radioIO2FSOut->isChecked() ? "timingIo" : "none"));
	io2thresh.insert("threshold", QVariant(ui->spinIO2Thresh->value()));

	/* Load up the IO mapping configuration */
	values.insert("ioMappingCombOr1", QVariant(orConfig[0]));
	values.insert("ioMappingCombOr2", QVariant(orConfig[1]));
	values.insert("ioMappingCombOr3", QVariant(orConfig[2]));
	values.insert("ioMappingCombAnd", QVariant(andConfig));
	values.insert("ioMappingCombXor", QVariant(xorConfig));
	values.insert("ioMappingStopRec", QVariant(stopConfig));
	values.insert("ioMappingShutter", QVariant(shutterConfig));
	values.insert("ioMappingIo1", QVariant(io1config));
	values.insert("ioMappingIo2", QVariant(io2config));
	values.insert("ioInputConfigIo1", QVariant(io1thresh));
	values.insert("ioInputConfigIo2", QVariant(io2thresh));

	/* Apply the settings via D-Bus */
	camera->cinst->setPropertyGroup(values);
}

void IOSettingsWindow::on_cmdApply_clicked()
{
	/* Do it the new way using the D-Bus API */
	if (pych) {
		setIoSettings();
		return;
	}

	camera->io->setTriggerInvert(ui->chkIO3InvertIn->isChecked() << 2 | ui->chkIO2InvertIn->isChecked() << 1 | ui->chkIO1InvertIn->isChecked());
	camera->io->setTriggerEnable(ui->chkIO3TriggerInEn->isChecked() << 2 | ui->radioIO2TrigIn->isChecked() << 1 |  ui->radioIO1TrigIn->isChecked());
	camera->io->setTriggerDebounceEn(ui->chkIO3Debounce->isChecked() << 2 | ui->chkIO2Debounce->isChecked() << 1 | ui->chkIO1Debounce->isChecked());

	camera->io->setOutSource(ui->radioIO2FSOut->isChecked() << 2 | ui->radioIO1FSOut->isChecked() << 1);
	camera->io->setOutLevel(ui->chkIO2Pull->isChecked() << 2 | ui->chkIO1Pull->isChecked() << 1 | ui->chkIO1WeakPull->isChecked());
	camera->io->setOutInvert(ui->chkIO2InvertOut->isChecked() << 2 | ui->chkIO1InvertOut->isChecked() << 1 | ui->chkIO1InvertOut->isChecked());


	camera->io->setThreshold(1, ui->spinIO1Thresh->value());
	camera->io->setThreshold(2, ui->spinIO2Thresh->value());

	camera->io->setTriggeredExpEnable(ui->radioIO2TriggeredShutter->isChecked() || ui->radioIO1TriggeredShutter->isChecked());
	
	camera->io->setExtShutterSrcEn(	((ui->radioIO2TriggeredShutter->isChecked() || ui->radioIO2ShutterGating->isChecked()) << 1) |
								 (ui->radioIO1TriggeredShutter->isChecked() || ui->radioIO1ShutterGating->isChecked()));
	
	camera->io->setShutterGatingEnable(ui->radioIO2ShutterGating->isChecked() || ui->radioIO1ShutterGating->isChecked());

	qDebug() << "Trig Ena" << camera->io->getTriggerEnable() << "Trig Inv" << camera->io->getTriggerInvert() << "Trig Deb" << camera->io->getTriggerDebounceEn();
	qDebug() << "Source En" << camera->io->getOutSource() << "Source Level" << camera->io->getOutLevel() << "Source Inv" << camera->io->getOutInvert();
	qDebug() << "IO1 Thresh" << camera->io->getThreshold(1) << "IO2 Thresh" << camera->io->getThreshold(2);
}

void IOSettingsWindow::on_radioIO1TriggeredShutter_toggled(bool checked)
{
	if(checked)
	{
		ui->radioIO2TriggeredShutter->setEnabled(false);
		ui->radioIO2ShutterGating->setEnabled(false);
	}
	else
	{
		ui->radioIO2TriggeredShutter->setEnabled(true);
		ui->radioIO2ShutterGating->setEnabled(true);
	}
}

void IOSettingsWindow::on_radioIO1ShutterGating_toggled(bool checked)
{
	on_radioIO1TriggeredShutter_toggled(checked);
}

void IOSettingsWindow::on_radioIO2TriggeredShutter_toggled(bool checked)
{
	if(checked)
	{
		ui->radioIO1TriggeredShutter->setEnabled(false);
		ui->radioIO1ShutterGating->setEnabled(false);
	}
	else
	{
		ui->radioIO1TriggeredShutter->setEnabled(true);
		ui->radioIO1ShutterGating->setEnabled(true);
	}
}

void IOSettingsWindow::on_radioIO2ShutterGating_toggled(bool checked)
{
	on_radioIO2TriggeredShutter_toggled(checked);
}
