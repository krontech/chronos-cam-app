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

IOSettingsWindow::IOSettingsWindow(QWidget *parent, Camera * cameraInst) :
	QWidget(parent),
	ui(new Ui::IOSettingsWindow)
{
	ui->setupUi(this);
	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	this->move(0,0);

	connect(ui->cmdCancel, SIGNAL(clicked()), this, SLOT(close()));

	camera = cameraInst;

	getIoSettings();

	lastIn = getIoLevels();

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
	UInt32 nextIn = getIoLevels();
	if(nextIn != lastIn)
	{
		lastIn = nextIn;
		updateIO();
	}
}

void IOSettingsWindow::updateIO()
{
	char str[100];
	sprintf(str, "IO1: %s\r\nIO2: %s\r\nIn3: %s", lastIn & (1 << 0) ? "Hi" : "Lo",
												lastIn & (1 << 1) ? "Hi" : "Lo",
												lastIn & (1 << 2) ? "Hi" : "Lo");
	ui->lblStatus->setText(str);
}

void IOSettingsWindow::on_cmdOK_clicked()
{
	on_cmdApply_clicked();
	close();
}

void IOSettingsWindow::setIoConfig1(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO1Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO1InvertIn->isChecked()));
	config.insert("source", QVariant("io1"));
}

void IOSettingsWindow::setIoConfig2(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO2Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO2InvertIn->isChecked()));
	config.insert("source", QVariant("io2"));
}

void IOSettingsWindow::setIoConfig3(QVariantMap &config)
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

	QVariantMap triggerConfig;
	QVariantMap shutterConfig;

	QVariantMap io1config;
	QVariantMap io2config;

	int io1pull = 0;
	int io2pull = 0;

	int nRecTrig = 0;
	int nExpTrig = 0;
	int nShGate = 0;

	if (ui->chkIO1WeakPull->isChecked()) io1pull += 1;
	if (ui->chkIO1Pull->isChecked()) io1pull += 2;
	if (ui->chkIO2Pull->isChecked()) io2pull += 1;

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

	triggerConfig.insert("source", QVariant("none"));
	shutterConfig.insert("shutter", QVariant("none"));

	/* Prepare the combinatorial block for recording end trigger. */
	if (nRecTrig > 1) {
		if (ui->radioIO1TrigIn->isChecked()) setIoConfig1(orConfig[0]);
		if (ui->radioIO2TrigIn->isChecked()) setIoConfig2(orConfig[1]);
		if (ui->chkIO3TriggerInEn->isChecked()) setIoConfig3(orConfig[2]);
		triggerConfig.insert("source", QVariant("comb"));
	}
	/* Otherwise directly connect the recording end trigger. */
	else if (ui->radioIO1TrigIn->isChecked()) setIoConfig1(triggerConfig);
	else if (ui->radioIO2TrigIn->isChecked()) setIoConfig2(triggerConfig);
	else if (ui->chkIO3TriggerInEn->isChecked()) setIoConfig3(triggerConfig);

	/* Prepare the combinatorial block for exposure trigger and/or shutter gating. */
	if ((nExpTrig + nShGate) > 1) {
		if (ui->radioIO1TriggeredShutter->isChecked()) setIoConfig1(orConfig[0]);
		else if (ui->radioIO1ShutterGating->isChecked()) setIoConfig1(orConfig[0]);
		if (ui->radioIO2TriggeredShutter->isChecked()) setIoConfig2(orConfig[1]);
		else if (ui->radioIO2ShutterGating->isChecked()) setIoConfig2(orConfig[1]);
		shutterConfig.insert("source", QVariant("comb"));
	}
	/* Otherwise directly connect the frame trigger. */
	else if (ui->radioIO1TriggeredShutter->isChecked()) setIoConfig1(shutterConfig);
	else if (ui->radioIO2TriggeredShutter->isChecked()) setIoConfig2(shutterConfig);
	else if (ui->radioIO1ShutterGating->isChecked()) setIoConfig1(shutterConfig);
	else if (ui->radioIO2ShutterGating->isChecked()) setIoConfig2(shutterConfig);

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
	io1config.insert("drive", QVariant(io1pull));
	io1config.insert("source", QVariant(ui->radioIO1FSOut->isChecked() ? "timingIo" : "alwaysHigh"));
	io2config.insert("invert", QVariant(ui->chkIO2InvertOut->isChecked()));
	io2config.insert("drive", QVariant(io2pull));
	io2config.insert("source", QVariant(ui->radioIO2FSOut->isChecked() ? "timingIo" : "alwaysHigh"));

	/* Load up the IO mapping configuration */
	values.insert("ioMappingCombOr1", QVariant(orConfig[0]));
	values.insert("ioMappingCombOr2", QVariant(orConfig[1]));
	values.insert("ioMappingCombOr3", QVariant(orConfig[2]));
	values.insert("ioMappingCombAnd", QVariant(andConfig));
	values.insert("ioMappingCombXor", QVariant(xorConfig));
	values.insert("ioMappingTrigger", QVariant(triggerConfig));
	values.insert("ioMappingShutter", QVariant(shutterConfig));
	values.insert("ioMappingIo1", QVariant(io1config));
	values.insert("ioMappingIo2", QVariant(io2config));
	values.insert("ioThresholdIo1", QVariant(ui->spinIO1Thresh->value()));
	values.insert("ioThresholdIo2", QVariant(ui->spinIO2Thresh->value()));

	/* Apply the settings via D-Bus */
	camera->cinst->setPropertyGroup(values);
}

