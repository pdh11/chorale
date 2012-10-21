/* explorer_window.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "explorer_window.h"
#include "tagtable.h"
#include "tree_widget.h"
#include "browse_widget.h"
#include "libmediadb/registry.h"
#include "tree_model.h"
#include <qsplitter.h>
#include <qlayout.h>
#include <QTreeView>
#include <QHeaderView>
#include "libmediadb/db.h"

namespace choraleqt {

ExplorerWindow::ExplorerWindow(mediadb::Database *db,
			       mediadb::Registry *registry)
    : QDialog(NULL, NULL),
      m_db(db),
      m_registry(registry)
{
    QVBoxLayout *vlayout = new QVBoxLayout(this, 0, 1);

    setAttribute(Qt::WA_DeleteOnClose);

    m_splitter = new QSplitter(this);
    vlayout->addWidget(m_splitter);

//    TreeWidget *m_tree = new TreeWidget(m_splitter, m_root);

    QTreeView *tv = new QTreeView(m_splitter);
    m_treemodel = new TreeModel(db);
    tv->setModel(m_treemodel);
    tv->header()->hide();

    m_browse = new BrowseWidget(m_splitter, registry->IndexForDB(db));

    m_splitter->setStretchFactor(0,0);
    m_splitter->setStretchFactor(1,1);

    connect(tv, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnTreeSelectionChanged(const QModelIndex&)));

//    connect(m_tree, SIGNAL(selectionChanged(Q3ListViewItem*)),
//	    this, SLOT(OnTreeSelectionChanged(Q3ListViewItem*)));

    m_browse->SetNode(m_treemodel->NodeForIndex(QModelIndex()));

    resize(500,600);

    show();
}

ExplorerWindow::~ExplorerWindow()
{
    delete m_treemodel;
}

void ExplorerWindow::OnTreeSelectionChanged(const QModelIndex& qmi)
{
    mediatree::NodePtr node = m_treemodel->NodeForIndex(qmi);
    if (node)
	m_browse->SetNode(node);
}

void ExplorerWindow::OnTreeSelectionChanged(Q3ListViewItem *lvi)
{
    if (lvi)
    {
	NodeItem *ni = (NodeItem*)lvi;

	m_browse->SetNode(ni->GetNode());
    }
}

};
