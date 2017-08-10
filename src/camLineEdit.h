#ifndef CAMLINEEDIT_H
#define CAMLINEEDIT_H

#include <QLineEdit>

class CamLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit CamLineEdit(QWidget * parent = 0);

protected:
  virtual void focusInEvent(QFocusEvent *e);
  virtual void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *);
};

#endif // CAMLINEEDIT_H
