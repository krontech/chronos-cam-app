#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QtGui>
#include <QtCore>
#include <QWidget>
#include <QDialog>

#define DEFAULT_BACKGROUND_BUTTON	"background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(0, 0, 0, 255), stop:1 rgba(255, 255, 255, 255));"
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

namespace Ui {
class keyboard;
}

class keyboard : public QDialog
{
	Q_OBJECT
	
public:
	explicit keyboard(QWidget *parent = 0);
	QWidget * getLastFocsedWidget() {return lastFocusedWidget;}
	~keyboard();

public slots:
	void            show();

signals:
	void characterGenerated(QChar character);
	void codeGenerated(int code);

protected:
	bool event(QEvent *e);

private slots:
	void saveFocusWidget(QWidget *oldFocus, QWidget *newFocus);
	void buttonClicked(QWidget *w);

	void on_caps_clicked();

	void on_back_clicked();

	void on_space_clicked();

	void on_shift_clicked();

	void on_up_clicked();

	void on_down_clicked();

	void on_left_clicked();

	void on_right_clicked();

	void on_enter_clicked();

	void on_close_clicked();



private:
	bool capslock;
	bool shift;
	Ui::keyboard *ui;
	QWidget *lastFocusedWidget;
	QSignalMapper signalMapper;
};

#endif // KEYBOARD_H
