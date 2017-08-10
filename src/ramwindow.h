#ifndef RAMWINDOW_H
#define RAMWINDOW_H

#include <QWidget>

#include "util.h"
#include "camera.h"

namespace Ui {
class ramWindow;
}

class ramWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit ramWindow(QWidget *parent = 0, Camera * cameraInst = NULL);
	~ramWindow();
	
private slots:
	void on_cmdSet_clicked();

	void on_cmdRead_clicked();

private:
	Camera * camera;
	Ui::ramWindow *ui;
};

#endif // RAMWINDOW_H
