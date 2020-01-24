#include <QApplication>
#include <QWSServer>
#include <QFile>

#include "updatewindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

#ifdef Q_WS_QWS
	QWSServer::setCursorVisible( false );
	QWSServer::setBackground(QBrush(Qt::transparent));
#endif

	// Load stylesheet from file, if one exists.
	QFile fStyle("stylesheet.qss");
	if (fStyle.open(QFile::ReadOnly)) {
		QString sheet = QLatin1String(fStyle.readAll());
		qApp->setStyleSheet(sheet);
		fStyle.close();
	}

	/* Load and execute the update window */
	UpdateWindow w;
	w.show();
	return a.exec();
}