void IOSettingsWindow::getIoTriggerConfig(QVariantMap &config)
{
	QString source = config["source"].toString();
	if (source == "io1") {
		ui->radioIO1TrigIn->setChecked(true);
		ui->chkIO1Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO1InvertIn->setChecked(config["invert"].toBool());
	}
	else if (source == "io2") {
		ui->radioIO2TrigIn->setChecked(true);
		ui->chkIO2Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO2InvertIn->setChecked(config["invert"].toBool());
	}
	else if (source == "io3") {
		ui->chkIO3TriggerInEn->setChecked(true);
		ui->chkIO3Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO3InvertIn->setChecked(config["invert"].toBool());
	}
}

void IOSettingsWindow::getIoShutterConfig(QVariantMap &config, QString expMode)
{
	QString source = config["source"].toString();
	if (source == "io1") {
		if (expMode == "shutterGating") {
			ui->radioIO1ShutterGating->setChecked(true);
		} else {
			ui->radioIO1TriggeredShutter->setChecked(true);
		}

		ui->chkIO1Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO1InvertIn->setChecked(config["invert"].toBool());
	}
	else if (source == "io2") {
		if (expMode == "shutterGating") {
			ui->radioIO2ShutterGating->setChecked(true);
		} else {
			ui->radioIO2TriggeredShutter->setChecked(true);
		}

		ui->chkIO2Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO2InvertIn->setChecked(config["invert"].toBool());
	}
}

void IOSettingsWindow::getIoSettings()
{
	QStringList names = {
		"ioMappingCombOr1",
		"ioMappingCombOr2",
		"ioMappingCombOr3",
		"ioMappingTrigger",
		"ioMappingShutter",
		"ioMappingIo1",
		"ioMappingIo2",
		"ioThresholdIo1",
		"ioThresholdIo2",
		"exposureMode"
	};

	QVariantMap orConfig[3];
	QVariantMap triggerConfig;
	QVariantMap shutterConfig;

	QVariantMap io1config;
	QVariantMap io2config;

	QVariantMap ioMapping = camera->cinst->getPropertyGroup(names);

	int io1pull;

	ioMapping["ioMappingCombOr1"].value<QDBusArgument>() >> orConfig[0];
	ioMapping["ioMappingCombOr2"].value<QDBusArgument>() >> orConfig[1];
	ioMapping["ioMappingCombOr3"].value<QDBusArgument>() >> orConfig[2];
	ioMapping["ioMappingTrigger"].value<QDBusArgument>() >> triggerConfig;
	ioMapping["ioMappingShutter"].value<QDBusArgument>() >> shutterConfig;

	ioMapping["ioMappingIo1"].value<QDBusArgument>() >> io1config;
	ioMapping["ioMappingIo2"].value<QDBusArgument>() >> io2config;

	/* Start with nothing selected. */
	ui->radioIO1None->setChecked(true);
	ui->radioIO2None->setChecked(true);
	ui->chkIO3TriggerInEn->setChecked(false);

	ui->chkIO1InvertOut->setChecked(io1config["invert"].toBool());
	ui->radioIO1FSOut->setChecked(io1config["source"].toString() == "timingIo");
	ui->chkIO2InvertOut->setChecked(io2config["invert"].toBool());
	ui->radioIO2FSOut->setChecked(io2config["source"].toString() == "timingIo");
	ui->chkIO2Pull->setChecked(io2config["drive"].toInt() != 0);

	ui->spinIO1Thresh->setValue(ioMapping.value("ioThresholdIo1", 2.5).toDouble());
	ui->spinIO2Thresh->setValue(ioMapping.value("ioThresholdIo2", 2.5).toDouble());

	io1pull = io1config["drive"].toInt();
	ui->chkIO1WeakPull->setChecked((io1pull & 1) != 0);
	ui->chkIO1Pull->setChecked((io1pull & 2) != 0);

	/* Load the record-end configuration. */
	if (triggerConfig["source"].toString() == "comb") {
		getIoTriggerConfig(orConfig[0]);
		getIoTriggerConfig(orConfig[1]);
		getIoTriggerConfig(orConfig[2]);
	} else {
		getIoTriggerConfig(triggerConfig);
	}

	/* Load the exposure trigger or shutter gating configuration. */
	if (shutterConfig["source"].toString() == "comb") {
		QString expMode = ioMapping["exposureMode"].toString();
		getIoShutterConfig(orConfig[0], expMode);
		getIoShutterConfig(orConfig[1], expMode);
		getIoShutterConfig(orConfig[2], expMode);
	} else {
		getIoShutterConfig(shutterConfig, ioMapping["exposureMode"].toString());
	}
}

UInt32 IOSettingsWindow::getIoLevels(void)
{
	UInt32 ioLevelBitmap = 0;
	QVariant qv = camera->cinst->getProperty("ioSourceStatus");
	if (qv.isValid()) {
		QVariantMap ioLevelMap;
		qv.value<QDBusArgument>() >> ioLevelMap;

		if (ioLevelMap["io1"].toBool()) ioLevelBitmap |= (1 << 0);
		if (ioLevelMap["io2"].toBool()) ioLevelBitmap |= (1 << 1);
		if (ioLevelMap["io3"].toBool()) ioLevelBitmap |= (1 << 2);
	}
	return ioLevelBitmap;
}

void IOSettingsWindow::on_cmdApply_clicked()
{
	/* Do it the new way using the D-Bus API */
	setIoSettings();
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
