#include "cloud_database_widget.h"
#include "cloud_window.h"
#include "cloud_style.h"
#include "browse_widget.h"
#include "libmediadb/registry.h"
#include "libmediadb/db.h"
#include <QTreeView>
#include <QHeaderView>

#include "imagery/file.xpm"
#include "imagery/dir.xpm"

namespace cloud {

DatabaseWidget::DatabaseWidget(Window *parent, mediadb::Registry *registry,
			       mediadb::Database *db)
    : QSplitter(parent),
      m_model(db),
      m_tree(new QTreeView(this)),
      m_dir_pixmap(ShadeImage(parent->Foreground(), parent->Background(),
			      dir_xpm)),
      m_file_pixmap(ShadeImage(parent->Foreground(), parent->Background(),
			      file_xpm)),
      m_browse(new choraleqt::BrowseWidget(this, registry->IndexForDB(db),
					   &m_dir_pixmap, &m_file_pixmap))
{
    m_tree->setModel(&m_model);
    m_tree->header()->hide();

    setStretchFactor(0,0);
    setStretchFactor(1,1);

    connect(m_tree, SIGNAL(clicked(const QModelIndex&)),
	    this, SLOT(OnTreeSelectionChanged(const QModelIndex&)));

    m_browse->SetNode(m_model.NodeForIndex(QModelIndex()));
}

DatabaseWidget::~DatabaseWidget()
{
}

void DatabaseWidget::OnTreeSelectionChanged(const QModelIndex& qmi)
{
    mediatree::NodePtr node = m_model.NodeForIndex(qmi);
    if (node)
	m_browse->SetNode(node);
}

} // namespace cloud
