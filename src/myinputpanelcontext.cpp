/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>

#include "myinputpanelcontext.h"
#include "keyboard.h"

#include <algorithm>

MyInputPanelContext::MyInputPanelContext()
{
	inputPanelAlphabetic = new keyboard();
	inputPanelNumeric = new keyboardNumeric();
	inputPanel = inputPanelAlphabetic;//default

    connect(inputPanelAlphabetic, SIGNAL(characterGenerated(QChar)), SLOT(sendCharacter(QChar)));
	connect(inputPanelAlphabetic, SIGNAL(codeGenerated(int)), SLOT(sendCode(int)));

	connect(inputPanelNumeric, SIGNAL(characterGenerated(QChar)), SLOT(sendCharacter(QChar)));
	connect(inputPanelNumeric, SIGNAL(codeGenerated(int)), SLOT(sendCode(int)));

	keyboardActive = false;
}


MyInputPanelContext::~MyInputPanelContext()
{
    delete inputPanel;
}


bool MyInputPanelContext::filterEvent(const QEvent* event)
{
	qDebug() << "IPanel Event" << event;
	QWidget *widget = focusWidget();
	QWidget *window;

	if(widget)
		window = widget->window();



	if (event->type() == QEvent::RequestSoftwareInputPanel)
	{
		if(!window || !widget)			//Can't do anything if there's not a window to create the input panel for
			return false;

		if(!keyboardActive)
		{
			originalPos = window->pos();
		}

		QString className = widget->metaObject()->className();
		if(className.contains("spinbox", Qt::CaseInsensitive))
			inputPanel = inputPanelNumeric;
		else
			inputPanel = inputPanelAlphabetic;

		qDebug() << "inputPanel Show";
		inputPanel->show();

		//If widget is not fully within visible screen area, move window to center the widget in the visible screen area
		if(widget->mapToGlobal(QPoint(0,0)).y() + widget->size().height() > QApplication::desktop()->screenGeometry().height() - inputPanel->height())
		{
			window->move(window->pos().x(),
					   std::max(
						 (QApplication::desktop()->screenGeometry().height() - inputPanel->height()) / 2 - //Center of visible screen above keyboard
						   widget->mapToGlobal(QPoint(0,0)).y() - widget->size().height() / 2 //Focused widget center
						   , //max value of these two to prevent the window from moving up too far if a widget that is very close to the bottom of the screen requests a keyboard.
						   -inputPanel->height()));
		}

		keyboardActive = true;

        return true;
	}
	else if ((event->type() == QEvent::CloseSoftwareInputPanel) && (true == keyboardActive))
	{
		inputPanel->hide();
		window = inputPanel->getLastFocsedWidget()->window();
			window->move(originalPos);
		keyboardActive = false;
		return true;
	}
    return false;
}


QString MyInputPanelContext::identifierName()
{
    return "MyInputPanelContext";
}

void MyInputPanelContext::reset()
{
}

bool MyInputPanelContext::isComposing() const
{
    return false;
}

QString MyInputPanelContext::language()
{
    return "en_US";
}

void MyInputPanelContext::sendCharacter(QChar character)
{
    QPointer<QWidget> w = focusWidget();

    if (!w)
        return;

    QKeyEvent keyPress(QEvent::KeyPress, character.unicode(), Qt::NoModifier, QString(character));
    QApplication::sendEvent(w, &keyPress);

    if (!w)
        return;

    QKeyEvent keyRelease(QEvent::KeyPress, character.unicode(), Qt::NoModifier, QString());
    QApplication::sendEvent(w, &keyRelease);
}

void MyInputPanelContext::sendCode(int code)
{
	QPointer<QWidget> w = focusWidget();
	Qt::Key key;

	switch(code)
	{
	case KC_BACKSPACE:
		key = Qt::Key_Backspace;
		break;
	case KC_UP:
		key = Qt::Key_Up;
		break;
	case KC_DOWN:
		key = Qt::Key_Down;
		break;
	case KC_LEFT:
		key = Qt::Key_Left;
		break;
	case KC_RIGHT:
		key = Qt::Key_Right;
		break;
	case KC_ENTER:
		key = Qt::Key_Enter;
		break;
	case KC_CLOSE:
		if(keyboardActive) {
			/* Restore the window positions. */
			QWidget *window;
			window = inputPanel->getLastFocsedWidget()->window();
			inputPanel->hide();
			window->move(originalPos);
			keyboardActive = false;

			/* Also send an Enter event. */
			key = Qt::Key_Enter;
			break;
		}
		return;
	}

	if (!w)
		return;

	QKeyEvent keyPress(QEvent::KeyPress, key, Qt::NoModifier);
	QApplication::sendEvent(w, &keyPress);

	if (!w)
		return;

	QKeyEvent keyRelease(QEvent::KeyRelease, key, Qt::NoModifier);
	QApplication::sendEvent(w, &keyRelease);
}
