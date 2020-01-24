#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QWidget>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <pthread.h>

namespace Ui {
class UpdateWindow;
}

enum UpdateStates {
	UPDATE_CHECKSUMS,
	UPDATE_PREINST,
	UPDATE_DEBPKG,
	UPDATE_POSTINST,
	UPDATE_REBOOT,
};

class UpdateWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit UpdateWindow(QString manifestPath, QWidget *parent = 0);
	~UpdateWindow();
	
private:
	Ui::UpdateWindow *ui;
	QDir      location;
	QFileInfo manifest;
	QProcess *process;
	QTimer   *timer;
	QStringList packages;
	QString   preinst;
	QString   postinst;
	int       countdown;
	enum UpdateStates state;

	void parseManifest();
	void startReboot(int secs);

public slots:
	void started();
	void finished(int code, QProcess::ExitStatus status);
	void error(QProcess::ProcessError err);
	void readyReadStandardOutput();
	void readyReadStandardError();

	void timeout();
};

#endif // UPDATEWINDOW_H
