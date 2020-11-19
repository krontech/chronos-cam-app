#ifndef FILEINFOMODEL
#define FILEINFOMODEL

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

#include <QAbstractTableModel>
#include "fileinfo.h"

class FileInfoModel : public QAbstractTableModel
{
    friend class ExtBrowser;
    friend class ExtBrowserDelegate;
private:
    QList<FileInfo> m_data;
public:
    FileInfoModel(
            QObject* parent = {}
    )
        : QAbstractTableModel{parent}
    {}
public:
    int
    rowCount(
            QModelIndex const& ) const override
    {
        return m_data.count();
    }
public:
    int
    columnCount(
            QModelIndex const& ) const override
    {
        return 4;
    }
public:
    QVariant
    data(
            QModelIndex const&  index,
            int                 role ) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
        {
            return {};
        }
        auto const& file_info = m_data[index.row()];
        switch (index.column())
        {
            case 0: return file_info.get_name();
            case 1: return file_info.get_file_type();
            case 2: return file_info.get_size();
            case 3: return file_info.get_time();
        };

        Q_ASSERT ( false );

        return {};
    }
private:
    QList<FileInfo> const&
    get_data() const
    {
        return m_data;
    }
public:
    QVariant
    headerData(
            int             section,
            Qt::Orientation orientation,
            int             role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return {};
        }
        switch (section)
        {
            case 0: return "Name";
            case 1: return "Type";
            case 2: return "Size";
            case 3: return "Time";
        }

        Q_ASSERT ( false );

        return {};
    }
public:
    void
    set_data(
            QList<FileInfo> const& data )
    {
        beginResetModel();

        m_data = data;

        endResetModel();
    }
public:
    Qt::ItemFlags
    flags(
            const QModelIndex &index ) const override
    {
        int const row = index.row();

        if ( m_data.length() <= row )
        {
            return QAbstractTableModel::flags( index );
        }

        auto const fileinfo = m_data.at( row );

        /// make invalid items unselectable
        if ( !fileinfo.is_valid() )
        {
            /// Has to be NoItemFlags; can't be ItemIsSelectable
            /// because it doesn't work when user drag selects through
            /// invalid item.
            return Qt::NoItemFlags;

        }

        return QAbstractTableModel::flags( index );
    }
};

#endif // FILEINFOMODEL

