#ifndef EXTBROWSERDELEGATE_H
#define EXTBROWSERDELEGATE_H

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


#include <QItemDelegate>
#include <QImage>

class FileInfoModel;
class ExtBrowser;

class ExtBrowserDelegate : public QItemDelegate
{
    Q_OBJECT

private:
    FileInfoModel*  m_model;
private:
    QImage const folder_icon   {":/qss/assets/images/folder_icon.png"};
    QImage const folder_icon_up{":/qss/assets/images/folder_icon_up.png"};

public:
    ExtBrowserDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
public:
    void
    set_model(
        FileInfoModel* const model );
};

#endif // EXTBROWSERDELEGATE_H
