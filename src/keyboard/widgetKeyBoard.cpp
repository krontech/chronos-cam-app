/***************************************************************************
 *   Copyright (C) 2008 by Giovanni Romano                                 *
 *   Giovanni.Romano.76@gmail.com                                          *
 *                                                                         *
 *   This program may be distributed under the terms of the Q Public       *
 *   License as defined by Trolltech AS of Norway and appearing in the     *
 *   file LICENSE.QPL included in the packaging of this file.              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/


#include "widgetKeyBoard.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QScreen>
#include <QKeyEvent>
#include <QDir>

bool widgetKeyBoard::m_created = false;


widgetKeyBoard::widgetKeyBoard(bool embeddedKeyboard, QWidget *activeForm, QWidget *parent) : QWidget(parent), m_nextInput(NULL),
		m_embeddedKeyboard(embeddedKeyboard), m_echoMode(false), m_zoomFacilityEmbedded(false), m_enablePasswordEcho(false),
		m_player(QDir::currentPath() + CLICK_SOUND), m_clipboard(QApplication::clipboard())
{
	connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
			this, SLOT(saveFocusWidget(QWidget*,QWidget*)));

    this->m_clipboard->clear();
    this->setWindowIcon(QPixmap(":/TastieraVirtuale/logo"));
}

widgetKeyBoard::~widgetKeyBoard()
{
}


void widgetKeyBoard::enableSwitchingEcho(bool status)
{
    this->m_enablePasswordEcho = status;
}

bool widgetKeyBoard::isEnabledSwitchingEcho(void)
{
    return (this->m_enablePasswordEcho);
}

/*inline */ bool widgetKeyBoard::isEmbeddedKeyboard(void)
{
    return (this->m_embeddedKeyboard);
}

void widgetKeyBoard::setZoomFacility(bool active)
{
    this->m_zoomFacilityEmbedded = active;
}

bool widgetKeyBoard::isZoomFacilityEnable(void)
{
    return (this->m_zoomFacilityEmbedded);
}

QKeyPushButton * widgetKeyBoard::createNewKey(QString keyValue)
{
    QKeyPushButton *tmp = new QKeyPushButton(this);
    QString        style = QString(DEFAULT_STYLE_BUTTON) + QString(DEFAULT_BACKGROUND_BUTTON);
    QSize          imageDim;
    int            width = 0, height = 0;

    if (this->isEmbeddedKeyboard() == true) {
        imageDim.setWidth(16);
        imageDim.setHeight(16);
    }
    else {
        imageDim.setWidth(32);
        imageDim.setHeight(32);
    }
    if (IS_PASSWORD(keyValue) == true || IS_PASSWORD_EMB(keyValue) == true) {
        tmp->setIconSize(imageDim);
        tmp->setIcon(QPixmap(":/TastieraVirtuale/normalEcho"));
    }
    else if (IS_PASTE(keyValue) == true) {
        tmp->setIconSize(imageDim);
        tmp->setEnabled(false);
        tmp->setIcon(QPixmap(":/TastieraVirtuale/paste"));
    }
    else if (IS_COPY(keyValue) == true) {
        tmp->setIconSize(imageDim);
        tmp->setIcon(QPixmap(":/TastieraVirtuale/copy"));
    }
    else if (IS_CUT_LEFT(keyValue) == true) {
        tmp->setIconSize(imageDim);
        tmp->setIcon(QPixmap(":/TastieraVirtuale/cut"));
    }
    tmp->setText(keyValue);
    if (this->isEmbeddedKeyboard() == true) {
        width = KEY_WIDTH_EMBEDDED;
        height = KEY_HEIGHT_EMBEDDED;
        style += QString(EMBEDDED_KEYBOARD);
    }
    else {
        width = 54;
        height = 40;
    }
    tmp->setObjectName(keyValue);
    tmp->setMinimumSize(width, height);
    tmp->setMaximumSize(width, height);
    tmp->setStyleSheet(style);
    tmp->setVisible(true);
    return (tmp);
}

