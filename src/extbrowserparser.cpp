

#include <QList>
#include <QString>
#include <QStringList>
#include <cassert>
#include "fileinfo.h"
#include "storagedevice_info.h"

#define EXTBROWSER_EXTRA_CHECKS

static
void
add_shortcut_to_parent_folder(
        QList<FileInfo>&     ret )
{
    ret.append(
        FileInfo::make_shortcut_to_parent_folder() );
}

static
bool
check_if_folder(
        QString const& line )
{
    assert( 0 < line.size() );

    return QChar('/') == line.at( line.size()-1 );
}

static
void
trim_string(
        QString&    ret,
        bool const  is_folder )
{
    assert ( 0 < ret.length() );
    assert ( QChar('"') == ret.at( 0 ) );

    ret = ret.remove( 0, 1 );

    auto const ret_length = ret.length();

    if ( is_folder )
    {
    assert (  1 < ret_length );
    assert (  QChar('/')==ret.at(ret_length-1) && QChar('"')==ret.at(ret_length-2) );
    }
    else
    {
    assert ( 0 < ret_length );
    assert ( QChar('"')==ret.at(ret_length-1) );
    }

    ret.chop(
        is_folder
        ?
            2
        :
            1 );
}

static
QString
compute_file_name(
        QStringList const& line,
        bool        const  is_folder )
{
    auto const line_length = line.length();

    assert ( 7 <= line_length );
    assert ( 0 < line.at(6).size() );
    assert ( QChar('"') == line.at(6).at(0) );

    auto const line_length_m1 = line_length - 1;

    QString ret;

    for(
        int i = 6;
        i < line_length_m1;
        ++i )
    {
        ret += line.at( i ) + QChar(' ');
    }

    ret += line.at( line_length_m1 );

    trim_string(
        ret,
        is_folder );

    return ret;
}

static
bool
check_if_filename_valid(
        QString const& file_name )
{
    return
        -1 == file_name.indexOf( QChar('?') );
}

static
QString
compute_file_type(
        QString const& file_name )
{
    int const last_ix =
        file_name.lastIndexOf( QChar('.') );

    if ( -1 == last_ix )
    {
        return {};
    }

    QString const extension =
        file_name.right(
            file_name.length() - last_ix - 1 );

    if ( "mp4" == extension )
    {
        return "H.264";
    }

    if ( "raw" == extension )
    {
        return "RAW";
    }

    if ( "dng" == extension )
    {
        return "DNG";
    }

    if ( "tiff" == extension )
    {
        return "TIFF";
    }

    return {};

}

static
QString
compute_file_size(
        QStringList const& tokens )
{
    return tokens.at( 2 );
}

static
QString
compute_file_time(
        QStringList const& tokens )
{
    return
         tokens.at( 3 ) + QChar(' ')
        +tokens.at( 4 ) + QChar(' ')
        +tokens.at( 5 );
}

QList<FileInfo>
parse_ls_output(
        QString const&  ls_output,
        bool    const   hide_regular_files )
{
    QList<FileInfo> ret;

    if ( 0 == ls_output.length() )
    {
        return ret;
    }

    add_shortcut_to_parent_folder(
        ret );

    auto const output_lines =
        ls_output.split(
            QChar{'\n'},
            QString::SkipEmptyParts );

    auto const line_count = output_lines.size();

    assert ( 0 < line_count );
    assert ( output_lines.at(0).startsWith( "total", Qt::CaseInsensitive ) );

    ret.reserve( line_count - 1 );

    for(
        int i = 1;
        i < line_count;
        ++i )
    {
        auto const current_line = output_lines.at( i );

        auto const tokens =
            current_line.split(
                QChar(' '),
                QString::SkipEmptyParts );

        bool const is_folder =
            check_if_folder( current_line );

        auto const file_name =
            compute_file_name(
                tokens,
                is_folder );

        bool const is_valid =
            check_if_filename_valid( file_name );

        if ( is_folder )
        {
            ret.append(
                {   file_name,
                    is_valid } );

            continue;
        }

        if ( hide_regular_files )
        {
            continue;
        }

        ret.append(
            {   file_name,
                compute_file_type( file_name ),
                compute_file_size( tokens ),
                compute_file_time( tokens ),
                is_valid });
    }

    return ret;
}

static
bool
has_label(
        int const tokens_count )
{
    return 3 == tokens_count;
}

QList<StorageDevice_Info>
parse_lsblk_output(
        QString const& lsblk_output )
{
    QList<StorageDevice_Info> ret;

    if ( 0 == lsblk_output.length() )
    {
        return ret;
    }

    auto const output_lines =
        lsblk_output.split(
            QChar{'\n'},
            QString::SkipEmptyParts );

    auto const line_count = output_lines.size();

    assert ( 0 < line_count );

    ret.reserve( line_count );

    for(
        int i = 0;
        i < line_count;
        ++i )
    {
        auto const current_line = output_lines.at( i );

        auto const tokens =
            current_line.split(
                QChar(' '),
                QString::SkipEmptyParts );

        auto const tokens_count = tokens.size();

        assert ( 2 <= tokens_count && 3 >= tokens_count );

        QString const& mount_folder     = tokens.at(0);
        QString const& storage_capacity = tokens.at(1);

        if ( has_label( tokens_count ) )
        {
            QString const& storage_label = tokens.at(2);

            ret.append(
                {   mount_folder,
                    storage_label } );

            continue;
        }

        ret.append(
            {   mount_folder,
                QString{"SD_"}+storage_capacity } );
    }

    return ret;
}

