#ifndef TAG_EDITOR_WIDGET_H
#define TAG_EDITOR_WIDGET_H

#include <QTableWidget>
#include "libmediatree/node.h"
#include <vector>

namespace choraleqt {

class TagEditorWidget: public QTableWidget
{
    Q_OBJECT

    unsigned int m_flags;
    mediatree::NodePtr m_parent;
    std::vector<mediatree::NodePtr> m_items;

    // Being a QAbstractItemView
    void commitData(QWidget*);

public:
    enum {
	SHOW_PATH = 0x1
    };

    TagEditorWidget(QWidget *parent, unsigned int flags);

    void SetNode(mediatree::NodePtr);
};

} // namespace choraleqt

#endif
