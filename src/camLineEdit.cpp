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
#include "camLineEdit.h"
#include <QApplication>
#include <QDebug>

CamLineEdit::CamLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	//qDebug() << "CamLineEdit Consturcted";
    hasUnits = false;
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

void CamLineEdit::selectText(){
	int length = text().length();
	if(length == 0) return;
	if(text()[length - 1].isLetter() && this->hasUnits && length > 1){
		if(text()[length - 2] == ' ')//check whether there is a space present so we know how many chars to avoid selecting
			setSelection(0, length - 2);
		else
			setSelection(0, length - 1);
	}
	else selectAll();
}

bool CamLineEdit::getHasUnits(){
	return hasUnits;
}
void CamLineEdit::setHasUnits(bool value){
	hasUnits = value;
}
