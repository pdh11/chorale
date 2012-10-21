#ifndef QEXPLORER_WINDOW_H
#define QEXPLORER_WINDOW_H 1

#include <qdialog.h>
#include "libmediatree/node.h"

class QSplitter;
class Q3ListViewItem;
class QModelIndex;

namespace mediadb { class Database; };
namespace mediadb { class Registry; };

namespace choraleqt {

class TagTable;
class TreeWidget;
class BrowseWidget;
class TreeModel;

class ExplorerWindow: public QDialog
{
    Q_OBJECT;

    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

    QSplitter *m_splitter;
    TagTable *m_table;
    BrowseWidget *m_browse;
    TreeModel *m_treemodel;

public:
    ExplorerWindow(mediadb::Database *db,
		   mediadb::Registry *registry);
    ~ExplorerWindow();

public slots:
    void OnTreeSelectionChanged(Q3ListViewItem*);
    void OnTreeSelectionChanged(const QModelIndex&);
};

}; // namespace choraleqt

#endif
