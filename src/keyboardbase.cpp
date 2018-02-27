#include "keyboardbase.h"

keyboardBase::keyboardBase(QWidget *parent) :
	QWidget(parent)
{

}

keyboardBase::~keyboardBase()
{
	delete this;
}

void keyboardBase::show()
{
	//QDesktopWidget * qdw = QApplication::desktop();

	this->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	QWidget::show();
	//Moving after showing because window height isn't set until show()
	this->move(0,QApplication::desktop()->screenGeometry().height() - height());
	QTimer::singleShot(1, this, SLOT(selectAllInFocusedWidget()));
}

void keyboardBase::selectAllInFocusedWidget(){

	QString senderClass = lastFocusedWidget->metaObject()->className();
	qDebug() << senderClass;

	if(senderClass == "CamTextEdit")
	{
		QTextEdit *textEdit = qobject_cast<QTextEdit*>(lastFocusedWidget);
		if(lastFocusedWidget->objectName() != "KickstarterBackersTextEdit"){
			textEdit->moveCursor(QTextCursor::End);//This and the following line deselect any text that might already be selected
			textEdit->moveCursor(QTextCursor::Left);
			emit characterGenerated(QChar('a')); //insert arbitrary char to have selectAll() have any effect
			emit codeGenerated(KC_BACKSPACE);
			//backspace is to remove the arbitrary char
			textEdit->selectAll();
		}
	}

	if(senderClass == "CamLineEdit")
	{
		QLineEdit *lineEdit = qobject_cast<QLineEdit*>(lastFocusedWidget);
		if(lastFocusedWidget->objectName() != "linePassword"){//or else using KC_RIGHT on the service password lineEdit would cause the tabWidget on utilWindow to advance to the next tab
			lineEdit->deselect();
			emit characterGenerated(QChar('a')); //insert arbitrary char to have selectAll() have any effect
			emit codeGenerated(KC_BACKSPACE);
			lineEdit->selectAll();
		}
	}

	if(senderClass.contains("SpinBox"))
	{
		QAbstractSpinBox *spinBox = qobject_cast<QAbstractSpinBox*>(lastFocusedWidget);
		emit characterGenerated(QChar('a')); //insert arbitrary char to have selectAll() have any effect
		spinBox->selectAll();
		//spinBoxes and doubleSpinBoxes both inherit from QAbstractSpinBox
		//these will not accept alphabetic chars, so a backspace is not needed in that case
	}


}

bool keyboardBase::event(QEvent *e)
{
	switch (e->type()) {

	case QEvent::WindowActivate:
		if (lastFocusedWidget)
		{
			lastFocusedWidget->activateWindow();
			qDebug() << "keyboardBase::event lastFocusedWidget activated" << lastFocusedWidget;
		}
		break;
	default:
		break;
	}

	return QWidget::event(e);
}



void keyboardBase::saveFocusWidget(QWidget * /*oldFocus*/, QWidget *newFocus)
{
	if (newFocus != 0 && !this->isAncestorOf(newFocus)) {
		lastFocusedWidget = newFocus;
		qDebug() << "keyboardBase::saveFocusWidget lastFocusedWidget set to" << newFocus;
	}
}


void keyboardBase::on_up_clicked()
{
	emit codeGenerated(KC_UP);
}

void keyboardBase::on_down_clicked()
{
	emit codeGenerated(KC_DOWN);
}

void keyboardBase::on_left_clicked()
{
	emit codeGenerated(KC_LEFT);
}

void keyboardBase::on_right_clicked()
{
	emit codeGenerated(KC_RIGHT);
}

void keyboardBase::on_enter_clicked()
{
	emit codeGenerated(KC_ENTER);
}

void keyboardBase::on_close_clicked()
{
	emit codeGenerated(KC_CLOSE);
}

void keyboardBase::on_back_clicked()
{
	emit codeGenerated(KC_BACKSPACE);
}

