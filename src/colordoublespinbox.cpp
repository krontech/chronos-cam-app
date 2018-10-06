#include "colordoublespinbox.h"
#include "camLineEdit.h"

ColorDoubleSpinBox::ColorDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
	CamLineEdit * le = new CamLineEdit(this);	//Have QSpinBox use our CamLineEdit, which takes care of software input panel
	QDoubleSpinBox::setLineEdit(le);
}

ColorDoubleSpinBox::~ColorDoubleSpinBox()
{
	delete QDoubleSpinBox::lineEdit();
}
