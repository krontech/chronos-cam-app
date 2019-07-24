
#include <QDebug>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "qjson4/QJsonDocument.h"
#include "qjson4/QJsonObject.h"
#include "qjson4/QJsonArray.h"
#include "frameGeometry.h"
#include "io.h"

using namespace std;



void parseJsonIoSettings(QString jsonString)
{
	//build JSON document d
	QJsonDocument d = QJsonDocument::fromJson(jsonString.toUtf8());

	QJsonObject qjo= d.object();

	QVariantMap root_map = qjo.toVariantMap();
	QVariantMap io_map = root_map["ioMapping"].toMap();

	QVariantMap item_map;

	// start
	item_map = io_map["start"].toMap();
	QString startSource = item_map["source"].toString();
	bool startDebounce = item_map["debounce"].toInt();
	bool startInvert = item_map["invert"].toInt();
	qDebug() << "start:" << startSource << startDebounce << startInvert;

	// io1
	item_map = io_map["io1In"].toMap();
	pychIo1Threshold = item_map["threshold"].toDouble();
	qDebug() << "io 1:" << pychIo1Threshold;

	// io2
	item_map = io_map["io2In"].toMap();
	pychIo2Threshold = item_map["threshold"].toDouble();
	qDebug() << "io 1:" << pychIo2Threshold;

	// delay
	item_map = io_map["delay"].toMap();
	UInt32 delayTime = item_map["delayTime"].toInt();
	QString delaySource = item_map["source"].toString();
	bool delayDebounce = item_map["debounce"].toInt();
	bool delayInvert = item_map["invert"].toInt();
	qDebug() << "delay:" << delaySource << delayTime << delayDebounce << delayInvert;
}

void parseJsonResolution(QString jsonString, FrameGeometry *geometry)
{
	//build JSON document d
	QJsonDocument d = QJsonDocument::fromJson(jsonString.toUtf8());
	qDebug() << QString::fromUtf8(d.toJson(QJsonDocument::Compact));

	qDebug() << d.isObject();
	QJsonObject qjo= d.object();

	QVariantMap root_map = qjo.toVariantMap();
	QVariantMap res_map = root_map["resolution"].toMap();

	geometry->hRes = res_map["hRes"].toInt();
	geometry->vRes = res_map["vRes"].toInt();
	geometry->hOffset = res_map["hOffset"].toInt();
	geometry->vOffset = res_map["vOffset"].toInt();
	geometry->vDarkRows = res_map["vDarkRows"].toInt();
	geometry->bitDepth = res_map["bitDepth"].toInt();
	geometry->minFrameTime = res_map["minFrameTime"].toDouble();
}

void parseJsonTiming(QString jsonString, FrameGeometry *geometry, FrameTiming *timing)
{
	//build JSON document d
	QJsonDocument d = QJsonDocument::fromJson(jsonString.toUtf8());
	QJsonObject qjo= d.object();
	QVariantMap root_map = qjo.toVariantMap();

	timing->exposureMax = root_map["exposureMax"].toInt();
	timing->minFramePeriod = root_map["minFramePeriod"].toInt();
	timing->exposureMin = root_map["exposureMin"].toInt();
	timing->cameraMaxFrames = root_map["cameraMaxFrames"].toInt();
}

void buildJsonResolution(QString *jsonString, FrameGeometry *geometry)
{
	jsonString->append("{\n   \"resolution\": {");
	jsonString->append(",\n      \"hRes\": " + QString::number(geometry->hRes));
	jsonString->append(",\n      \"vRes\": " + QString::number(geometry->vRes));
	jsonString->append(",\n      \"hOffset\": " + QString::number(geometry->hOffset));
	jsonString->append(",\n      \"vOffset\": " + QString::number(geometry->vOffset));
	jsonString->append(",\n      \"vDarkRows\": " + QString::number(geometry->vDarkRows));
	jsonString->append(",\n      \"bitDepth\": " + QString::number(geometry->bitDepth));
	jsonString->append(",\n      \"minFrameTime\": " + QString::number(geometry->minFrameTime));
	jsonString->append("\n   }\n}");
}

void buildJsonCalibration(QString *jsonString, QString calType)
{
	jsonString->clear();
	jsonString->append("{ \"" + calType);
	jsonString->append("\":1}");

	qDebug() << *jsonString;
}

void buildJsonTiming(QString *jsonString, FrameGeometry *geometry)
{
	jsonString->append("{\n   \"hRes\": " + QString::number(geometry->hRes));
	jsonString->append(",\n   \"vRes\": " + QString::number(geometry->vRes));
	jsonString->append(",\n   \"hOffset\": " + QString::number(geometry->hOffset));
	jsonString->append(",\n   \"vOffset\": " + QString::number(geometry->vOffset));
	jsonString->append(",\n   \"vDarkRows\": " + QString::number(geometry->vDarkRows));
	//jsonString->append(",\n   \"bitDepth\": " + QString::number(geometry->bitDepth));
	//jsonString->append(",\n   \"minFrameTime\": " + QString::number(geometry->minFrameTime));
	jsonString->append("\n}");
}

void buildJsonArray(QString parameter, QString *jsonString, UInt32 size, double *values)
{
	jsonString->append("{\n   \"");
	jsonString->append(parameter);
	jsonString->append("\": [");
	for (int i=0; i<size; i++)
	{
		jsonString->append("\n      " + QString::number(values[i]));
		if (i != size-1)
		{
			jsonString->append(",");
		}
	}
	jsonString->append("\n   ]\n}");
}

