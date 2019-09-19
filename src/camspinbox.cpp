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
#include "camspinbox.h"
#include "camLineEdit.h"
#include <QDebug>

CamSpinBox::CamSpinBox(QWidget *parent) :
	QSpinBox(parent)
{
	CamLineEdit * le = new CamLineEdit(this);	//Have QSpinBox use our CamLineEdit, which takes care of software input panel
	QSpinBox::setLineEdit(le);
	connect(le, SIGNAL(textChanged(QString)), this, SLOT(textChanged_slot()));
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
	if(oldValue < minimum()) setValue(minimum());
}

void CamSpinBox::mouseReleaseEvent(QMouseEvent * e)
{
	QSpinBox::mouseReleaseEvent(e);
}

void CamSpinBox::selectText(){
	CamLineEdit * le = qobject_cast<CamLineEdit*>(this->lineEdit());
	le->selectText();
}

void CamSpinBox::textChanged_slot(){
	oldValue = text().toInt();
}