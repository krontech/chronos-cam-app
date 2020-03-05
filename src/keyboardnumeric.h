#ifndef KEYBOARDNUMERIC_H
#define KEYBOARDNUMERIC_H

#include <QtGui>
#include <QtCore>
#include <QWidget>
#include <QDialog>
#include "keyboardbase.h"

namespace Ui {
class keyboardNumeric;
}

class keyboardNumeric : public keyboardBase
{
	Q_OBJECT

public:
	explicit keyboardNumeric(QWidget *parent = 0);
	~keyboardNumeric();

private:
	Ui::keyboardNumeric *ui;

private slots:
	void buttonClicked(QWidget *w);
	void saveFocusWidget(QWidget *, QWidget *newFocus);
};

#endif // KEYBOARDNUMERIC_H
