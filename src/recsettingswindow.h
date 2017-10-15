#ifndef RECSETTINGSWINDOW_H
#define RECSETTINGSWINDOW_H

#include <QWidget>
#include <QCloseEvent>

#include "camera.h"

namespace Ui {
class RecSettingsWindow;
}

class RecSettingsWindow : public QWidget
{
	Q_OBJECT
	
public:
	explicit RecSettingsWindow(QWidget *parent = 0, Camera * cameraInst = NULL);
	~RecSettingsWindow();

private slots:
	void on_cmdOK_clicked();

	void on_spinHRes_valueChanged(int arg1);

	void on_spinHRes_editingFinished();

	void on_spinVRes_valueChanged(int arg1);

	void on_spinVRes_editingFinished();

	void on_spinHOffset_valueChanged(int arg1);

	void on_spinHOffset_editingFinished();

	void on_spinVOffset_valueChanged(int arg1);

	void on_spinVOffset_editingFinished();

	void on_chkCenter_toggled(bool checked);

	void on_cmdMax_clicked();

	void on_linePeriod_returnPressed();

	void on_lineRate_returnPressed();

	void on_lineExp_returnPressed();

	void on_cmdExpMax_clicked();

	void closeEvent(QCloseEvent *event);

	void on_comboRes_activated(const QString &arg1);

    void on_comboRes_activated(int index);

private:
	void updateOffsetLimits();
	void updateInfoText();
	void setResFromText(char * str);
	Ui::RecSettingsWindow *ui;
	Camera * camera;

signals:
	void settingsChanged();

};

#endif // RECSETTINGSWINDOW_H
