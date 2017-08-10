//Derrive class so we can have input panel on single click

#include "camspinbox.h"
#include "camLineEdit.h"
#include <QDebug>

CamSpinBox::CamSpinBox(QWidget *parent) :
	QSpinBox(parent)
{
	CamLineEdit * le = new CamLineEdit(this);	//Have QSpinBox use our CamLineEdit, which takes care of software input panel
	QSpinBox::setLineEdit(le);
//	qDebug() << "CamSpinBox Consturcted";
}

CamSpinBox::~CamSpinBox()
{
	delete QSpinBox::lineEdit();	//TODO: Do we need to delete this here? Docs say QSpinBox takes ownership of the passed QLineEdit
}

void CamSpinBox::focusInEvent(QFocusEvent *e)
{
	QSpinBox::focusInEvent(e);
}

void CamSpinBox::focusOutEvent(QFocusEvent *e)
{
	QSpinBox::focusOutEvent(e);
}

void CamSpinBox::mouseReleaseEvent(QMouseEvent * e)
{
	QSpinBox::mouseReleaseEvent(e);
}
