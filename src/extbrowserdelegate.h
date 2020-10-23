#ifndef EXTBROWSERDELEGATE_H
#define EXTBROWSERDELEGATE_H


#include <QItemDelegate>
#include <QImage>

class FileInfoModel;
class ExtBrowser;

class ExtBrowserDelegate : public QItemDelegate
{
    Q_OBJECT

private:
    FileInfoModel*  m_model;
private:
    QImage const folder_icon   {":/qss/assets/images/folder_icon.png"};
    QImage const folder_icon_up{":/qss/assets/images/folder_icon_up.png"};

public:
    ExtBrowserDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
public:
    void
    set_model(
        FileInfoModel* const model );
};

#endif // EXTBROWSERDELEGATE_H
