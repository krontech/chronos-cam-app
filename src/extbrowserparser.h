#ifndef EXTBROWSERPARSER_H
#define EXTBROWSERPARSER_H

#include <QString>
#include <QList>
#include "fileinfo.h"

QList<FileInfo>
parse_ls_output(
        QString const&  ls_output,
        bool    const   is_root );

#endif // EXTBROWSERPARSER_H

