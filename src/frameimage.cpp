#include "frameimage.h"
#include <QDebug>
#include <QMouseEvent>

FrameImage::FrameImage(QFrame *parent) : QFrame(parent)
{

}

void FrameImage::mousePressEvent(QMouseEvent *event){
	qDebug()<<"frame press";
	qDebug()<<"start x: " << cursorStartPoint.rx();
	qDebug()<<"start y: " << cursorStartPoint.ry();
	cursorStartPoint = event->globalPos();
	frameImageStartPoint = pos();
}

void FrameImage::mouseMoveEvent(QMouseEvent *event){
	qDebug()<<"frame move";
	newXPosition = frameImageStartPoint.x() + event->globalX() - cursorStartPoint.rx();
	newYPosition = frameImageStartPoint.y() + event->globalY() - cursorStartPoint.ry();
	emit moved(); //So that RecSettingsWindow knows that it has to change the positions and sizes of the passepartout (transparent) grey areas surrounding the frameImage
	move(newXPosition, newYPosition);
	qDebug()<< "x = " << newXPosition;
	qDebug()<< "y = " << newYPosition;
}
