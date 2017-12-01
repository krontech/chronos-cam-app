#ifndef TRIGGERDELAYWINDOW_H
#define TRIGGERDELAYWINDOW_H

#include <QWidget>

namespace Ui {
class triggerDelayWindow;
}

class triggerDelayWindow : public QWidget
{
    Q_OBJECT

public:
    explicit triggerDelayWindow(QWidget *parent = 0);
    ~triggerDelayWindow();

private:
    Ui::triggerDelayWindow *ui;
};

#endif // TRIGGERDELAYWINDOW_H
