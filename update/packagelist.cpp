#include "packagelist.h"
#include "ui_packagelist.h"

#include <QDesktopWidget>
#include <QTextEdit>

PackageList::PackageList(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PackageList)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window /*| Qt::WindowStaysOnTopHint*/ | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_DeleteOnClose);

	/* Resize the window to use the entire screen. */
	QRect screen = QApplication::desktop()->screenGeometry();
	move(0, 0);
	resize(screen.width(), screen.height());

	/* Reisize the textbox to use the full window size, with a 10px border. */
	//ui->text->setLineWrapMode(QTextEdit::NoWrap);
	ui->text->move(10, 10);
	ui->text->resize(screen.width() - 20, screen.height() - ui->cmdClose->height() - 30);
	ui->cmdClose->move(screen.width() - ui->cmdClose->width() - 10, screen.height() - ui->cmdClose->height() - 10);

	process = new QProcess(this);
	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
	QStringList dpkgArgs = {
		"--showformat=${db:Status-Abbrev} ${Package;-28} ${Version;-24} ${binary:Summary}\n", "-W"
	};
	process->start("dpkg-query", dpkgArgs);

	/* Set edit focus so that the jog wheel works to scroll through packages. */
#ifdef QT_KEYPAD_NAVIGATION
	ui->text->setEditFocus(true);
#endif
}

PackageList::~PackageList()
{
	delete process;
	delete ui;
}

void PackageList::on_cmdClose_clicked()
{
	close();
}

void PackageList::readyReadStandardOutput()
{
	char buf[1024];
	qint64 len;

	process->setReadChannel(QProcess::StandardOutput);
	while ((len = process->readLine(buf, sizeof(buf))) > 0) {
		ui->text->appendPlainText(QString(buf).trimmed());
	}
}
