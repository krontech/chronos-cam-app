#include <QtGui>
#include "extbrowserdelegate.h"
#include "fileinfomodel.h"

#include <iostream>

ExtBrowserDelegate::ExtBrowserDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}


void ExtBrowserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
/*    bool isSelected = option.state & QStyle::State_Selected;
    std::cout << "D" << index.row() << " " << index.column() << " " << isSelected << std::endl;*/

    assert ( index.row() < m_model->get_data().length() );

    auto const& file_info =
        m_model->get_data().at( index.row() );

    if(   ( 0 != index.column() )
        | ( file_info.is_file() ) )
    {
        QItemDelegate::paint( painter, option, index );
        return;
    }

    QStyleOptionButton button;
    QRect r = option.rect;//getting the rect of the cell
    int x,y,w,h;
    x = r.left();// + r.width() - 30;//the X coordinate
    y = r.top();//the Y coordinate
    w = 100;//button width
    h = 30;//button height
    button.rect = QRect(x,y,w,h);
    button.text = file_info.get_name();
    button.state = QStyle::State_Enabled;

    QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter);
}

bool ExtBrowserDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    //return QItemDelegate::editorEvent(event,model,option,index);

    if( QEvent::MouseButtonRelease == event->type()
        && 0==index.column()
        && !m_model->get_data().at( index.row() ).is_file() )
    {
        QMouseEvent * e = (QMouseEvent *)event;
        int clickX = e->x();
        int clickY = e->y();

        QRect r = option.rect;//getting the rect of the cell
        int x,y,w,h;
        x = r.left();// + r.width() - 30;//the X coordinate
        y = r.top();//the Y coordinate
        w = 100;//button width
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

void ExtBrowserDelegate::set_model( FileInfoModel const*const model )
{
    m_model = model;

}
