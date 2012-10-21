#include "cloud_window.h"
#include "cloud_style.h"
#include "libutil/trace.h"
#include <QtGui>

#include "imagery/resize.xpm"
#include "imagery/icon16.xpm"
#include "imagery/close.xpm"
#include "imagery/minimise.xpm"

namespace cloud {

Window::Window(QColor fg, QColor bg)
    : QWidget(NULL, Qt::FramelessWindowHint),
      m_fg(fg),
      m_bg(bg),
      m_resize(ShadeImage(fg,bg,resize_xpm)),
      m_icon16(icon16_xpm),
      m_close(ShadeImage(bg,fg,close_xpm)), // note inverse
      m_minimise(ShadeImage(bg,fg,minimise_xpm)), // note inverse
      m_x(0),
      m_y(0),
      m_size_grip(this),
      m_selected(0),
      m_widget(NULL),
      m_mousing(NONE)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

unsigned int Window::AppendMenuEntry(const MenuEntry& me)
{
    m_menu.push_back(me);
    update();
    resizeEvent(NULL);
    return (unsigned int)m_menu.size()-1;
}

void Window::paintEvent(QPaintEvent*)
{
    unsigned int mainx = m_x - 200;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // This fills in any spare pixels where the mask arc plotter might differ
    // by a few pixels
    painter.fillRect(0,0, 200, m_y, m_fg);
    painter.fillRect(210,20, m_x-220, m_y-30, m_bg);
    painter.setBackground(m_bg);

    painter.drawPixmap(m_x-32, m_y-32, m_resize);

    // Needn't worry about top corners as mask will remove them
    painter.fillRect(200, 0, m_x-200, 20, m_fg);
    painter.fillRect(m_x-10, 0, 10, 40, m_fg);
    painter.fillRect(200, 0, 10, 40, m_fg);
    painter.fillRect(200, m_y-10, 40, 10, m_fg);
    painter.fillRect(200, m_y-40, 10, 40, m_fg);

    QPen pen(m_fg);
    pen.setWidth(10);
    painter.setPen(pen);
    painter.drawRoundedRect(205,15, mainx-10, m_y-20, 20, 20);

    // Draw title bar

    painter.drawPixmap(220, 2, m_icon16);
    painter.drawPixmap(m_x-36, 2, m_close);
    painter.drawPixmap(m_x-56, 2, m_minimise);
    painter.save();
    QFont font = painter.font();
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    painter.setPen(m_bg);
    painter.setBackground(m_fg);
    QFontMetrics fm = painter.fontMetrics();
    painter.drawText(240, 10+fm.height()/2 - fm.descent(), "Cloud");
    painter.restore();

    // Draw menu

    QColor altbase = palette().brush(QPalette::AlternateBase);
    fm = painter.fontMetrics();

    for (unsigned int i=0; i<m_menu.size(); ++i)
    {
	painter.setClipRect(0,0,200,m_y);
	painter.fillRect(9, i*50 + 55, 200, 50,
			 (i == m_selected) ? m_bg : altbase);

	painter.setClipRect(0,0,185,m_y);
	painter.drawRoundedRect(5, i*50+55, 240, 50, 20, 20);
	painter.drawRect(5, i*50+55, 240, 50);

	painter.setClipRect(0,0,200,m_y);

	painter.drawPixmap(12, i*50+62, m_menu[i].pixmap);

	painter.drawText(48, i*50 + 80 + fm.height()/2 - fm.descent(), 
			 QString::fromUtf8(m_menu[i].text.c_str()));
    }

    painter.setClipping(false);

    // Join selected item to main screen
    if (m_selected < m_menu.size())
    {
	painter.fillRect(200, m_selected*50+40, 10, 80, m_bg);
    }

    for (unsigned int i=0; i<m_menu.size(); ++i)
    {
	if (i <= m_selected)
	{
//	    painter.setClipRect(185, i*50+25, 30, 30);
//	    painter.drawRoundedRect(0,0,205,i*50+45, 20,20);
//	    painter.setClipping(false);
	    painter.drawArc(165, i*50 + 15, 40, 40, 270*16, 90*16);
	}
	if (i >= m_selected)
	    painter.drawArc(165, i*50 + 105, 40, 40, 0*16, 90*16);
    }
}

void Window::SetMainWidget(QWidget *widget)
{
    if (m_widget)
	m_widget->hide();
    m_widget = widget;
    if (widget)
    {
	widget->resize(m_x-230, m_y-40);
	widget->move(215, 25);
	setMinimumSize(widget->minimumSize() + QSize(230,40));
	widget->show();
    }
    else
	setMinimumSize(300, (int)m_menu.size()*50 + 100);
}

void Window::resizeEvent(QResizeEvent *re)
{
    if (re)
    {
	m_x = re->size().width();
	m_y = re->size().height();
    }
    unsigned int mainx = m_x - 200;

    QBitmap alpha(m_x, m_y);
    alpha.fill(Qt::color0);

    QPainter painter(&alpha);

    QPen pen(Qt::color1);
    pen.setWidth(10);
    painter.setPen(pen);
    painter.drawRoundedRect(205,5, mainx-10, m_y-10, 20,20);

    for (unsigned int i=0; i<m_menu.size(); ++i)
    {
	painter.drawRoundedRect(5, i*50+55, 240,50, 20,20);
	painter.fillRect(9, i*50+59, 220,42, Qt::color1);
    }

    if (!m_menu.empty())
    {
	painter.drawArc(165, 15,
			40, 40, 270*16, 90*16);
	painter.drawArc(165, (int)m_menu.size()*50 + 55,
			40, 40, 0*16, 90*16);
    }

    painter.fillRect(209,9, mainx-18, m_y-18, Qt::color1);
    painter.fillRect(m_x-20, m_y-20,20,20, Qt::color1);

    if (m_widget)
	m_widget->resize(m_x-230, m_y-40);

    setMask(alpha);

    m_size_grip.resize(20,20);
    m_size_grip.move(m_x-20, m_y-20);
}

int Window::GetMousing(unsigned int x, unsigned int y)
{
    if (x<200 && x>10)
    {
	y -= 60;
	if ((y%50) < 40)
	    return MENUITEM + (y/50);
	return NONE;
    }
    if (y>18 || y<2)
	return NONE;
    if (x > m_x-36 && x < m_x-20)
	return CLOSE;
    if (x > m_x-56 && x < m_x-40)
	return MINIMISE;
    if (x > 240 && x < m_x-60)
	return DRAG;
    return NONE;
}

void Window::mousePressEvent(QMouseEvent *me)
{
    m_mousing = GetMousing(me->x(), me->y());
    if (m_mousing == DRAG)
    {
	m_drag_position = me->globalPos() - geometry().topLeft();
//	TRACE << "Starting drag mdp=" << m_drag_position.x() << ","
//	      << m_drag_position.y() << " pos=" << me->globalPos().x() << ","
//	      << me->globalPos().y() << "\n";
	me->accept();
    }
}

void Window::mouseMoveEvent(QMouseEvent *me)
{
    if (m_mousing == DRAG)
    {
	move(me->globalPos() - m_drag_position);
//	TRACE << "drag to pos=" << me->globalPos().x() << ","
//	      << me->globalPos().y() << "\n";
	me->accept();
    }
}

void Window::mouseReleaseEvent(QMouseEvent *me)
{
    int mousing = GetMousing(me->x(), me->y());
    if (mousing == m_mousing)
    {
	me->accept();
	switch (mousing)
	{
	case MINIMISE:
	    setWindowState(Qt::WindowMinimized);
	    break;
	case CLOSE:
	    close();
	    break;
	default:
	    if (mousing >= MENUITEM)
	    {
		m_selected = mousing - MENUITEM;
		if (m_menu[m_selected].onselect)
		    m_menu[m_selected].onselect();
		update();
	    }
	    break;
	}
    }

    m_mousing = NONE;
}

} // namespace cloud
