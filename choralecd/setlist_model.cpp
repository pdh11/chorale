/* setlist_model.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the LGPL.
 */
#include "setlist_model.h"
#include "libutil/trace.h"
#include "browse_widget.h"
#include "liboutput/queue.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libmediadb/registry.h"
#include <boost/format.hpp>
#include <QMimeData>

namespace choraleqt {

SetlistModel::SetlistModel(const QFont& base_font,
			   output::Queue *queue, mediadb::Registry *registry)
    : m_bold_font(base_font),
      m_queue(queue),
      m_registry(registry),
      m_current_index(0),
      m_timecode_sec(0)
{
    m_bold_font.setBold(true);
    setSupportedDragActions(Qt::MoveAction);
}

SetlistModel::~SetlistModel()
{
}

int SetlistModel::rowCount(const QModelIndex&) const
{
    return m_queue->Count();
}

int SetlistModel::columnCount(const QModelIndex&) const
{
    return 2;
}

static std::string timecode(unsigned int sec)
{
    if (sec >= 3600)
	return (boost::format("%u:%02u:%02u") % (sec/3600)
		% ((sec/60) % 60) % (sec % 60)).str();
    return (boost::format("%u:%02u") % (sec/60) % (sec%60)).str();
}

QVariant SetlistModel::data(const QModelIndex& i, int role) const
{
    if (role == Qt::TextAlignmentRole && i.column() == 1)
	return QVariant(int(Qt::AlignRight|Qt::AlignVCenter));

    if (role == Qt::FontRole && i.row() == (int)m_current_index)
	return QVariant(m_bold_font);

    if (i.row() < 0 || i.row() >= (int)m_names.size() || role != Qt::DisplayRole)
	return QVariant();

    if (i.column() == 1)
    {
	std::string s;
	unsigned int duration_sec = m_durations_sec[i.row()];
	if (i.row() == (int)m_current_index)
	{
	    s = timecode(m_timecode_sec);
	    if (duration_sec)
		s += "/" + timecode(duration_sec);
	}
	else
	{
	    if (duration_sec)
		s = timecode(duration_sec);
	    else
		s = "-:--";
	}
	return QVariant(QString(s.c_str()));
    }

    return QVariant(m_names[i.row()]);
}

QStringList SetlistModel::mimeTypes() const
{
    QStringList qsl;
    qsl << "application/x-chorale-ids";
    return qsl;
}

QMimeData *SetlistModel::mimeData(const QModelIndexList& items) const
{
    std::vector<IDPair> idpv;
    idpv.reserve(items.size());

    TRACE << "Generating mimeData for " << items.size() << " items\n";

    for (unsigned int i=0; i<(unsigned)items.size(); ++i)
    {
	const QModelIndex& qmi = items[i];
	const output::Queue::Entry& op = m_queue->EntryAt(qmi.row());

	TRACE << "Yup item for row " << qmi.row() << "\n";
	if (qmi.column() != 0)
	    continue;

	IDPair idp;
	idp.dbid = m_registry->GetIndex(op.db);
	idp.fid = op.id;
	idpv.push_back(idp);
    }
    
    QByteArray a;
    a.resize((int)(idpv.size() * sizeof(IDPair)));
    memcpy(a.data(), &idpv[0], idpv.size() * sizeof(IDPair));

    QMimeData *md = new QMimeData;
    md->setData("application/x-chorale-ids", a);
    return md;
}

Qt::DropActions SetlistModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions SetlistModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags SetlistModel::flags(const QModelIndex& ix) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled
	| Qt::ItemIsDropEnabled;
    
    if (ix.isValid())
	flags |= Qt::ItemIsDragEnabled;
    
    return flags;
}

bool SetlistModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
				int row, int, const QModelIndex& parent)
{
    if (action == Qt::IgnoreAction)
	return true;

    if (!data->hasFormat("application/x-chorale-ids"))
	return false;

    int beginRow;
    if (row != -1)
	beginRow = row;
    else if (parent.isValid())
	beginRow = parent.row();
    else
	beginRow = rowCount(QModelIndex());

    QByteArray qba = data->data("application/x-chorale-ids");
    std::vector<IDPair> idps;
    size_t count = qba.size() / sizeof(IDPair);
    idps.resize(count);
    memcpy(&idps[0], qba.data(), count*sizeof(IDPair));

    TRACE << "Got mime data for " << count << " items action=" 
	  << (int)action << "\n";

    std::vector<output::Queue::Entry> ops;
    ops.reserve(idps.size());

    for (size_t i=0; i<count; ++i)
    {
	const IDPair& idp = idps[i];

	output::Queue::Entry op;
	op.id = idp.fid;
	op.db = m_registry->Get(idp.dbid);
	if (op.db)
	    ops.push_back(op);
    }

    if (ops.empty())
	return false;

    TRACE << "Which became " << ops.size() << " real items\n";

    beginInsertRows(QModelIndex(),
		    beginRow, beginRow + (int)ops.size()-1);

    for (size_t i=0; i<ops.size(); ++i)
    {
	m_queue->EntryInsert(ops[i].db, ops[i].id, beginRow);

	db::QueryPtr qp = ops[i].db->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, ops[i].id));
	db::RecordsetPtr rs = qp->Execute();
	m_names.insert(m_names.begin() + beginRow,
		       QString::fromUtf8((rs->GetString(mediadb::TITLE)
					  + " ("
					  + rs->GetString(mediadb::ARTIST)
					  + ")").c_str()));
	unsigned int duration_sec = rs->GetInteger(mediadb::DURATIONMS);
	duration_sec += 999;
	duration_sec /= 1000;
	m_durations_sec.insert(m_durations_sec.begin() + beginRow, 
			       duration_sec);

	++beginRow;
    }

    endInsertRows();

    return true;
}

bool SetlistModel::removeRows(int row, int count, const QModelIndex& parent)
{
    TRACE << "Removing rows " << row << ".." << (row+count) << "\n";
    beginRemoveRows(parent, row, row+count-1);
    m_queue->EntryRemove(row, row+count);
    m_names.erase(m_names.begin()+row,
		  m_names.begin()+row+count);
    m_durations_sec.erase(m_durations_sec.begin()+row,
			  m_durations_sec.begin()+row+count);
    endRemoveRows();
    return true;
}

void SetlistModel::SetIndexAndTimecode(unsigned int ix, unsigned int tc_sec)
{
    // ix is a queue index, convert to entries index
    ix = m_queue->QueueAt(ix);

    if (ix != m_current_index)
    {
	unsigned old_ix = m_current_index;
	m_current_index = ix;
	m_timecode_sec = tc_sec;
	emit dataChanged(index(old_ix,0), index(old_ix,1));
	emit dataChanged(index(ix,0), index(ix,1));
    }
    else if (m_timecode_sec != tc_sec)
    {
	m_timecode_sec = tc_sec;
	emit dataChanged(index(m_current_index,1), index(m_current_index,1));
    }
}

} // namespace choraleqt
