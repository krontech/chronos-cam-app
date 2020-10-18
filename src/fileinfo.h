#ifndef FILEINFO
#define FILEINFO

class FileInfo final
{
private:
    enum class Type : unsigned int {
        file,
        folder,
        up_link
    };

private:
    QString    m_name{};
    QString    m_file_type{};
    QString    m_size{};
    QString    m_time{};
    Type       m_type;
    bool       m_is_valid;

public:
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
    FileInfo(
            QString const&  name,
            bool    const   is_valid
    )
        :   m_name      (name)
        ,   m_type      (Type::folder)
        ,   m_is_valid  (is_valid)
    {}
private:
    explicit
    FileInfo()
        :   m_name      ("Up")
        ,   m_type      (Type::up_link)
        ,   m_is_valid  (true)
    {}

public:
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
    is_valid() const
    {
        return m_is_valid;
    }
};

#endif // FILEINFO

