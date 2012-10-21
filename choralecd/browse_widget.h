#ifndef BROWSE_WIDGET_H
#define BROWSE_WIDGET_H 1

#include <q3iconview.h>
#include <vector>
#include "libmediatree/node.h"

namespace choraleqt {

struct IDPair
{
    unsigned int dbid;
    unsigned int fid;
};

class BrowseIconDrag: public Q3IconDrag
{
    Q_OBJECT;
    std::vector<IDPair> m_pairs;

public: 
    BrowseIconDrag(QWidget *dragSource);

    const char* format(int i) const;
    QByteArray encodedData(const char* mime) const;
    static bool canDecode(QMimeSource* e);
    void append(const Q3IconDragItem &item, const QRect &pr,
		const QRect &tr, unsigned int dbid, unsigned int fid);
};

class BrowseWidget: public Q3IconView
{
    Q_OBJECT;

    unsigned int m_dbid;
    mediatree::NodePtr m_node;

public:
    BrowseWidget(QWidget *parent, unsigned int dbid);

    Q3DragObject *dragObject();
    
    void SetNode(mediatree::NodePtr);
};

class BrowseItem: public Q3IconViewItem
{
    mediatree::NodePtr m_node;

public:
    BrowseItem(Q3IconView *parent, const QString& text, const QPixmap& icon,
	       mediatree::NodePtr node)
	: Q3IconViewItem(parent, text, icon),
	  m_node(node)
    {
    }

    mediatree::NodePtr GetNode() { return m_node; }
};

}; // namespace choraleqt

#endif
