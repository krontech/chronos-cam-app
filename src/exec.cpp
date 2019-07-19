
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

using namespace std;

void buildJsonCalibration(QString *jsonString, QString calType)
{
	jsonString->clear();
	jsonString->append("{ \"" + calType);
	jsonString->append("\":1}");

}

void startCalibrationCamJson( QString *jsonOutString, QString *jsonInString)
{
	int fd_p2c[2], fd_c2p[2], bytes_read;
	pid_t childpid;
	char readbuffer[80];
	string program_name = "/usr/bin/cam-json";

	string gulp_command = jsonInString->toStdString();
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

		execl(program_name.c_str(), program_name.c_str(), "startCalibration", "-", (char *) 0);

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
