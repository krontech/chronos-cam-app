/****************************************************************************
 *  Copyright (C) 2019-2020 Kron Technologies Inc <http://www.krontech.ca>. *
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
#ifndef PACKAGELIST_H
#define PACKAGELIST_H

#include <QDialog>
#include <QProcess>

namespace Ui {
class PackageList;
}

class PackageList : public QDialog
{
	Q_OBJECT

public:
	explicit PackageList(QWidget *parent = 0);
	~PackageList();

private:
	Ui::PackageList *ui;
	QProcess *process;

private slots:
	void on_cmdClose_clicked();
	void readyReadStandardOutput();
};

#endif // PACKAGELIST_H
