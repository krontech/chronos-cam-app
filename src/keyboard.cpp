/****************************************************************************
 *  Copyright (C) 2013-2017 Kron Technologies Inc <http://www.krontech.ca>. *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/
#include "keyboard.h"
#include "ui_keyboard.h"

keyboard::keyboard(QWidget *parent) :
	keyboardBase(parent),//QDialog(parent),
	ui(new Ui::keyboard)
{
	ui->setupUi(this);
	capslock = shift = false;
	setLowercase();

	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
			this, SLOT(saveFocusWidget(QWidget*,QWidget*)));

	signalMapper.setMapping(ui->Q, ui->Q);
	signalMapper.setMapping(ui->W, ui->W);
	signalMapper.setMapping(ui->E, ui->E);
	signalMapper.setMapping(ui->R, ui->R);
	signalMapper.setMapping(ui->T, ui->T);
	signalMapper.setMapping(ui->Y, ui->Y);
	signalMapper.setMapping(ui->U, ui->U);
	signalMapper.setMapping(ui->I, ui->I);
	signalMapper.setMapping(ui->O, ui->O);
	signalMapper.setMapping(ui->P, ui->P);
	signalMapper.setMapping(ui->A, ui->A);
	signalMapper.setMapping(ui->S, ui->S);
	signalMapper.setMapping(ui->D, ui->D);
	signalMapper.setMapping(ui->F, ui->F);
	signalMapper.setMapping(ui->G, ui->G);
	signalMapper.setMapping(ui->H, ui->H);
	signalMapper.setMapping(ui->J, ui->J);
	signalMapper.setMapping(ui->K, ui->K);
	signalMapper.setMapping(ui->L, ui->L);
	signalMapper.setMapping(ui->Z, ui->Z);
	signalMapper.setMapping(ui->X, ui->X);
	signalMapper.setMapping(ui->C, ui->C);
	signalMapper.setMapping(ui->V, ui->V);
	signalMapper.setMapping(ui->B, ui->B);
	signalMapper.setMapping(ui->N, ui->N);
	signalMapper.setMapping(ui->M, ui->M);
	signalMapper.setMapping(ui->slash, ui->slash);
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



	connect(ui->Q, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->W, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->E, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->R, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->T, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->Y, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->U, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->I, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->O, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->P, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->A, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->S, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->D, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->F, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->G, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->H, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->J, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->K, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->L, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->Z, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->X, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->C, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->V, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->B, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->N, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->M, SIGNAL(clicked()), &signalMapper, SLOT(map()));
	connect(ui->slash, SIGNAL(clicked()), &signalMapper, SLOT(map()));
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

keyboard::~keyboard()
{
	delete ui;
}



void keyboard::buttonClicked(QWidget *w)
{
	QPushButton * pb = (QPushButton *)w;
	QChar chr = pb->text()[0];
	emit characterGenerated(capslock || shift ? chr : chr.toLower());
	if(shift)
	{
		on_shift_clicked();
	}
}

void keyboard::on_caps_clicked()
{
	if(capslock)
	{
		capslock = false;
		ui->caps->setStyleSheet(QString(""));
		if(!shift) setLowercase();
		else setUppercase();
	}
	else
	{
		capslock = true;
		ui->caps->setStyleSheet(QString(KEYBOARD_BACKGROUND_BUTTON));
		if(shift)setLowercase();
		else setUppercase();
	}
}

void keyboard::on_space_clicked()
{
	emit characterGenerated(QChar(' '));
}

void keyboard::on_shift_clicked()
{
	if(shift)
	{
		shift = false;
		ui->shift->setStyleSheet(QString(""));
		if(!capslock) setLowercase();
		else setUppercase();
	}
	else
	{
		shift = true;
		ui->shift->setStyleSheet(QString(KEYBOARD_BACKGROUND_BUTTON));
		if(capslock) setLowercase();
		else setUppercase();
	}
}

void keyboard::setUppercase(){
	ui->Q->setText("Q");
	ui->W->setText("W");
	ui->E->setText("E");
	ui->R->setText("R");
	ui->T->setText("T");
	ui->Y->setText("Y");
	ui->U->setText("U");
	ui->I->setText("I");
	ui->O->setText("O");
	ui->P->setText("P");
	ui->A->setText("A");
	ui->S->setText("S");
	ui->D->setText("D");
	ui->F->setText("F");
	ui->G->setText("G");
	ui->H->setText("H");
	ui->J->setText("J");
	ui->K->setText("K");
	ui->L->setText("L");
	ui->Z->setText("Z");
	ui->X->setText("X");
	ui->C->setText("C");
	ui->V->setText("V");
	ui->B->setText("B");
	ui->N->setText("N");
	ui->M->setText("M");
	ui->slash->setText("\\");
	ui->num0->setText("_");
	ui->num1->setText("!");
	ui->num2->setText("@");
	ui->num3->setText("#");
	ui->num4->setText("$");
	ui->num5->setText("-");
	ui->num6->setText("+");
	ui->num7->setText("'");
	ui->num8->setText("(");
	ui->num9->setText(")");
	ui->numdot->setText(",");
}

void keyboard::setLowercase(){
	ui->Q->setText("q");
	ui->W->setText("w");
	ui->E->setText("e");
	ui->R->setText("r");
	ui->T->setText("t");
	ui->Y->setText("y");
	ui->U->setText("u");
	ui->I->setText("i");
	ui->O->setText("o");
	ui->P->setText("p");
	ui->A->setText("a");
	ui->S->setText("s");
	ui->D->setText("d");
	ui->F->setText("f");
	ui->G->setText("g");
	ui->H->setText("h");
	ui->J->setText("j");
	ui->K->setText("k");
	ui->L->setText("l");
	ui->Z->setText("z");
	ui->X->setText("x");
	ui->C->setText("c");
	ui->V->setText("v");
	ui->B->setText("b");
	ui->N->setText("n");
	ui->M->setText("m");
	ui->num0->setText("0");
	ui->num1->setText("1");
	ui->num2->setText("2");
	ui->num3->setText("3");
	ui->num4->setText("4");
	ui->num5->setText("5");
	ui->num6->setText("6");
	ui->num7->setText("7");
	ui->num8->setText("8");
	ui->num9->setText("9");
	ui->numdot->setText(".");
	ui->slash->setText("/");
}
