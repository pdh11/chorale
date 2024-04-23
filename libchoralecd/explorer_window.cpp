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
#include "tag_editor_widget.h"
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
			       mediadb::Registry *registry,
			       unsigned int tag_widget_flags)
    : QMainWindow(),
      m_db(db),
      m_registry(registry),
      m_splitter(NULL),
      m_browse(NULL),
      m_treemodel(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString::fromUtf8("Exploring"));

    m_splitter = new QSplitter(this);
    setCentralWidget(m_splitter);

    QTreeView *tv = new QTreeView(m_splitter);
    m_treemodel = new TreeModel(db);
    tv->setModel(m_treemodel);
    tv->header()->hide();

    m_browse = new BrowseWidget(m_splitter, registry->GetIndex(db));

    m_tag_editor = new TagEditorWidget(m_splitter, tag_widget_flags);

    m_splitter->setStretchFactor(0,0);
    m_splitter->setStretchFactor(1,1);
    m_splitter->setStretchFactor(2,1);

    connect(tv, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnTreeSelectionChanged(const QModelIndex&)));

    m_browse->SetNode(m_treemodel->NodeForIndex(QModelIndex()));

    m_tag_editor->hide();

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
    {
	if (m_browse->isHidden())
	    m_tag_editor->SetNode(node);
	else
	    m_browse->SetNode(node);
    }
    else
	TRACE << "No node\n";
}

void ExplorerWindow::SetTagMode()
{
    m_tag_editor->show();
    m_browse->hide();
}

void ExplorerWindow::SetBrowseMode()
{
    m_browse->show();
    m_tag_editor->hide();
}

} // namespace choraleqt
