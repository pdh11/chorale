#ifndef CLOUD_DATABASE_WIDGET_H
#define CLOUD_DATABASE_WIDGET_H 1

#include "tree_model.h"
#include <QSplitter>
#include <QPixmap>

class QTreeView;
namespace choraleqt { class BrowseWidget; }
namespace mediadb { class Registry; }
namespace mediadb { class Database; }

namespace cloud {

class Window;

class DatabaseWidget: public QSplitter
{
    Q_OBJECT

    choraleqt::TreeModel m_model;
    QTreeView *m_tree;
    QPixmap m_dir_pixmap;
    QPixmap m_file_pixmap;
    choraleqt::BrowseWidget *m_browse;

public:
    DatabaseWidget(Window *parent, mediadb::Registry *registry, 
		   mediadb::Database *db);
    ~DatabaseWidget();

public slots:
    void OnTreeSelectionChanged(const QModelIndex&);
};

} // namespace cloud

#endif
