#ifndef CAMTEXTEDIT_H
#define CAMTEXTEDIT_H

#include <QTextEdit>

class CamTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	CamTextEdit(QWidget * parent = 0);

protected:
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);
	void mouseReleaseEvent(QMouseEvent *);
};



#endif // CAMTEXTEDIT_H
