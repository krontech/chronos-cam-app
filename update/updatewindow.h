#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QWidget>
#include <QLocalServer>

#include <pthread.h>

namespace Ui {
class UpdateWindow;
}

class UpdateWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit UpdateWindow(QWidget *parent = 0);
	~UpdateWindow();
	
private:
	Ui::UpdateWindow *ui;
	pthread_t handle;

public slots:
	void message(QString msg);
};

#endif // UPDATEWINDOW_H
