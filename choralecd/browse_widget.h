#ifndef BROWSE_WIDGET_H
#define BROWSE_WIDGET_H 1

#include <QListWidget>
#include <vector>
#include "libmediatree/node.h"

namespace choraleqt {

struct IDPair
{
    unsigned int dbid;
    unsigned int fid;
};

class BrowseWidget: public QListWidget
{
    Q_OBJECT;

    unsigned int m_dbid;
    mediatree::NodePtr m_node;

public:
    BrowseWidget(QWidget *parent, unsigned int dbid);
    
    void SetNode(mediatree::NodePtr);

    // Being a QListWidget
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QList<QListWidgetItem*> items) const;
};

class BrowseItem: public QListWidgetItem
{
    mediatree::NodePtr m_node;

public:
    BrowseItem(QListWidget *parent, const QString& text, 
	       mediatree::NodePtr node)
	: QListWidgetItem(text, parent),
	  m_node(node)
    {
    }

    mediatree::NodePtr GetNode() { return m_node; }
};

}; // namespace choraleqt

#endif
