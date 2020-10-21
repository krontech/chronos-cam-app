#ifndef FILEINFOMODEL
#define FILEINFOMODEL

#include <QAbstractTableModel>
#include <cassert>
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
        const auto& file_info = m_data[index.row()];
        switch (index.column())
        {
            case 0: return file_info.get_name();
            case 1: return file_info.get_file_type();
            case 2: return file_info.get_size();
            case 3: return file_info.get_time();
        };

        assert( false );

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

        assert( false );

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

        /*beginInsertRows(
            {},
            m_data.count(),
            m_data.count() );
       m_data.append( file_info );
       endInsertRows();*/
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

        bool const is_file = fileinfo.is_file();

        /*if (   (!is_file)
             | (!fileinfo.is_valid()) )
        {
            //return Qt::NoItemFlags;
            return Qt::ItemIsSelectable;

        }*/
        return QAbstractTableModel::flags( index );
    }
};

#endif // FILEINFOMODEL

