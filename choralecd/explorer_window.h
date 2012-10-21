#ifndef QEXPLORER_WINDOW_H
#define QEXPLORER_WINDOW_H 1

#include <qdialog.h>
#include "libmediatree/node.h"

class QSplitter;
class Q3ListViewItem;

namespace mediadb { class Database; };
namespace mediadb { class Registry; };

namespace choraleqt {

class TagTable;
class TreeWidget;
class BrowseWidget;

class ExplorerWindow: public QDialog
{
    Q_OBJECT;

    mediatree::NodePtr m_root;
    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

    TreeWidget *m_tree;
    QSplitter *m_splitter;
    TagTable *m_table;
    BrowseWidget *m_browse;

public:
    ExplorerWindow(mediatree::NodePtr root, 
		   mediadb::Database *db,
		   mediadb::Registry *registry);

public slots:
    void OnTreeSelectionChanged(Q3ListViewItem*);
};

}; // namespace choraleqt

#endif
