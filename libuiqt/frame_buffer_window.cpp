#include "frame_buffer_window.h"
#include "frame_buffer.h"
#include <QPainter>

#if HAVE_CAIRO

namespace ui {
namespace qt {

FrameBufferWindow::FrameBufferWindow(unsigned int cx, unsigned int cy)
    : QWidget(),
      m_width(cx),
      m_height(cy),
      m_frame_buffer(new FrameBuffer(cx,cy)),
      m_image((uchar*)m_frame_buffer->GetData(), cx, cy, QImage::Format_RGB32)
{
    setMaximumSize(cx, cy);
    setMinimumSize(cx, cy);
}

FrameBufferWindow::~FrameBufferWindow()
{
    delete m_frame_buffer;
}

void FrameBufferWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawImage(0,0,m_image);
}

} // namespace ui::qt
} // namespace ui

#endif // HAVE_CAIRO
