#ifndef EXTBROWSERDELEGATE_H
#define EXTBROWSERDELEGATE_H


#include <QItemDelegate>

class ExtBrowserDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ExtBrowserDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

#endif // EXTBROWSERDELEGATE_H
