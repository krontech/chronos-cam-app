

#include <QList>
#include <QString>
#include <QStringList>
#include <cassert>
#include "fileinfo.h"

#define EXTBROWSER_EXTRA_CHECKS

static
void
add_shortcut_to_parent_folder(
        bool           const is_root,
        QList<FileInfo>&     ret )
{
    if ( false == is_root )
    {
        ret.append(
            FileInfo::make_shortcut_to_parent_folder() );
    }
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
        return "?";
    }

    return
        file_name.right(
            file_name.length() - last_ix - 1 );
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
        bool    const   is_root )
{
    QList<FileInfo> ret;

    if ( 0 == ls_output.length() )
    {
        return ret;
    }

    add_shortcut_to_parent_folder(
        is_root,
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
                    is_valid} );

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

