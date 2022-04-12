#ifndef INDICATEWINDOW_H
#define INDICATEWINDOW_H

#include <QWidget>

namespace Ui {
class IndicateWindow;
}

class IndicateWindow : public QWidget
{
    Q_OBJECT

public:
    explicit IndicateWindow(QWidget *parent = 0);
    ~IndicateWindow();
    void setRecModeText(QString str);
    void setRGInfoText(QString str);
    void setTriggerText(QString str);
    void setEstimatedTime(int seconds);

private:
    Ui::IndicateWindow *ui;
};

#endif // INDICATEWINDOW_H
