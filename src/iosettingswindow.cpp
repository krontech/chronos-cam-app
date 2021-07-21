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
	config.insert("invert", QVariant(ui->chkIO1Invert->isChecked()));
	config.insert("source", QVariant("io1"));
}

void IOSettingsWindow::setIoConfig2(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO2Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO2Invert->isChecked()));
	config.insert("source", QVariant("io2"));
}

void IOSettingsWindow::setIoConfig3(QVariantMap &config)
{
	config.insert("debounce", QVariant(ui->chkIO3Debounce->isChecked()));
	config.insert("invert", QVariant(ui->chkIO3Invert->isChecked()));
	config.insert("source", QVariant("io3"));
}

/* Generate the IO configuration and apply it via the API. */
void IOSettingsWindow::setIoSettings()
{
	QSettings settings;

	QVariantMap values;

	QVariantMap orConfig[3];
	QVariantMap andConfig;
	QVariantMap xorConfig;

	QVariantMap triggerConfig;
	QVariantMap shutterConfig;

	QVariantMap io1config;
	QVariantMap io2config;

	QVariantMap startConfig;
	QVariantMap stopConfig;
	QVariantMap delayConfig;

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
	if (ui->radioIO3TrigIn->isChecked()) nRecTrig++;
	if (ui->radioIO1TriggeredShutter->isChecked()) nExpTrig++;
	if (ui->radioIO2TriggeredShutter->isChecked()) nExpTrig++;
	if (ui->radioIO3TriggeredShutter->isChecked()) nExpTrig++;
	if (ui->radioIO1ShutterGating->isChecked()) nShGate++;
	if (ui->radioIO2ShutterGating->isChecked()) nShGate++;
	if (ui->radioIO3ShutterGating->isChecked()) nShGate++;

	/* Prepare the combinatorial block. */
	/* Special case for Toggle Trigger mode, which can start and stop recordings. */
	if (ui->radioIO1ToggleTrig->isChecked()) {
		orConfig[0].insert("source", QVariant("io1"));
		orConfig[0].insert("debounce", QVariant(true));
		orConfig[0].insert("invert", QVariant(true));

		orConfig[1].insert("source", QVariant("none"));
		orConfig[1].insert("debounce", QVariant(false));
		orConfig[1].insert("invert", QVariant(false));

		orConfig[2].insert("source", QVariant("none"));
		orConfig[2].insert("debounce", QVariant(false));
		orConfig[2].insert("invert", QVariant(false));

		xorConfig.insert("source", QVariant("none"));
		xorConfig.insert("debounce", QVariant(false));
		xorConfig.insert("invert", QVariant(false));

		andConfig.insert("source", QVariant("delay"));
		andConfig.insert("debounce", QVariant(false));
		andConfig.insert("invert", QVariant(false));

		triggerConfig.insert("source", QVariant("io1"));
		triggerConfig.insert("debounce", QVariant(true));
		triggerConfig.insert("invert", QVariant(true));

		shutterConfig.insert("source", QVariant("none"));
		shutterConfig.insert("debounce", QVariant(false));
		shutterConfig.insert("invert", QVariant(false));

		startConfig.insert("source", QVariant("comb"));
		startConfig.insert("debounce", QVariant(true));
		startConfig.insert("invert", QVariant(false));

		stopConfig.insert("source", QVariant("none"));
		stopConfig.insert("debounce", QVariant(false));
		stopConfig.insert("invert", QVariant(false));

		delayConfig.insert("source", QVariant("recording"));
		delayConfig.insert("debounce", QVariant(false));
		delayConfig.insert("invert", QVariant(true));
	}
	else {
		orConfig[0].insert("source", QVariant("none"));
		orConfig[1].insert("source", QVariant("none"));
		orConfig[2].insert("source", QVariant("none"));
		andConfig.insert("source", QVariant("alwaysHigh"));
		xorConfig.insert("source", QVariant("none"));

		triggerConfig.insert("source", QVariant("none"));
		shutterConfig.insert("source", QVariant("none"));

		startConfig.insert("source", QVariant("none"));
		startConfig.insert("debounce", QVariant(false));
		startConfig.insert("invert", QVariant(false));

		stopConfig.insert("source", QVariant("none"));
		stopConfig.insert("debounce", QVariant(false));
		stopConfig.insert("invert", QVariant(false));

		delayConfig.insert("source", QVariant("none"));
		delayConfig.insert("debounce", QVariant(false));
		delayConfig.insert("invert", QVariant(false));
	}


	/* Prepare the combinatorial block for recording end trigger. */
	if (nRecTrig > 1) {
		if (ui->radioIO1TrigIn->isChecked()) setIoConfig1(orConfig[0]);
		if (ui->radioIO2TrigIn->isChecked()) setIoConfig2(orConfig[1]);
		if (ui->radioIO3TrigIn->isChecked()) setIoConfig3(orConfig[2]);
		triggerConfig.insert("source", QVariant("comb"));
	}
	/* Otherwise directly connect the recording end trigger. */
	else if (ui->radioIO1TrigIn->isChecked()) setIoConfig1(triggerConfig);
	else if (ui->radioIO2TrigIn->isChecked()) setIoConfig2(triggerConfig);
	else if (ui->radioIO3TrigIn->isChecked()) setIoConfig3(triggerConfig);

	/* Connect the shutter to at most one of the inputs. */
	if (ui->radioIO1TriggeredShutter->isChecked()) setIoConfig1(shutterConfig);
	else if (ui->radioIO2TriggeredShutter->isChecked()) setIoConfig2(shutterConfig);
	else if (ui->radioIO3TriggeredShutter->isChecked()) setIoConfig3(shutterConfig);
	else if (ui->radioIO1ShutterGating->isChecked()) setIoConfig1(shutterConfig);
	else if (ui->radioIO2ShutterGating->isChecked()) setIoConfig2(shutterConfig);
	else if (ui->radioIO3ShutterGating->isChecked()) setIoConfig3(shutterConfig);

	/* Configure the IO1 output drivers. */
	if (ui->radioIO1FSOut->isChecked()) {
		io1config.insert("debounce", QVariant(ui->chkIO1Debounce->isChecked()));
		io1config.insert("invert", QVariant(ui->chkIO1Invert->isChecked()));
		io1config.insert("drive", QVariant(2)); /* Force 20mA output when driving */
		io1config.insert("source", "timingIo");
	}
	else if (ui->radioIO1ToggleTrig->isChecked()) {
		io1config.insert("source", "alwaysHigh");
		io1config.insert("debounce", QVariant(false));
		io1config.insert("invert", QVariant(false));
		io1config.insert("drive", QVariant(2));
	}
	else {
		io1config.insert("source", "alwaysHigh");
		io1config.insert("debounce", QVariant(false));
		io1config.insert("invert", QVariant(false));
		io1config.insert("drive", QVariant(io1pull));
	}

	/* Configure the IO2 output drivers. */
	if (ui->radioIO2FSOut->isChecked()) {
		io2config.insert("debounce", QVariant(ui->chkIO2Debounce->isChecked()));
		io2config.insert("invert", QVariant(ui->chkIO2Invert->isChecked()));
		io2config.insert("drive", QVariant(2)); /* Force 20mA output when driving */
		io2config.insert("source", "timingIo");
	}
	else if (ui->radioIO1ToggleTrig->isChecked()) {
		io2config.insert("source", "alwaysHigh");
		io2config.insert("debounce", QVariant(false));
		io2config.insert("invert", QVariant(false));
		io2config.insert("drive", QVariant(0));
	}
	else {
		io2config.insert("source", "alwaysHigh");
		io2config.insert("debounce", QVariant(false));
		io2config.insert("invert", QVariant(false));
		io2config.insert("drive", QVariant(io2pull));
	}

	/* Save the UI state of invert and debounce and drive strength. */
	settings.setValue("io/io1invert", ui->chkIO1Invert->isChecked());
	settings.setValue("io/io1debounce", ui->chkIO1Debounce->isChecked());
	settings.setValue("io/io1drive", io1pull);
	settings.setValue("io/io2invert", ui->chkIO2Invert->isChecked());
	settings.setValue("io/io2debounce", ui->chkIO2Debounce->isChecked());
	settings.setValue("io/io2drive", io2pull);
	settings.setValue("io/io3invert", ui->chkIO3Invert->isChecked());
	settings.setValue("io/io3debounce", ui->chkIO3Debounce->isChecked());

	/* HACK: This feels like the wrong place to be selecting the exposure program. */
	if (nShGate) {
		values.insert("exposureMode", QVariant("shutterGating"));
	} else if (nExpTrig) {
		values.insert("exposureMode", QVariant("frameTrigger"));
	} else {
		values.insert("exposureMode", QVariant("normal"));
	}

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

	/* Load extra parameters for the trigger toggle mode (trigger starts and stops recordings) */
	values.insert("ioMappingStartRec", QVariant(startConfig));
	values.insert("ioMappingStopRec", QVariant(stopConfig));
	values.insert("ioMappingDelay", QVariant(delayConfig));

	/* Necessary delay to prevent toggle mode from immediately starting again after stopping */
	if(ui->radioIO1ToggleTrig->isChecked()){
		values.insert("ioDelayTime", QVariant(0.50));
	} else {
		values.insert("ioDelayTime", QVariant(0));
	}

	/* Apply the settings via D-Bus */
	camera->cinst->setPropertyGroup(values);
}

