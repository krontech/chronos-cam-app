#ifndef EXTBROWSERDELEGATE_H
#define EXTBROWSERDELEGATE_H


#include <QItemDelegate>

class FileInfoModel;
class ExtBrowser;

class ExtBrowserDelegate : public QItemDelegate
{
    Q_OBJECT

    FileInfoModel*  m_model;
    ExtBrowser*     m_browser;

public:
    ExtBrowserDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
public:
    void
    set_model(
        FileInfoModel* const model,
        ExtBrowser*    const browser );
};

#endif // EXTBROWSERDELEGATE_H
