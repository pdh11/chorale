#ifndef TREE_MODEL_H
#define TREE_MODEL_H 1

#include <QAbstractItemModel>
#include <QPixmap>
#include "libmediatree/node.h"

namespace choraleqt {

/** A QtAbstractItemModel representing a mediatree:: tree.
 *
 * Suitable for use in a QTreeView in the left-hand pane of an explorer-type
 * view.
 */
class TreeModel: public QAbstractItemModel
{
    Q_OBJECT

    class Item;
    Item *m_root;

    QPixmap m_dir_pixmap;
    QPixmap m_query_pixmap;

public:
    explicit TreeModel(db::Database*);
    ~TreeModel();

    mediatree::NodePtr NodeForIndex(const QModelIndex&);

    // Being a QAbstractItemModel
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
			int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex& parent) const;
};

} // namespace choraleqt

#endif
