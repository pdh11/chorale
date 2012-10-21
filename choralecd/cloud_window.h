#ifndef CLOUD_WINDOW_H
#define CLOUD_WINDOW_H 1

#include <QWidget>
#include <QColor>
#include <QPixmap>
#include <QSizeGrip>
#include <string>
#include <vector>
#include "libutil/bind.h"

namespace cloud {

class InvisibleSizeGrip: public QSizeGrip
{
    Q_OBJECT

public:
    InvisibleSizeGrip(QWidget *parent_widget) : QSizeGrip(parent_widget) {}

    void paintEvent(QPaintEvent*) {}
};

struct MenuEntry
{
    QPixmap pixmap;
    bool small;
    std::string text;
    util::Callback onselect;
};

class Window: public QWidget
{
    Q_OBJECT

    QColor m_fg;
    QColor m_bg;
    QPixmap m_network;
    QPixmap m_cd;
    QPixmap m_resize;
    QPixmap m_icon16;
    QPixmap m_close;
    QPixmap m_minimise;
    unsigned int m_x, m_y;
    InvisibleSizeGrip m_size_grip;
    std::vector<MenuEntry> m_menu;
    unsigned int m_selected;
    QWidget *m_widget;
    enum {
	NONE,
	MINIMISE,
	CLOSE,
	DRAG,

	MENUITEM ///< Must be last one
    };
    int m_mousing;
    QPoint m_drag_position;

    int GetMousing(unsigned int x, unsigned int y);

public:
   Window(QColor fg, QColor bg);

    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

    QSize sizeHint() const { return QSize(600,400); }

    unsigned int AppendMenuEntry(const MenuEntry&);
    void DeleteMenuEntry(unsigned int which);
    
    void SetMainWidget(QWidget*);

    QColor Foreground() { return m_fg; }
    QColor Background() { return m_bg; }
};

} // namespace cloud

#endif
