#ifndef CAMSPINBOX_H
#define CAMSPINBOX_H

#include <QSpinBox>

class CamSpinBox : public QSpinBox
{
	Q_OBJECT
public:
	CamSpinBox(QWidget * parent = 0);
	~CamSpinBox();
protected:
	virtual void focusInEvent(QFocusEvent *e);
	virtual void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
};


#endif // CAMSPINBOX_H
