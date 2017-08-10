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

	char str[100];

	sprintf(str, "%4.2f", camera->io->getThreshold(1));
	ui->lineIO1Thresh->setText(str);

	sprintf(str, "%4.2f", camera->io->getThreshold(2));
	ui->lineIO2Thresh->setText(str);

	sprintf(str, "%d", camera->io->getTriggerDelayFrames());
	ui->lineTrigDelayFrames->setText(str);

	ui->radioIO1None->setChecked(true);
	ui->radioIO2None->setChecked(true);

	ui->chkIO1InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 0));
	ui->chkIO2InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 1));
	ui->chkIO3InvertIn->setChecked(camera->io->getTriggerInvert() & (1 << 2));

	ui->radioIO1TrigIn->setChecked(camera->io->getTriggerEnable() & (1 << 0));
	ui->radioIO2TrigIn->setChecked(camera->io->getTriggerEnable() & (1 << 1));
	ui->chkIO3TriggerInEn->setChecked(camera->io->getTriggerEnable() & (1 << 2));
	ui->radioIO1TriggeredShutter->setChecked(camera->io->getTriggeredExpEnable() && (camera->io->getExtShutterSrcEn() & (1 << 0)));
	ui->radioIO1ShutterGating->setChecked(camera->io->getShutterGatingEnable());

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

void IOSettingsWindow::on_cmdApply_clicked()
{
	camera->io->setTriggerInvert(ui->chkIO3InvertIn->isChecked() << 2 | ui->chkIO2InvertIn->isChecked() << 1 | ui->chkIO1InvertIn->isChecked());
	camera->io->setTriggerEnable(ui->chkIO3TriggerInEn->isChecked() << 2 | ui->radioIO2TrigIn->isChecked() << 1 |  ui->radioIO1TrigIn->isChecked());
	camera->io->setTriggerDebounceEn(ui->chkIO3Debounce->isChecked() << 2 | ui->chkIO2Debounce->isChecked() << 1 | ui->chkIO1Debounce->isChecked());

	camera->io->setOutSource(ui->radioIO2FSOut->isChecked() << 2 | ui->radioIO1FSOut->isChecked() << 1);
	camera->io->setOutLevel(ui->chkIO2Pull->isChecked() << 2 | ui->chkIO1Pull->isChecked() << 1 | ui->chkIO1WeakPull->isChecked());
	camera->io->setOutInvert(ui->chkIO2InvertOut->isChecked() << 2 | ui->chkIO1InvertOut->isChecked() << 1 | ui->chkIO1InvertOut->isChecked());


	camera->io->setThreshold(1, ui->lineIO1Thresh->text().toDouble());
	camera->io->setThreshold(2, ui->lineIO2Thresh->text().toDouble());

	camera->io->setTriggerDelayFrames(ui->lineTrigDelayFrames->text().toUInt());

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
