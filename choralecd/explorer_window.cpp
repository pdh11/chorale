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
#include "browse_widget.h"
#include "libmediadb/registry.h"
#include "tree_model.h"
#include <qsplitter.h>
#include <qlayout.h>
#include <QTreeView>
#include <QHeaderView>
#include "libmediadb/db.h"
#include "libutil/trace.h"

namespace choraleqt {

ExplorerWindow::ExplorerWindow(mediadb::Database *db,
			       mediadb::Registry *registry)
    : QMainWindow(),
      m_db(db),
      m_registry(registry),
      m_splitter(NULL),
      m_browse(NULL),
      m_treemodel(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Exploring");

    m_splitter = new QSplitter(this);
    setCentralWidget(m_splitter);

    QTreeView *tv = new QTreeView(m_splitter);
    m_treemodel = new TreeModel(db);
    tv->setModel(m_treemodel);
    tv->header()->hide();

    m_browse = new BrowseWidget(m_splitter, registry->GetIndex(db));

    m_splitter->setStretchFactor(0,0);
    m_splitter->setStretchFactor(1,1);

    connect(tv, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnTreeSelectionChanged(const QModelIndex&)));

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
    TRACE << "OTSC: " << qmi.row() << "\n";
    mediatree::NodePtr node = m_treemodel->NodeForIndex(qmi);
    if (node)
	m_browse->SetNode(node);
    else
	TRACE << "No node\n";
}

} // namespace choraleqt
