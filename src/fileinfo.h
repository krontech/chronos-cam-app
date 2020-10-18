#ifndef FILEINFO
#define FILEINFO

class FileInfo
{
private:
    QString m_name;
    QString m_type;
    QString m_size;
    QString m_time;
    bool    m_is_folder;

public:
    FileInfo(
            QString const& name,
            QString const& type,
            QString const& size,
            QString const& time
    )
        :   m_name      (name)
        ,   m_type      (type)
        ,   m_size      (size)
        ,   m_time      (time)
        ,   m_is_folder (false)
    {}
public:
    FileInfo(
            QString const& name
    )
        :   m_name      (name)
        ,   m_is_folder (true)
    {}

    /// getters

public:
    QString const&
    get_name() const
    {
        return m_name;
    }
public:
    QString const&
    get_type() const
    {
        return m_type;
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
public:
    bool
    is_folder() const
    {
        return m_is_folder;
    }
};

#endif // FILEINFO

