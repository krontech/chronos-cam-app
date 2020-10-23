#include <QtGui>
#include "extbrowserdelegate.h"
#include "fileinfomodel.h"

ExtBrowserDelegate::ExtBrowserDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void ExtBrowserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto const& file_info =
        m_model->get_data().at( index.row() );

    /// if it's 'file type' column and it's a folder draw the icons
    if(   ( 1 == index.column() )
        & ( !file_info.is_file() ) )
    {
        QRect r = option.rect; //getting the rect of the cell
        int x,y,w,h;
        x = r.left();
        y = r.top();
        w = 60; // button width
        h = 40; // button height

        painter->save();

            painter->drawImage(
                QRect(x,y,w,h),
                file_info.is_folder()
                ?
                    folder_icon
                :
                    folder_icon_up);

        painter->restore();

        return;
    }

    QItemDelegate::paint( painter, option, index );
}

void
ExtBrowserDelegate::set_model(
        FileInfoModel* const model )
{
    m_model     = model;
}
