#ifndef EXTBROWSERDELEGATE_H
#define EXTBROWSERDELEGATE_H


#include <QItemDelegate>

class FileInfoModel;

class ExtBrowserDelegate : public QItemDelegate
{
    Q_OBJECT

    const FileInfoModel* m_model;

public:
    ExtBrowserDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    void set_model( FileInfoModel const*const model );
};

#endif // EXTBROWSERDELEGATE_H
