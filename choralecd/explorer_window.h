#ifndef QEXPLORER_WINDOW_H
#define QEXPLORER_WINDOW_H 1

#include <QMainWindow>
#include "libmediatree/node.h"

class QSplitter;
class QModelIndex;

namespace mediadb { class Database; }
namespace mediadb { class Registry; }

namespace choraleqt {

class TagTable;
class BrowseWidget;
class TreeModel;

/** Top-level window offering an Explorer-like view of a mediadb::Database.
 */
class ExplorerWindow: public QMainWindow
{
    Q_OBJECT

    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

    QSplitter *m_splitter;
    BrowseWidget *m_browse;
    TreeModel *m_treemodel;

public:
    ExplorerWindow(mediadb::Database *db,
		   mediadb::Registry *registry);
    ~ExplorerWindow();

public slots:
    void OnTreeSelectionChanged(const QModelIndex&);
};

} // namespace choraleqt

#endif
