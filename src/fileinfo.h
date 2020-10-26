#ifndef FILEINFO
#define FILEINFO

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

/// Holds display information about item of table view; device, file, folder, move-to-folder-up-link.
class FileInfo final
{
private:
    /// Types of representable files.
    /// Devices are treated as folders.
    enum class Type : unsigned int {
        file,
        folder,
        up_link
    };

private:
    QString    m_name{};
    QString    m_file_type{};   /// Type of the regular file (MPEEMPEG-4, TIFF, DNG...)
    QString    m_size{};        /// Last modification time of the file.
    QString    m_time{};
    Type       m_type;          /// Type of the item this instance represents (file, folder, up-link).
    bool       m_is_valid;      /// Files/Folders with unicode names are NOT SUPPORTED.

public:
    /// Regular file constructor
    FileInfo(
            QString const&  name,
            QString const&  type,
            QString const&  size,
            QString const&  time,
            bool    const   is_valid
    )
        :   m_name      (name)
        ,   m_file_type (type)
        ,   m_size      (size)
        ,   m_time      (time)
        ,   m_type      (Type::file)
        ,   m_is_valid  (is_valid)
    {}
public:
    /// Folder constructor
    FileInfo(
            QString const&  name,
            bool    const   is_valid
    )
        :   m_name      (name)
        ,   m_type      (Type::folder)
        ,   m_is_valid  (is_valid)
    {}
private:
    /// Up-link constructor
    explicit
    FileInfo()
        :   m_name      ("..")
        ,   m_type      (Type::up_link)
        ,   m_is_valid  (true)
    {}

public:
    /// Up-link constructor has no parameters but
    /// we don't want default constructor to create up-link.
    static
    FileInfo
    make_shortcut_to_parent_folder()
    {
        return FileInfo{};
    }

    /// getters

public:
    QString const&
    get_name() const
    {
        return m_name;
    }
public:
    QString const&
    get_file_type() const
    {
        return m_file_type;
    }
public:
    QString const&
    get_size() const
    {
        return m_size;
    }
public:
    QString const&
    get_time() const
    {
        return m_time;
    }

    /// checkers

public:
    bool
    is_file() const
    {
        return Type::file == m_type;
    }
public:
    bool
    is_folder() const
    {
        return Type::folder == m_type;
    }
public:
    bool
    is_up_link() const
    {
        return Type::up_link == m_type;
    }
public:
    bool
    is_valid() const
    {
        return m_is_valid;
    }
};

#endif // FILEINFO

