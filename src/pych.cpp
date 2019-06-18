
#include <QDebug>
#include <stdio.h>

#include <unistd.h>
#include <string>
#include <iostream>
#include "qjson4/QJsonDocument.h"
#include "qjson4/QJsonObject.h"
#include "qjson4/QJsonArray.h"
#include "frameGeometry.h"
#include "io.h"

UInt32 shadowExposure;
UInt32 shadowFramePeriod;

void setRecShadow(void)
{
	qDebug() << "setRecShadow";
}


void getRecShadow(void)
{
	qDebug() << "getRecShadow";
}
