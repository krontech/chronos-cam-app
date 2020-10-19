#include <QtGui>
#include "extbrowserdelegate.h"
#include "fileinfomodel.h"
#include "extbrowser.h"
#include "movedirection.h"
#include "extbrowserparser.h"

ExtBrowserDelegate::ExtBrowserDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}


void ExtBrowserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
/*    bool isSelected = option.state & QStyle::State_Selected;
    std::cout << "D" << index.row() << " " << index.column() << " " << isSelected << std::endl;*/

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

    auto const& file_info =
        m_model->get_data().at( index.row() );

    if( QEvent::MouseButtonRelease == event->type()
        && 0==index.column()
        && !file_info.is_file() )
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
                MoveDirection direction = MoveDirection::descend;
                if ( file_info.is_up_link() )
                {
                    direction = MoveDirection::ascend;
                } else {
                    assert ( file_info.is_folder() );
                }

                QString const ls_output =
                    m_browser->move_to_folder_and_get_contents(
                        direction,
                        file_info.get_name() );

                auto const model_data =
                    parse_ls_output(
                        ls_output,
                        m_browser->is_at_root() );

                m_model->set_data( model_data );
            }
        return true;
    }

    return QItemDelegate::editorEvent(event,model,option,index);

    return true;
}

void
ExtBrowserDelegate::set_model(
        FileInfoModel* const model,
        ExtBrowser*    const browser )
{
    m_model     = model;
    m_browser   = browser;
}
