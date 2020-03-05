#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

#include <QString>
#include <QMap>

class NetworkConfig
{
public:
	NetworkConfig();

	static QString getUUID(QString device="eth0");

	bool dhcp;
	QString ipAddress;
	unsigned int prefixLen;
	QString gateway;

	QString find(QString name);

private:
	QMap<QString, QString> config;
};

#endif // NETWORKCONFIG_H
