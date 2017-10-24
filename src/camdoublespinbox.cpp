/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
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
