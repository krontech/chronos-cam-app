#ifndef FRAMEIMAGE_H
#define FRAMEIMAGE_H

#include <QFrame>
#include <QMouseEvent>

class FrameImage : public QFrame
{
	Q_OBJECT
public:
	explicit FrameImage(QFrame *parent = 0);
	QPoint cursorStartPoint, frameImageStartPoint;
	int newXPosition;
	int newYPosition;

signals:
	void moved();

private:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

public slots:
};

#endif // FRAMEIMAGE_H