void widgetKeyBoard::controlKeyEcho(QLineEdit *control)
{
    QString         textToSearch;
    QKeyPushButton  *echoControlKey;
    QString         tmpStyle;
    QPushButton     *pasteButton = this->findChild<QPushButton *>(KEY_PASTE);
    QPushButton     *cutButton = this->findChild<QPushButton *>(KEY_CUT_LEFT);
    QPushButton     *copyButton = this->findChild<QPushButton *>(KEY_COPY);

    if (this->isEmbeddedKeyboard()) {
        textToSearch = KEY_HIDECHAR_EMBEDDED;
        tmpStyle = QString(EMBEDDED_KEYBOARD);
    }
    else
        textToSearch = KEY_HIDECHAR;
    if (pasteButton != NULL && copyButton != NULL) { // support for copy/paste functionality
        if (control->echoMode() == QLineEdit::Normal) {
            cutButton->setEnabled(true);
            copyButton->setEnabled(true);
            if (this->m_clipboard->text().length() > 0)
                pasteButton->setEnabled(true);
            else
                pasteButton->setEnabled(false);
        }
        else if (control->echoMode() == QLineEdit::Password) {
            copyButton->setEnabled(false);
            cutButton->setEnabled(false);
            pasteButton->setEnabled(false);
        }
    }
    echoControlKey = this->findChild<QKeyPushButton *>(textToSearch);
    if (echoControlKey != NULL) { // support for password echo functionality            
        if (control->echoMode() == QLineEdit::Normal) {            
            echoControlKey->setStyleSheet(QString(DEFAULT_STYLE_BUTTON) + QString(DEFAULT_BACKGROUND_BUTTON) +  tmpStyle); // segnalato come attivo
            echoControlKey->setIcon(QPixmap(":/TastieraVirtuale/normalEcho"));
        }
        else if (control->echoMode() == QLineEdit::Password) {            
            echoControlKey->setStyleSheet(QString(DEFAULT_STYLE_BUTTON) + QString(CHANGED_BACKGROUND_BUTTON) + tmpStyle); // segnalato come attivo
            echoControlKey->setIcon(QPixmap(":/TastieraVirtuale/passwdEcho"));
        }
        echoControlKey->repaint();
		QApplication::processEvents();
    }
}

//
// riceve i caratteri che sono stati digitati:
void widgetKeyBoard::receiptChildKey(QKeyEvent *event,  bool reset)
{

	QString newKey = event->text();

	emit characterGenerated(newKey[0]);

	/*
    // inizia l'analisi del carattere ricevuto:
    QString newKey = event->text();
	QString tmpReceiptString = nextInput-> text();
    int     tmpPos = nextInput->cursorPosition();

    if (NO_SPECIAL_KEY(newKey) == false) {
        if (IS_RETURN(newKey) == true || IS_TAB(newKey) == true) { // trattasi di TAB, si sposta alla text successiva
            nextInput = this->setDefaultTextStyle(nextInput);
            nextInput->deselect();
            nextInput = this->getNextTextbox();
            this->controlKeyEcho(nextInput); // status of key echo here
            if (nextInput != NULL) {
                nextInput->setCursorPosition(nextInput->text().length()); // comment this line if you want caret position at current char inserted
                nextInput->setFocus(Qt::TabFocusReason);
            }
        }
        else if (IS_BACK(newKey) == true || IS_BACK_EMB(newKey) == true) { // trattasi di CANCELLARE carattere, elimina il carattere a sinistra
            if (tmpPos-1 >= 0) {
                tmpReceiptString = tmpReceiptString.remove(tmpPos-1, 1);
                nextInput->setText(tmpReceiptString);
                nextInput->setCursorPosition(tmpPos-1);
                nextInput->setSelection(tmpPos-2, 1);
            }
        }
        else if (IS_CANC(newKey) == true) { // trattasi di CANC carattere, elimina il carattere a destra
             tmpReceiptString = tmpReceiptString.remove(tmpPos, 1);
             nextInput->setText(tmpReceiptString);
             nextInput->setCursorPosition(tmpPos);
             nextInput->setSelection(tmpPos-1, 1);
        }
        else if (IS_COPY(newKey) == true || IS_CUT_LEFT(newKey) == true) {
            QPushButton *button = this->findChild<QPushButton *>(KEY_PASTE);

            if (button != NULL) {
                if (nextInput->text().length() != 0) {
                    this->m_clipboard->setText(nextInput->text());
                    if (IS_CUT_LEFT(newKey) == true)
                        nextInput->setText("");
                    button->setEnabled(true);
                }
                else
                    button->setEnabled(false);
            }
        }
        else if (IS_PASTE(newKey) == true)
            nextInput->setText(this->m_clipboard->text());
        else if (IS_ALT(newKey) == true || IS_CTRL_LEFT(newKey) == true)
        {
            ; // non esegue nessuna operazione
        }
    }
    else { // si tratta di un carattere da visualizzare nella casella di testo corrente
        tmpReceiptString = tmpReceiptString.insert(tmpPos, newKey);
        nextInput->setText(tmpReceiptString);
        nextInput->setCursorPosition(tmpPos+1);
        nextInput->setSelection(tmpPos, 1);
    }
	*/
}