void IOSettingsWindow::getIoTriggerConfig(QVariantMap &config)
{
	QString source = config["source"].toString();
	if (source == "io1") {
		ui->radioIO1TrigIn->setChecked(true);
		ui->chkIO1Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO1Invert->setChecked(config["invert"].toBool());
	}
	else if (source == "io2") {
		ui->radioIO2TrigIn->setChecked(true);
		ui->chkIO2Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO2Invert->setChecked(config["invert"].toBool());
	}
	else if (source == "io3") {
		ui->radioIO3TrigIn->setChecked(true);
		ui->chkIO3Debounce->setChecked(config["debounce"].toBool());
		ui->chkIO3Invert->setChecked(config["invert"].toBool());
	}
}

void IOSettingsWindow::getIoShutterConfig(QVariantMap &config, QString expMode)
{
	QString source = config["source"].toString();
	bool gating = (expMode == "shutterGating");
	bool trigger = (expMode == "frameTrigger");
	bool debounce = config["debounce"].toBool();
	bool invert = config["invert"].toBool();

	if (source == "io1") {
		ui->radioIO1ShutterGating->setChecked(gating);
		ui->radioIO1TriggeredShutter->setChecked(trigger);
		ui->chkIO1Debounce->setChecked(debounce);
		ui->chkIO1Invert->setChecked(invert);
	}
	else if (source == "io2") {
		ui->radioIO2ShutterGating->setChecked(gating);
		ui->radioIO2TriggeredShutter->setChecked(trigger);
		ui->chkIO2Debounce->setChecked(debounce);
		ui->chkIO2Invert->setChecked(invert);
	}
	else if (source == "io3") {
		ui->radioIO3ShutterGating->setChecked(gating);
		ui->radioIO3TriggeredShutter->setChecked(trigger);
		ui->chkIO3Debounce->setChecked(debounce);
		ui->chkIO3Invert->setChecked(invert);
	}
}

