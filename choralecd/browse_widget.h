#ifndef BROWSE_WIDGET_H
#define BROWSE_WIDGET_H 1

#include <QListWidget>
#include <vector>
#include "libmediatree/node.h"

namespace choraleqt {

/** A (database, file-id) pair used by drag-and-drop support.
 */
struct IDPair
{
    unsigned int dbid;
    unsigned int fid;
};

/** A QListWidget representing the children of a mediatree::NodePtr.
 */
class BrowseWidget: public QListWidget
{
    Q_OBJECT

    unsigned int m_dbid;
    mediatree::NodePtr m_node;
    QPixmap *m_dir_pixmap;
    QPixmap *m_file_pixmap;

public:
    BrowseWidget(QWidget *parent, unsigned int dbid,
		 QPixmap *dir_pixmap = NULL, QPixmap *file_pixmap = NULL);
    
    void SetNode(mediatree::NodePtr);

    // Being a QListWidget
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QList<QListWidgetItem*> items) const;
};

/** A QListWidgetItem representing a child mediatree::NodePtr.
 */
class BrowseItem: public QListWidgetItem
{
    mediatree::NodePtr m_node;

public:
    BrowseItem(QListWidget *parent, const QString& title, 
	       mediatree::NodePtr node)
	: QListWidgetItem(title, parent),
	  m_node(node)
    {
    }

    mediatree::NodePtr GetNode() { return m_node; }
};

} // namespace choraleqt

#endif