void widgetKeyBoard::switchKeyEchoMode(QLineEdit *control)
{
    this->setStatusEchoMode(!this->statusEchoMode());
    if (this->statusEchoMode() == true)
        control->setEchoMode(QLineEdit::Password);
    else
        control->setEchoMode(QLineEdit::Normal);
    this->controlKeyEcho(control);
}


void widgetKeyBoard::init_keyboard(QLineEdit *focusThisControl)
{
    this->m_nextInput = focusThisControl;
//	this->receiptChildKey(NULL, true);
}

void widgetKeyBoard::soundClick(void)
{
    this->m_player.play();
}

void widgetKeyBoard::show()
{
	//QDesktopWidget * qdw = QApplication::desktop();

	QRect rec = QApplication::desktop()->screenGeometry();
    this->init_keyboard(NULL);
    if (this->windowState() == Qt::WindowMinimized)
        this->setWindowState(Qt::WindowNoState);
    this->setStatusEchoMode(false);
    this->m_clipboard->clear();
	this->borderFrame(false);
    QWidget::show();
	//Moving after showing because window height isn't set until show()
	this->move(0,QApplication::desktop()->screenGeometry().height() - height());

}


void widgetKeyBoard::borderFrame(bool visible)
{
   // if (visible == true)
		this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint/*Qt::MSWindowsFixedSizeDialogHint*/);
    //else
    //    this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint);
}



void widgetKeyBoard::closeEvent(QCloseEvent * event)
{
    event->ignore();
}


void widgetKeyBoard::hide()
{
    try {
        setVisible(false);
    }
    catch (...) {
    }
}


