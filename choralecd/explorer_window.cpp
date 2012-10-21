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
#include <qsplitter.h>
#include <qlayout.h>

namespace choraleqt {

ExplorerWindow::ExplorerWindow(mediatree::NodePtr root,
			       mediadb::Database *db,
			       mediadb::Registry *registry)
    : QDialog(NULL, NULL),
      m_root(root),
      m_db(db),
      m_registry(registry)
{
    QVBoxLayout *vlayout = new QVBoxLayout(this, 0, 1);

    setAttribute(Qt::WA_DeleteOnClose);

    m_splitter = new QSplitter(this);
    vlayout->addWidget(m_splitter);
    m_tree = new TreeWidget(m_splitter, m_root);
//    m_table = new TagTable(7,7,m_splitter);
    m_browse = new BrowseWidget(m_splitter, registry->IndexForDB(db));

    connect(m_tree, SIGNAL(selectionChanged(Q3ListViewItem*)),
	    this, SLOT(OnTreeSelectionChanged(Q3ListViewItem*)));

    m_browse->SetNode(root);

    resize(500,600);

    show();
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
