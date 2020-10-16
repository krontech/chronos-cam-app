#ifndef FILEINFOMODEL
#define FILEINFOMODEL

#include <QAbstractTableModel>
#include <cassert>
#include "fileinfo.h"

class FileInfoModel : public QAbstractTableModel
{
private:
    QList<FileInfo> m_data;
public:
    FileInfoModel(
            QObject* parent = {} )
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
           case 1: return file_info.get_type();
           case 2: return file_info.get_size();
           case 3: return file_info.get_time();
       };

       assert( false );

       return {};
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
    append(
            FileInfo const& file_info )
    {
        beginInsertRows(
            {},
            m_data.count(),
            m_data.count() );
       m_data.append( file_info );
       endInsertRows();
    }
};

#endif // FILEINFOMODEL

