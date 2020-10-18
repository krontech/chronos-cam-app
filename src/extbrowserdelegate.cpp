#include <QtGui>
#include "extbrowserdelegate.h"

#include <iostream>

ExtBrowserDelegate::ExtBrowserDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}


void ExtBrowserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
/*    bool isSelected = option.state & QStyle::State_Selected;
    std::cout << "D" << index.row() << " " << index.column() << " " << isSelected << std::endl;*/

    if( 0 != index.column() )
    {
        QItemDelegate::paint( painter, option, index );
        return;
    }

    QStyleOptionButton button;
    QRect r = option.rect;//getting the rect of the cell
    int x,y,w,h;
    x = r.left();// + r.width() - 30;//the X coordinate
    y = r.top();//the Y coordinate
    w = 30;//button width
    h = 30;//button height
    button.rect = QRect(x,y,w,h);
    button.text = "=^.^=";
    button.state = QStyle::State_Enabled;

    QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter);
}

bool ExtBrowserDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    //return QItemDelegate::editorEvent(event,model,option,index);

    if( event->type() == QEvent::MouseButtonRelease
            && 0==index.column() )
    {
        QMouseEvent * e = (QMouseEvent *)event;
        int clickX = e->x();
        int clickY = e->y();

        QRect r = option.rect;//getting the rect of the cell
        int x,y,w,h;
        x = r.left();// + r.width() - 30;//the X coordinate
        y = r.top();//the Y coordinate
        w = 30;//button width
        h = 30;//button height

        if( clickX > x && clickX < x + w )
            if( clickY > y && clickY < y + h )
            {
                std::cout << "DINO " << index.row() << " " << index.column() << std::endl;
               /* QDialog * d = new QDialog();
                d->setGeometry(0,0,100,100);
                d->show();*/
            }
        return true;
    }

    return QItemDelegate::editorEvent(event,model,option,index);

    return true;
}