void widgetKeyBoard::createKeyboard(void)
{
	QKeyPushButton	*tmp = NULL;	
    QVBoxLayout     *tmpVLayout = new QVBoxLayout;
    QHBoxLayout     *tmpLayout = new QHBoxLayout;
    QString         tmpStyle = QString::null;

    if (widgetKeyBoard::m_created == true) // tastiera già  creata: esce
		return;
    widgetKeyBoard::m_created = true; // segnala che è stata creata la tastiera
	//
    // stampa tasti numerici:
	tmpLayout->addWidget(createNewKey("\\"));
	for (short i = 49; i <= 57; i++) {
		tmpLayout->addWidget(createNewKey(QChar(i)));
	}
    tmpLayout->addWidget(createNewKey(tr("0")));
    tmpLayout->addWidget(createNewKey(tr("/")));
    tmpLayout->addWidget(createNewKey(tr("?")));
    tmpLayout->addWidget(createNewKey(tr("'")));
    if (this->isEmbeddedKeyboard() == true)
        tmpStyle = QString(KEY_BACKSPACE_EMBEDDED);
    else
        tmpStyle = QString(KEY_BACKSPACE);
    tmp = createNewKey(tmpStyle);
    tmp->setMaximumWidth(tmp->maximumWidth() * 2);
    tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
    tmpStyle = QString(KEY_CANC);
    if (this->isEmbeddedKeyboard() == true)
        tmpStyle = tmpStyle.toLower();
    tmp = createNewKey(tmpStyle);
    tmp->setMaximumWidth(tmp->maximumWidth());
    tmp->setMinimumWidth(tmp->minimumWidth());
    tmpLayout->addWidget(tmp);
#if !defined(QT_ARCH_MACOSX)
    tmpLayout->insertStretch(13, 1);
#endif
	tmpVLayout->insertLayout(0, tmpLayout);
	//
	// Stampa linea della "Q":
	tmpLayout = new QHBoxLayout;	
    QVBoxLayout *layoutReturn = new QVBoxLayout;
	tmp = createNewKey(KEY_TAB);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2 - 5);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2 - 5);
    tmpLayout->addWidget(tmp);
    tmpLayout->addWidget(createNewKey(tr("Q")));
    tmpLayout->addWidget(createNewKey(tr("W")));
    tmpLayout->addWidget(createNewKey(tr("E")));
    tmpLayout->addWidget(createNewKey(tr("R")));
    tmpLayout->addWidget(createNewKey(tr("T")));
    tmpLayout->addWidget(createNewKey(tr("Y")));
    tmpLayout->addWidget(createNewKey(tr("U")));
    tmpLayout->addWidget(createNewKey(tr("I")));
    tmpLayout->addWidget(createNewKey(tr("O")));
    tmpLayout->addWidget(createNewKey(tr("P")));
    tmpLayout->addWidget(createNewKey(tr("+")));
    tmpLayout->addWidget(createNewKey(tr("*")));
    tmpLayout->addWidget(createNewKey(tr("-")));
#if !defined(QT_ARCH_MACOSX)
    tmpLayout->insertStretch(-1, 1);
#endif
    layoutReturn->insertLayout(0, tmpLayout, 1); // inserisce la riga della "Q"
	//
	// Stampa linea della "A":
	tmpLayout = new QHBoxLayout;	
	tmp = createNewKey(KEY_CAPS);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2 + 5);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2 + 5);                
    if (this->isEmbeddedKeyboard() == true)
        tmpStyle = QString(EMBEDDED_KEYBOARD);
    tmp->setStyleSheet(QString(DEFAULT_STYLE_BUTTON) + QString(CHANGED_BACKGROUND_BUTTON) + tmpStyle); // segnalato come attivo
	tmpLayout->addWidget(tmp);
    tmpLayout->addWidget(createNewKey(tr("A")));
    tmpLayout->addWidget(createNewKey(tr("S")));
    tmpLayout->addWidget(createNewKey(tr("D")));
    tmpLayout->addWidget(createNewKey(tr("F")));
    tmpLayout->addWidget(createNewKey(tr("G")));
    tmpLayout->addWidget(createNewKey(tr("H")));
    tmpLayout->addWidget(createNewKey(tr("J")));
    tmpLayout->addWidget(createNewKey(tr("K")));
    tmpLayout->addWidget(createNewKey(tr("L")));
    tmpLayout->addWidget(createNewKey(tr("Ã²")));
    tmpLayout->addWidget(createNewKey(tr("Ã ")));
    tmpLayout->addWidget(createNewKey(tr("Ã¹")));
    tmpLayout->addWidget(createNewKey(tr("@")));
	tmpLayout->insertStretch(-1, 1);
#if !defined(QT_ARCH_MACOSX)
	layoutReturn->setSpacing(5);
