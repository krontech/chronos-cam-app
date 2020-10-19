#ifndef EXTBROWSERPARSER_H
#define EXTBROWSERPARSER_H

#include <QString>
#include <QList>
#include "fileinfo.h"

QList<FileInfo>
parse_ls_output(
        QString const&  ls_output,
        bool    const   is_root,
        bool    const   hide_regular_files );

#endif // EXTBROWSERPARSER_H

