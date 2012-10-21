#ifndef SETLIST_MODEL_H
#define SETLIST_MODEL_H 1

#include <QFont>
#include <QAbstractTableModel>
#include <vector>

namespace output { class Queue; }
namespace mediadb { class Registry; }

namespace choraleqt {

/** A SetlistModel wraps up an output::Queue as a QAbstractTableModel
 * ready for handing to a QTableView.
 */
class SetlistModel: public QAbstractTableModel
{
    Q_OBJECT

    QFont m_bold_font;
    output::Queue *m_queue;
    mediadb::Registry *m_registry;
    std::vector<QString> m_names;
    std::vector<unsigned int> m_durations_sec;
    unsigned int m_current_index;
    unsigned int m_timecode_sec;

public:
    SetlistModel(const QFont& base_font,
		 output::Queue *queue, mediadb::Registry *registry);
    ~SetlistModel();

    void SetIndexAndTimecode(unsigned int index, unsigned int timecode_sec);
    QString Name(unsigned int ix) const { return m_names[ix]; }

    // Being a QAbstractListModel
    int rowCount(const QModelIndex&) const;
    int columnCount(const QModelIndex&) const;
    QVariant data(const QModelIndex&, int role) const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList&) const;
    Qt::DropActions supportedDragActions() const;
    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex&) const;
    bool dropMimeData(const QMimeData*, Qt::DropAction action,
		      int row, int column, const QModelIndex& parent);
    bool removeRows(int row, int count, const QModelIndex& parent);
};

} // namespace choraleqt

#endif
