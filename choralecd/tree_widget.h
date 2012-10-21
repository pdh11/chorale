#ifndef TREE_WIDGET_H
#define TREE_WIDGET_H 1

#include <q3listview.h>
#include "libmediatree/node.h"

namespace choraleqt {

class NodeItem: public Q3ListViewItem
{
    mediatree::NodePtr m_node;

public:
    NodeItem(Q3ListView *parent, mediatree::NodePtr node)
	: Q3ListViewItem(parent), m_node(node) {}
    NodeItem(Q3ListViewItem *parent, mediatree::NodePtr node)
	: Q3ListViewItem(parent), m_node(node) {}

    void setOpen(bool);
    void setup();
    QString text(int column) const;

    mediatree::NodePtr GetNode() { return m_node; }
};

class TreeWidget: public Q3ListView
{
    Q_OBJECT;

    mediatree::NodePtr m_root;
public:
    TreeWidget(QWidget *parent, mediatree::NodePtr root);
};

}; // namespace choraleqt

#endif
