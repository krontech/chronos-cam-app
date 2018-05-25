#ifndef WHITEBALANCEDIALOG_H
#define WHITEBALANCEDIALOG_H

#include <QDialog>
#include "camera.h"
#include "statuswindow.h"

namespace Ui {
class whiteBalanceDialog;
}

class whiteBalanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit whiteBalanceDialog(QWidget *parent = 0, Camera * cameraInst = NULL);
	~whiteBalanceDialog();

private slots:
	void on_comboWB_currentIndexChanged(int index);

	void on_cmdSetCustomWB_clicked();

private:
	Ui::whiteBalanceDialog *ui;
	Camera * camera;
	bool windowInitComplete;
	StatusWindow * sw;
	double sceneWhiteBalPresets[6][3];
	void addPreset(double r, double b, double g, QString s);
};

#endif // WHITEBALANCEDIALOG_H
