#ifndef CLOUD_STYLE_H
#define CLOUD_STYLE_H 1

#include <QPlastiqueStyle>

namespace cloud {

class Style: public QPlastiqueStyle
{
    Q_OBJECT

    QColor m_fg;
    QColor m_bg;

public:
    Style(QColor fg, QColor bg) : m_fg(fg), m_bg(bg) {}

    QColor ShadeColour(unsigned int pct);

    void SetColours(QColor fg, QColor bg);

    // Being a QStyle
    void polish(QPalette&);
    void polish(QWidget *w) { QPlastiqueStyle::polish(w); }
    void polish(QApplication *a) { QPlastiqueStyle::polish(a); }
};

QPixmap ShadeImage(QColor fg, QColor bg, const char *const *xpm);

} // namespace cloud

#endif
