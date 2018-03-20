#ifndef KEYBOARDBASE_H
#define KEYBOARDBASE_H

#include <QtGui>
#include <QtCore>
#include <QWidget>
#include <QDialog>

#define KEYBOARD_BACKGROUND_BUTTON	"background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 0, 0, 255), stop:1 rgba(255, 255, 255, 255));"
#define CHANGED_BACKGROUND_BUTTON	"background:lightgray;color:darkred;"

enum {
	KC_BACKSPACE = 1,
	KC_UP,
	KC_DOWN,
	KC_LEFT,
	KC_RIGHT,
	KC_ENTER,
	KC_CLOSE
};

class keyboardBase : public QWidget
{
	Q_OBJECT
public:
	explicit keyboardBase(QWidget *parent = 0);
	QWidget * getLastFocsedWidget() {return lastFocusedWidget;}
	~keyboardBase();
	QWidget *lastFocusedWidget;
    QSignalMapper signalMapper;

public slots:
	void            show();
    void selectAllInFocusedWidget();

signals:
	void characterGenerated(QChar character);
	void codeGenerated(int code);

protected:
	bool event(QEvent *e);

signals:

public slots:

private slots:
	void saveFocusWidget(QWidget *oldFocus, QWidget *newFocus);
	void on_back_clicked();
	void on_up_clicked();

	void on_down_clicked();

	void on_left_clicked();

	void on_right_clicked();

	void on_enter_clicked();

	void on_close_clicked();

private:

};

#endif // KEYBOARDBASE_H
