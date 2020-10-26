#ifndef EXTBROWSERPARSER_H
#define EXTBROWSERPARSER_H

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

