#ifndef LIBUIQT_FRAME_BUFFER_H
#define LIBUIQT_FRAME_BUFFER_H 1

#include "libui/frame_buffer.h"
#include <stdint.h>

#if HAVE_CAIRO

namespace ui {
namespace qt {

class FrameBuffer: public ui::FrameBuffer
{
    uint32_t *m_data;

public:
    FrameBuffer(unsigned int width, unsigned int height);
    ~FrameBuffer();

    unsigned int *GetData() { return m_data; }
};

} // namespace ui::qt
} // namespace ui

#endif // HAVE_CAIRO

#endif
