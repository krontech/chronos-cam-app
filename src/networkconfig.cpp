#include "networkconfig.h"
#include "util.h"

#include <QDebug>
#include <QStringList>

NetworkConfig::NetworkConfig()
{

	/* Get the configuration for the ethernet interface. */
	QString uuid = getUUID();
	QStringList cfgData = runCommand(QString("nmcli c show %1").arg(uuid)).split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
	QStringListIterator iter(cfgData);
	while (iter.hasNext()) {
		/* Split the string on the first colon into name and value */
		QString x = iter.next();
		int colon = x.indexOf(':');
		if (colon > 0) {
			QString name = x.left(colon);
			QString value = x.mid(colon+1).trimmed();

			/* Add the option to the config dictionary. */
			config[name] = value;
		}
	}

	/* Parse out the IPv4 configuration. */
	dhcp = config.value("ipv4.method", "auto") == "auto";

	QString addresses = config.value("ipv4.addresses", "ip = 0.0.0.0/24, gw = 0.0.0.0");
	QRegExp ipRegex("ip\\s=\\s[0-9.]{7,18}/[0-9]+");
	int index = ipRegex.indexIn(addresses);
	if (index > 0) {
		QStringList match = addresses.mid(index + 5, ipRegex.matchedLength() - 5).split('/');
		ipAddress = match[0];
		prefixLen = match[1].toUInt();
	}
	else {
		ipAddress = "0.0.0.0";
		prefixLen = 32;
	}
	QRegExp gwRegex("gw\\s=\\s[0-9.]{7,18}");
	index = gwRegex.indexIn(addresses);
	if (index > 0) {
		gateway = addresses.mid(index + 5, gwRegex.matchedLength() - 5);
	}
	else {
		gateway = "0.0.0.0";
	}
}

QString NetworkConfig::getUUID(QString device)
{
	/* Get the UUID of the ethernet interface. */
	return runCommand("nmcli --fields UUID,DEVICE c show | grep " + device + " | awk \'{print $1}\'").trimmed();
}

/* Search for a network config variable by name */
QString NetworkConfig::find(QString name)
{
	return config.value(name, QString());
}
