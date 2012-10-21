#include "frame_buffer.h"

#if HAVE_CAIRO

#include <cairo.h>
#include <string.h>

namespace ui {
namespace qt {

FrameBuffer::FrameBuffer(unsigned int cx, unsigned int cy)
{
    m_data = new uint32_t[cx*cy];
    
    memset(m_data, 0, cx*cy*sizeof(uint32_t));

    cairo_surface_t *surface =
	cairo_image_surface_create_for_data((unsigned char*)m_data,
					    CAIRO_FORMAT_ARGB32,
					    cx, cy, cx*4);
    SetSurface(surface);
    cairo_t *cr = cairo_create(surface);
    SetContext(cr);
}

FrameBuffer::~FrameBuffer()
{
    delete[] m_data;
}

} // namespace ui::qt
} // namespace ui

#endif // HAVE_CAIRO
