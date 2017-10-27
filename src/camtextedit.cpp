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
