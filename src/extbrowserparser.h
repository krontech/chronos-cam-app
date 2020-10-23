#ifndef EXTBROWSERPARSER_H
#define EXTBROWSERPARSER_H

#include <QString>
#include <QList>
#include "fileinfo.h"
#include "storagedevice_info.h"

/// Parses the string containing the output of 'ls' system call.
/// @hide_regular_files: In folder selection mode only folders are shown.
QList<FileInfo>
parse_ls_output(
        QString const&  ls_output,
        bool    const   hide_regular_files );

QList<StorageDevice_Info>
parse_lsblk_output(
        QString const& lsblk_output );

void
get_network_shares(
        QList<StorageDevice_Info>& storage_devices );

#endif // EXTBROWSERPARSER_H