#endif
    layoutReturn->insertLayout(1, tmpLayout, 1); // inserisce la riga della "A"
	//
	// inserisce il vertical layout all'interno di un horizontal:	
	tmpLayout = new QHBoxLayout;
	tmpLayout->insertLayout(0, layoutReturn, 1);
	//
	// inserisce anche il tasto invio:
	tmp = createNewKey(KEY_RETURN);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmp->setMinimumHeight(tmp->minimumHeight() * 2);
	tmp->setMaximumHeight(tmp->maximumHeight() * 2);
	tmpLayout->addWidget(tmp);
	//
    // inserisce l'horizontal all'interno del layout verticale principale:
    tmpVLayout->insertLayout(1, tmpLayout);
	//	
	// Stampa linea della "Z":
	tmpLayout = new QHBoxLayout;
    tmp = createNewKey(KEY_CUT_LEFT);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
    tmpLayout->addWidget(createNewKey(tr("_")));
    tmpLayout->addWidget(createNewKey(tr("Z")));
    tmpLayout->addWidget(createNewKey(tr("X")));
    tmpLayout->addWidget(createNewKey(tr("C")));
    tmpLayout->addWidget(createNewKey(tr("V")));
    tmpLayout->addWidget(createNewKey(tr("B")));
    tmpLayout->addWidget(createNewKey(tr("N")));
    tmpLayout->addWidget(createNewKey(tr("M")));
    tmpLayout->addWidget(createNewKey(tr(",")));
    tmpLayout->addWidget(createNewKey(tr(";")));
    tmpLayout->addWidget(createNewKey(tr(":")));
    tmpLayout->addWidget(createNewKey(tr(".")));
    tmp = createNewKey(KEY_CTRL_LEFT);
	tmp->setMaximumWidth(tmp->maximumWidth() * 3 + 5);
	tmp->setMinimumWidth(tmp->minimumWidth() * 3 + 5);
	tmpLayout->addWidget(tmp);
	tmpVLayout->insertLayout(2, tmpLayout);
	//	
	// Stampa linea dello SPACE:
	tmpLayout = new QHBoxLayout;
    tmp = createNewKey(KEY_COPY);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
	tmp = createNewKey(KEY_ALT);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
    tmp = createNewKey(KEY_SPACE);
	tmp->setMaximumWidth(tmp->maximumWidth() * 10);
	tmp->setMinimumWidth(tmp->minimumWidth() * 10);
	tmpLayout->addWidget(tmp);
    // password echo button:
    if (this->isEmbeddedKeyboard() == true)
        tmpStyle = QString(KEY_HIDECHAR_EMBEDDED);
    else
        tmpStyle = QString(KEY_HIDECHAR);
    tmp = createNewKey(tmpStyle);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
    //
    tmp = createNewKey(KEY_PASTE);
	tmp->setMaximumWidth(tmp->maximumWidth() * 2);
	tmp->setMinimumWidth(tmp->minimumWidth() * 2);
	tmpLayout->addWidget(tmp);
	tmpVLayout->insertLayout(3, tmpLayout);
#if defined (QT_ARCH_MACOSX)
    tmpVLayout->setMargin(5);
#endif
	//
    // aggancia il layout a tutto il form:
    this->setLayout(tmpVLayout);
//	this->resize(800, 200);
	int h = this->height();
//    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

bool widgetKeyBoard::event(QEvent *e)
{
	switch (e->type()) {
//! [1]
	case QEvent::WindowActivate:
		if (lastFocusedWidget)
			lastFocusedWidget->activateWindow();
		break;
//! [1]
	default:
		break;
	}

	return QWidget::event(e);
}

//! [2]

void widgetKeyBoard::saveFocusWidget(QWidget * /*oldFocus*/, QWidget *newFocus)
{
	if (newFocus != 0 && !this->isAncestorOf(newFocus)) {
		lastFocusedWidget = newFocus;
	}
}
