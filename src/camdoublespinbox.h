#ifndef CAMDOUBLESPINBOX_H
#define CAMDOUBLESPINBOX_H

#include <QDoubleSpinBox>

class CamDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT
public:
	CamDoubleSpinBox(QWidget * parent = 0);
	~CamDoubleSpinBox();
protected:
	virtual void focusInEvent(QFocusEvent *e);
	virtual void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
};

#endif // CAMDOUBLESPINBOX_H
