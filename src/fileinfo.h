#ifndef FILEINFO
#define FILEINFO

class FileInfo
{
private:
    QString m_name;
    QString m_type;
    QString m_size;
    QString m_time;

public:
    FileInfo(
            QString const& name,
            QString const& type,
            QString const& size,
            QString const& time
    )
        :   m_name  (name)
        ,   m_type  (type)
        ,   m_size  (size)
        ,   m_time  (time)
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
};

#endif // FILEINFO

