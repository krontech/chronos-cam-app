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
#ifndef CAMLINEEDIT_H
#define CAMLINEEDIT_H

#include <QLineEdit>

class CamLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit CamLineEdit(QWidget * parent = 0);

    void selectText();
    bool getHasUnits();
    void setHasUnits(bool value);
protected:
  virtual void focusInEvent(QFocusEvent *e);
  virtual void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *);

private:
	bool hasUnits;
};

#endif // CAMLINEEDIT_H
