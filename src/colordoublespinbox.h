#ifndef COLORDOUBLESPINBOX_H
#define COLORDOUBLESPINBOX_H

#include <QDoubleSpinBox>

class ColorDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	ColorDoubleSpinBox(QWidget *parent = NULL);
	~ColorDoubleSpinBox();
};

#endif // COLORDOUBLESPINBOX_H
