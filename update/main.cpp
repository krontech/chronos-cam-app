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
#include <QApplication>
#include <QWSServer>
#include <QFile>

#include "updatewindow.h"
#include "updateprogress.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

#ifdef Q_WS_QWS
	QWSServer::setCursorVisible( false );
	QWSServer::setBackground(QBrush(Qt::transparent));
#endif
	a.setQuitOnLastWindowClosed(false);

	// Load stylesheet from file, if one exists.
	QFile fStyle("stylesheet.qss");
	if (fStyle.open(QFile::ReadOnly)) {
		QString sheet = QLatin1String(fStyle.readAll());
		qApp->setStyleSheet(sheet);
		fStyle.close();
	}

	/* Load and execute the update window */
	UpdateWindow w(NULL);
	w.show();
	return a.exec();
}