void IOSettingsWindow::getIoSettings()
{
	QSettings settings;

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
		"exposureMode",
		"ioMappingStartRec",
		"ioMappingStopRec",
		"ioMappingDelay",
		"ioDelayTime"
	};

	QVariantMap orConfig[3];
	QVariantMap triggerConfig;
	QVariantMap shutterConfig;
	QVariantMap startConfig;
	QVariantMap stopConfig;
	QVariantMap delayConfig;

	QVariantMap io1config;
	QVariantMap io2config;

	QVariantMap ioMapping = camera->cinst->getPropertyGroup(names);

	int io1pull, io2pull;

	ioMapping["ioMappingCombOr1"].value<QDBusArgument>() >> orConfig[0];
	ioMapping["ioMappingCombOr2"].value<QDBusArgument>() >> orConfig[1];
	ioMapping["ioMappingCombOr3"].value<QDBusArgument>() >> orConfig[2];
	ioMapping["ioMappingTrigger"].value<QDBusArgument>() >> triggerConfig;
	ioMapping["ioMappingShutter"].value<QDBusArgument>() >> shutterConfig;
	ioMapping["ioMappingStartRec"].value<QDBusArgument>() >> startConfig;
	ioMapping["ioMappingStopRec"].value<QDBusArgument>() >> stopConfig;
	ioMapping["ioMappingDelay"].value<QDBusArgument>() >> delayConfig;

	ioMapping["ioMappingIo1"].value<QDBusArgument>() >> io1config;
	ioMapping["ioMappingIo2"].value<QDBusArgument>() >> io2config;
	io1pull = io1config["drive"].toInt();

	/* Start with nothing selected. */
	ui->radioIO1None->setChecked(true);
	ui->radioIO2None->setChecked(true);
	ui->radioIO3None->setChecked(true);
	/* Load the UI cached values for invert and debounce. */
	ui->chkIO1Invert->setChecked(settings.value("io/io1invert", false).toBool());
	ui->chkIO1Debounce->setChecked(settings.value("io/io1debounce", false).toBool());
	ui->chkIO2Invert->setChecked(settings.value("io/io2invert", false).toBool());
	ui->chkIO2Debounce->setChecked(settings.value("io/io2debounce", false).toBool());
	ui->chkIO3Invert->setChecked(settings.value("io/io3invert", false).toBool());
	ui->chkIO3Debounce->setChecked(settings.value("io/io3debounce", false).toBool());

	/* Get the threshold values */
	ui->spinIO1Thresh->setValue(ioMapping.value("ioThresholdIo1", 2.5).toDouble());
	ui->spinIO2Thresh->setValue(ioMapping.value("ioThresholdIo2", 2.5).toDouble());

	/* Load config if configured as an output. */
	if (io1config["source"].toString() == "timingIo") {
		ui->chkIO1Invert->setChecked(io1config["invert"].toBool());
		ui->chkIO1Debounce->setChecked(io1config["debounce"].toBool());
		ui->radioIO1FSOut->setChecked(true);
		io1pull = settings.value("io/io1drive", 0).toInt();
	}
	else {
		io1pull = io1config["drive"].toInt();
	}
	if (io2config["source"].toString() == "timingIo") {
		ui->chkIO2Invert->setChecked(io2config["invert"].toBool());
		ui->chkIO2Debounce->setChecked(io2config["debounce"].toBool());
		ui->radioIO2FSOut->setChecked(true);
		io2pull = settings.value("io/io1drive", 0).toInt();
	}
	else {
		io2pull = io2config["drive"].toInt();
	}
	ui->chkIO1WeakPull->setChecked((io1pull & 1) != 0);
	ui->chkIO1Pull->setChecked((io1pull & 2) != 0);
	ui->chkIO2Pull->setChecked(io2pull != 0);

	/* Load the record-end configuration. */
	if (triggerConfig["source"].toString() == "comb") {
		getIoTriggerConfig(orConfig[0]);
		getIoTriggerConfig(orConfig[1]);
		getIoTriggerConfig(orConfig[2]);
	} else if (startConfig["source"].toString() == "comb" && delayConfig["source"].toString() == "recording") {
		/* In this case, we're in the trigger toggle mode */
		ui->radioIO1ToggleTrig->setChecked(true);
		ui->chkIO1Debounce->setChecked(io1config["debounce"].toBool());
		ui->chkIO1Invert->setChecked(io1config["invert"].toBool());
	}
	else {
		getIoTriggerConfig(triggerConfig);
	}

	/* Load the exposure trigger or shutter gating configuration. */
	getIoShutterConfig(shutterConfig, ioMapping["exposureMode"].toString());
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
		ui->radioIO3TriggeredShutter->setEnabled(false);
		ui->radioIO2ShutterGating->setEnabled(false);
		ui->radioIO3ShutterGating->setEnabled(false);
	}
	else
	{
		ui->radioIO2TriggeredShutter->setEnabled(true);
		ui->radioIO3TriggeredShutter->setEnabled(true);
		ui->radioIO2ShutterGating->setEnabled(true);
		ui->radioIO3ShutterGating->setEnabled(true);
	}
}

