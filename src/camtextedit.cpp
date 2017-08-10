//Derrive class so we can have input panel on single click

#include "camtextedit.h"
#include <QApplication>
#include <QDebug>

CamTextEdit::CamTextEdit(QWidget *parent) :
	QTextEdit(parent)
{
	//qDebug() << "CamTextEdit Consturcted";
}

void CamTextEdit::focusInEvent(QFocusEvent *e)
{
	QTextEdit::focusInEvent(e);
}

void CamTextEdit::focusOutEvent(QFocusEvent *e)
{
	QTextEdit::focusOutEvent(e);
}

void CamTextEdit::mouseReleaseEvent(QMouseEvent *)
{
	QEvent event(QEvent::RequestSoftwareInputPanel);	//Call up the software input panel
	qApp->sendEvent(this, &event);
}
