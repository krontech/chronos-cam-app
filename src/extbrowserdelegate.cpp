/****************************************************************************
 *  Copyright (C) 2013-2020 Kron Technologies Inc <http://www.krontech.ca>. *
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
        y = r.top()+10;
        w = 30; // button width
        h = 20; // button height

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
