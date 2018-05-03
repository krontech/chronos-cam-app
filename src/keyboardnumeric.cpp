#include "keyboardnumeric.h"
#include "ui_keyboardnumeric.h"

keyboardNumeric::keyboardNumeric(QWidget *parent) :
	keyboardBase(parent),
	ui(new Ui::keyboardNumeric)
{
	ui->setupUi(this);

	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
			this, SLOT(saveFocusWidget(QWidget*,QWidget*)));

	signalMapper.setMapping(ui->num0, ui->num0);
	signalMapper.setMapping(ui->num1, ui->num1);
	signalMapper.setMapping(ui->num2, ui->num2);
	signalMapper.setMapping(ui->num3, ui->num3);
	signalMapper.setMapping(ui->num4, ui->num4);
	signalMapper.setMapping(ui->num5, ui->num5);
	signalMapper.setMapping(ui->num6, ui->num6);
	signalMapper.setMapping(ui->num7, ui->num7);
	signalMapper.setMapping(ui->num8, ui->num8);
	signalMapper.setMapping(ui->num9, ui->num9);
	signalMapper.setMapping(ui->numdot, ui->numdot);



	connect(ui->num0, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num1, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num2, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num3, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num4, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num5, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num6, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num7, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num8, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->num9, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->numdot, SIGNAL(clicked()), &signalMapper, SLOT(map()));

	connect(&signalMapper, SIGNAL(mapped(QWidget*)),
			this, SLOT(buttonClicked(QWidget*)));
}

keyboardNumeric::~keyboardNumeric()
{
	delete ui;
}

void keyboardNumeric::buttonClicked(QWidget *w)
{
	QPushButton * pb = (QPushButton *)w;
	emit characterGenerated(pb->text()[0]);
}
