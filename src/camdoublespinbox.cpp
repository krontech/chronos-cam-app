//Derrive class so we can have input panel on single click

#include "camdoublespinbox.h"
#include "camLineEdit.h"
#include <QDebug>

CamDoubleSpinBox::CamDoubleSpinBox(QWidget *parent) :
	QDoubleSpinBox(parent)
{
	CamLineEdit * le = new CamLineEdit(this);	//Have QSpinBox use our CamLineEdit, which takes care of software input panel
	QDoubleSpinBox::setLineEdit(le);
//	qDebug() << "CamDoubleSpinBox Consturcted";
}

CamDoubleSpinBox::~CamDoubleSpinBox()
{
	delete QDoubleSpinBox::lineEdit();	//TODO: Do we need to delete this here? Docs say QDoubleSpinBox takes ownership of the passed QLineEdit
}

void CamDoubleSpinBox::focusInEvent(QFocusEvent *e)
{
	QDoubleSpinBox::focusInEvent(e);
}

void CamDoubleSpinBox::focusOutEvent(QFocusEvent *e)
{
	QDoubleSpinBox::focusOutEvent(e);
}

void CamDoubleSpinBox::mouseReleaseEvent(QMouseEvent * e)
{
	QDoubleSpinBox::mouseReleaseEvent(e);
}
