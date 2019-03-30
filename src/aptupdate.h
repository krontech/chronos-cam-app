#ifndef APTUPDATE_H
#define APTUPDATE_H

#include <QWidget>
#include <QProgressDialog>

#include "defines.h"
#include "types.h"

class AptUpdate
{
public:
	AptUpdate(QWidget * parent = 0);
	~AptUpdate();
	int exec();

private:
	QProgressDialog *progUpdate;
	QProgressDialog *progUpgrade;

	char stLabel[4][256];
	int stIndex;
};

#endif // APTUPDATE_H
