//Derrive class so we can have input panel on single click

#include "camLineEdit.h"
#include <QApplication>
#include <QDebug>

CamLineEdit::CamLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	//qDebug() << "CamLineEdit Consturcted";
}

void CamLineEdit::focusInEvent(QFocusEvent *e)
{
	QLineEdit::focusInEvent(e);
}

void CamLineEdit::focusOutEvent(QFocusEvent *e)
{
	QLineEdit::focusOutEvent(e);
}

void CamLineEdit::mouseReleaseEvent(QMouseEvent *)
{
	QEvent event(QEvent::RequestSoftwareInputPanel);	//Call up the software input panel
	qApp->sendEvent(this, &event);
}
