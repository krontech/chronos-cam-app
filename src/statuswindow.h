#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QWidget>

namespace Ui {
class StatusWindow;
}

class StatusWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit StatusWindow(QWidget *parent = 0);
	~StatusWindow();
	void setText(QString str);
	void setTimeout(int time) {timeout = time;}
	
private:
	Ui::StatusWindow *ui;
	QTimer * timer;
	int timeout;

protected:
	void showEvent( QShowEvent* event );

private slots:
	void on_StatusWindowTimer();
};

#endif // STATUSWINDOW_H
