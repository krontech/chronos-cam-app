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
#include <QList>
#include <QString>
#include <QStringList>
#include <cassert>
#include <mntent.h>
#include "fileinfo.h"
#include "storagedevice_info.h"
#include "defines.h"

/// Add 'Up' link to the files listing.
static
void
add_shortcut_to_parent_folder(
        QList<FileInfo>&     ret )
{
    ret.append(
        FileInfo::make_shortcut_to_parent_folder() );
}

/// Due to flags passed to 'ls' folders end with '/'
static
bool
check_if_folder(
        QString const& line )
{
    assert( 0 < line.size() );

    return QChar('/') == line.at( line.size()-1 );
}

/// Remove '"' from beginning/end of file names
/// and '/' from the end of folder names.
///
/// Due to flags passed to 'ls' file names are enveloped in quotes.
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

/// Join the space separated parts of file name and trim
/// the resulting string.
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

/// Handles unicode characters by marking them as invalid.
/// Unicode is NOT SUPPORTED!
static
bool
check_if_filename_valid(
        QString const& file_name )
{
    return
        -1 == file_name.indexOf( QChar('?') );
}

/// Hardcoded pretty names for Chronos supported file types.
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
        return "MPEG-4";
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

/// formats file time string
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

/// Example ls output:
///
/// total 576M
/// drwxr-xr-x 2  16K Oct 18  2020 "folder of tiffs"/
/// drwxr-xr-x 3  16K Oct 18  2020 "nested_folder"/
/// drwxr-xr-x 2  16K Oct 18  2020 "empty_folder"/
/// -rwxr-xr-x 1 4.9K Oct 18  2020 "vid_2020-09-11_23-24_25.mp4"
/// -rwxr-xr-x 1 4.9K Oct 18  2020 "vid_2020-09-23_07-19_01.mp4"
/// -rwxr-xr-x 1 4.9K Oct 18  2020 "vid_2020-09-23_10-33_11.mp4"
/// -rwxr-xr-x 1    0 Oct 18  2020 "unredable.mp4"
/// -r-xr-xr-x 1    0 Oct 18  2020 "undeletable.mp4"
/// -rwxr-xr-x 1    0 Oct 18  2020 "???.mp4"
/// -rwxr-xr-x 1    0 Oct 18  2020 "???.mp4"
/// -rwxr-xr-x 1   12 Oct 18  2020 "spaces in name.mp4"
/// -rwxr-xr-x 1   18 Oct 18  2020 "another_file"
/// -rwxr-xr-x 1    5 Oct 18  2020 "someFile.txt"
/// -rwxr-xr-x 1 115M May  5 16:53 "vid_2015-05-05_16-52-04.mp4"
/// -rwxr-xr-x 1 199M May  5 16:38 "vid_2015-05-05_16-36-25.mp4"
/// -rwxr-xr-x 1 263M May  5 16:27 "vid_2015-05-05_16-24-30.mp4"
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

/// External USB and eSATA drives might not have label. This function
/// generates informative name for the drive containing drive vandor,
/// model and partition size.
static
QString
compute_name_string_for_unnamed_HD(
        QString const& mount_point )
{
    std::string const device = mount_point.mid( 7, 3 ).toStdString();

    unsigned int len;
    char model[256];
    char vendor[256];
    char modelPath[256];
    char vendorPath[256];

    // Produce the paths to read the model and vendor strings
    sprintf(modelPath, "/sys/block/%s/device/model", device.c_str() );
    sprintf(vendorPath, "/sys/block/%s/device/vendor", device.c_str() );

    // Read the model and vendor strings for this block device
    FILE * fp = fopen(modelPath, "r");
    if(fp)
    {
        len = fread(model, 1, 255, fp);
        model[len] = '\0';
        fclose(fp);
    }
    else
        model[0] = '\0';

    fp = fopen(vendorPath, "r");
    if(fp)
    {
        len = fread(vendor, 1, 255, fp);
        vendor[len] = '\0';
        fclose(fp);
    }
    else
        vendor[0] = '\0';

    //remove all trailing whitespace and carrage returns
    int i;
    for(i = strlen(vendor) - 1; ' ' == vendor[i] || '\n' == vendor[i]; i--) {};	//Search back from the end and put a null at the first trailing space
    vendor[i + 1] = '\0';

    for(i = strlen(model) - 1; ' ' == model[i] || '\n' == model[i]; i--) {};	//Search back from the end and put a null at the first trailing space
    model[i + 1] = '\0';

    QString const label = QString{vendor} + QChar{' '} + QString{model};

    return label;
}

/// Example lsblk output:
///
/// /media/mmcblk1p1 14.7G SANJAYSSD
/// /media/mmcblk1p2 9.7G AnotherP
/// /media/mmcblk1p3 5.4G
QList<StorageDevice_Info>
parse_lsblk_output(
        QString const& lsblk_output )
{
    QList<StorageDevice_Info> storage_devices;

    /// we might have no devices connected; that's legal
    if ( 0 == lsblk_output.length() )
    {
        return storage_devices;
    }

    auto const output_lines =
        lsblk_output.split(
            QChar{'\n'},
            QString::SkipEmptyParts );

    auto const line_count = output_lines.size();

    assert ( 0 < line_count );

    storage_devices.reserve( line_count );

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

            storage_devices.append(
                {   mount_folder,
                    storage_label } );

            continue;
        }

        QString prefix;

        if ( mount_folder.startsWith( "/media/mmcblk") )
        {
            prefix = "SD_";
        }

        if ( mount_folder.startsWith( "/media/sd") )
        {
            QString const generated_label = compute_name_string_for_unnamed_HD( mount_folder );

            if ( 1 >= generated_label.length() )
            {
                prefix = "HD_";
            } else
            {
                prefix = generated_label + QChar{' '};
            }
        }

        storage_devices.append(
            {   mount_folder,
                prefix + storage_capacity } );
    }

    return storage_devices;
}

void
get_network_shares(
        QList<StorageDevice_Info>& storage_devices )
{
    FILE * mtab = setmntent("/etc/mtab", "r");
    struct mntent* m;
    struct mntent mnt;
    char strings[4096];

    while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings))))
    {
        if (mnt.mnt_dir == NULL) continue;

        if (   (0 == strcmp(mnt.mnt_dir, SMB_STORAGE_MOUNT))
            || (0 == strcmp(mnt.mnt_dir, NFS_STORAGE_MOUNT)))
        {
            QString const fsname{ mnt.mnt_fsname };

            auto const ix =
                fsname.indexOf( QChar{'/'}, 2 );

            if ( -1 == ix )
            {
                continue;
            }

            QString label;

            if ( 1 < (fsname.length() - ix) )
            {
                label = fsname.right( fsname.length() - ix - 1 );
            }
            else
            {
                label = "network";
            }

            storage_devices.append(
                {   mnt.mnt_dir,
                    label } );
        }
        else {
            continue;
        }
    }

    endmntent(mtab);
}

