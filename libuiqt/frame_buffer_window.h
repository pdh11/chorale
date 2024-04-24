#ifndef LIBUI_QT_FRAME_BUFFER_WINDOW_H
#define LIBUI_QT_FRAME_BUFFER_WINDOW_H 1

#if 0

#include <QWidget>

namespace ui {
namespace qt {

class FrameBuffer;

class FrameBufferWindow: public QWidget
{
    Q_OBJECT

    unsigned int m_width;
    unsigned int m_height;
    FrameBuffer *m_frame_buffer;
    QImage m_image;

public:
    FrameBufferWindow(unsigned int width, unsigned int height);
    ~FrameBufferWindow();

    FrameBuffer *GetBuffer() { return m_frame_buffer; }

    // Being a QWidget
    void paintEvent(QPaintEvent*);
};

} // namespace ui::qt
} // namespace ui

#endif

#endif
