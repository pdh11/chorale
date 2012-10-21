#include "cloud_style.h"

namespace cloud {

void Style::polish(QPalette& pal)
{
    pal = QPalette(m_fg);

    pal.setBrush(QPalette::Window, m_bg);
    pal.setBrush(QPalette::WindowText, m_fg);
    pal.setBrush(QPalette::Base, m_bg);
    pal.setBrush(QPalette::AlternateBase, ShadeColour(20));
    pal.setBrush(QPalette::Text, m_fg);
    pal.setBrush(QPalette::Button, m_fg);
    pal.setBrush(QPalette::ButtonText, m_bg);
    pal.setBrush(QPalette::Highlight, ShadeColour(80));
    pal.setBrush(QPalette::HighlightedText, m_bg);
}

QColor Style::ShadeColour(unsigned int pct)
{
    int r = (pct * m_fg.red() + (100-pct) * m_bg.red()) / 100;
    int g = (pct * m_fg.green() + (100-pct) * m_bg.green()) / 100;
    int b = (pct * m_fg.blue() + (100-pct) * m_bg.blue()) / 100;
    return QColor(qRgb(r,g,b));
}

QPixmap ShadeImage(QColor fg, QColor bg, const char *const *xpm)
{
    QImage im(xpm);

    for (int i=0; i<im.numColors(); ++i)
    {
	QRgb c = im.color(i);
	
	int r = (qRed(c) * bg.red() + (255-qRed(c)) * fg.red()) / 255;
	int g = (qRed(c) * bg.green() + (255-qRed(c)) * fg.green()) / 255;
	int b = (qRed(c) * bg.blue() + (255-qRed(c)) * fg.blue()) / 255;
	im.setColor(i,qRgba(r,g,b, qAlpha(c)));
    }

    return QPixmap::fromImage(im);
}

} // namespace cloud
