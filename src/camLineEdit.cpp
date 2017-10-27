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
