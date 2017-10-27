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
#ifndef CAMDOUBLESPINBOX_H
#define CAMDOUBLESPINBOX_H

#include <QDoubleSpinBox>

class CamDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT
public:
	CamDoubleSpinBox(QWidget * parent = 0);
	~CamDoubleSpinBox();
protected:
	virtual void focusInEvent(QFocusEvent *e);
	virtual void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
};

#endif // CAMDOUBLESPINBOX_H