void buildJsonIo(QString *jsonString)
{
	jsonString->append("{\n   \"ioMapping\": {");
	jsonString->append("\n      \"start\": {");
	jsonString->append("\n         \"source\": none");
	jsonString->append(",\n         \"debounce\": " + QString::number(0));
	jsonString->append(",\n         \"invert\": " + QString::number(0));
	jsonString->append("\n      },");
	jsonString->append(",\n      \"io1In\": {");
	jsonString->append("\n         \"threshold\": " + QString::number(pychIo1Threshold));
	jsonString->append("\n      },");
	jsonString->append(",\n      \"io2In\": {");
	jsonString->append("\n         \"threshold\": " + QString::number(pychIo2Threshold));
	jsonString->append("\n      },");
	jsonString->append("\n   }\n}");

	//qDebug() << *jsonString;
}



void parseJsonArray(QString parameter, QString jsonString, uint32_t size, double *values)
{
	//build JSON document d
	QJsonDocument d = QJsonDocument::fromJson(jsonString.toUtf8());

	QJsonObject qjo= d.object();
	QJsonArray qja = qjo[parameter].toArray();

	for (int i=0; i<size; i++)
	{
		values[i] = qja.at(i).toDouble();
	}
}


void setCamJson( QString jsonString)
{
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = jsonString.toStdString();
	string receive_output = "";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}


		execl(program_name.c_str(), program_name.c_str(), "set", "-", (char *) 0);

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
	}
}

void getCamJson(QString parameter, QString *jsonString)
{
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string sparameter = parameter.toStdString();

	string gulp_command = "{\"" + sparameter + "\" : 0}";
	string receive_output = "";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}
		execl(program_name.c_str(), program_name.c_str(), "get", "-", (char *) 0);

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		//cout << "\n\n===============\nWriting to child: <<" << gulp_command << ">>" << endl;
		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
		*jsonString = QString::fromStdString(receive_output);
	}
}

void startCalibrationCamJson( QString *jsonOutString, QString *jsonInString)
{
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = jsonInString->toStdString();
	string receive_output = "";

	qDebug() << "startCalibration";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}

		execl(program_name.c_str(), program_name.c_str(), "startCalibration", "-", (char *) 0);
		qDebug() << program_name.c_str() << program_name.c_str() << "startCalibration" << "-";

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
		*jsonOutString = QString::fromStdString(receive_output);
		//qDebug() << *jsonOutString;
	}
}

void startRecordingCamJson( QString *jsonString)
{
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = "";
	string receive_output = "";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}

		execl(program_name.c_str(), program_name.c_str(), "startRecording", "-", (char *) 0);
		qDebug() << program_name.c_str() << program_name.c_str() << "startRecording" << "-";

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
		*jsonString = QString::fromStdString(receive_output);
	}
}


void stopRecordingCamJson( QString *jsonString)
{
	qDebug() << "running stopRecordingCamJson";
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = "";
	string receive_output = "";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}

		execl(program_name.c_str(), program_name.c_str(), "stopRecording", "-", (char *) 0);
		qDebug() << program_name.c_str() << program_name.c_str() << "startRecording" << "-";

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
		*jsonString = QString::fromStdString(receive_output);
	}
}

void testResolutionCamJson(QString *jsonString, FrameGeometry *geometry)
{
	QString geometryString;

	buildJsonTiming(&geometryString, geometry);

	//qDebug() << "running testResolutionCamJson";
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = geometryString.toStdString();
	//string gulp_command = "{\"hRes\": 1280, \"vOffset\": 0, \"vRes\": 1024, \"vDarkRows\": 8}";
	//string gulp_command = "{\"hRes\": 1280, \"vOffset\": 0, \"vRes\": 1024, \"vDarkRows\": 8}";
	string receive_output = "";

	if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
	{
		cerr << "Failed to pipe\n";
		exit(1);
	}
	childpid = fork();

	if (childpid < 0)
	{
		cout << "Fork failed" << endl;
		exit(-1);
	}
	else if (childpid == 0)
	{
		if (dup2(fd_p2c[0], 0) != 0 ||
			close(fd_p2c[0]) != 0 ||
			close(fd_p2c[1]) != 0)
		{
			cerr << "Child: failed to set up standard input\n";
			exit(1);
		}
		if (dup2(fd_c2p[1], 1) != 1 ||
			close(fd_c2p[1]) != 0 ||
			close(fd_c2p[0]) != 0)
		{
			cerr << "Child: failed to set up standard output\n";
			exit(1);
		}

		execl(program_name.c_str(), program_name.c_str(), "getResolutionTimingLimits", "-", (char *) 0);

		cerr << "Failed to execute " << program_name << endl;
		exit(1);
	}
	else
	{
		signal(SIGCHLD, SIG_IGN);

		close(fd_p2c[0]);
		close(fd_c2p[1]);

		int nbytes = gulp_command.length();
		if (write(fd_p2c[1], gulp_command.c_str(), nbytes) != nbytes)
		{
			cerr << "Parent: short write to child\n";
			exit(1);
		}
		close(fd_p2c[1]);

		while (1)
		{
			bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

			if (bytes_read <= 0)
				break;

			readbuffer[bytes_read] = '\0';
			receive_output += readbuffer;
		}
		close(fd_c2p[0]);
		*jsonString = QString::fromStdString(receive_output);
	}
}