void IOSettingsWindow::on_radioIO1ShutterGating_toggled(bool checked)
{
	on_radioIO1TriggeredShutter_toggled(checked);
}

void IOSettingsWindow::on_radioIO1FSOut_toggled(bool checked)
{
	ui->chkIO1Pull->setEnabled(!checked);
	ui->chkIO1WeakPull->setEnabled(!checked);
}

void IOSettingsWindow::on_radioIO1ToggleTrig_toggled(bool checked)
{
    ui->chkIO1Pull->setEnabled(!checked);
    ui->chkIO1WeakPull->setEnabled(!checked);
    ui->chkIO1Debounce->setEnabled(!checked);
    ui->chkIO1Invert->setEnabled(!checked);
}

void IOSettingsWindow::on_radioIO2FSOut_toggled(bool checked)
{
    ui->chkIO2Pull->setEnabled(!checked);
}

void IOSettingsWindow::on_radioIO2TriggeredShutter_toggled(bool checked)
{
	if(checked)
	{
		ui->radioIO1TriggeredShutter->setEnabled(false);
		ui->radioIO3TriggeredShutter->setEnabled(false);
		ui->radioIO1ShutterGating->setEnabled(false);
		ui->radioIO3ShutterGating->setEnabled(false);
	}
	else
	{
		ui->radioIO1TriggeredShutter->setEnabled(true);
		ui->radioIO3TriggeredShutter->setEnabled(true);
		ui->radioIO1ShutterGating->setEnabled(true);
		ui->radioIO3ShutterGating->setEnabled(true);
	}
}

void IOSettingsWindow::on_radioIO2ShutterGating_toggled(bool checked)
{
	on_radioIO2TriggeredShutter_toggled(checked);
}

void IOSettingsWindow::on_radioIO3TriggeredShutter_toggled(bool checked)
{
	if(checked)
	{
		ui->radioIO1TriggeredShutter->setEnabled(false);
		ui->radioIO2TriggeredShutter->setEnabled(false);
		ui->radioIO1ShutterGating->setEnabled(false);
		ui->radioIO2ShutterGating->setEnabled(false);
	}
	else
	{
		ui->radioIO1TriggeredShutter->setEnabled(true);
		ui->radioIO2TriggeredShutter->setEnabled(true);
		ui->radioIO1ShutterGating->setEnabled(true);
		ui->radioIO2ShutterGating->setEnabled(true);
	}
}

void IOSettingsWindow::on_radioIO3ShutterGating_toggled(bool checked)
{
	on_radioIO3TriggeredShutter_toggled(checked);
}
