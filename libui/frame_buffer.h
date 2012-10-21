#ifndef LIBUI_FRAME_BUFFER_H
#define LIBUI_FRAME_BUFFER_H 1

#include "config.h"

#if HAVE_CAIRO

#include <cairo/cairo.h>

namespace ui {

class FrameBuffer
{
    cairo_t *m_cairo;
    cairo_surface_t *m_surface;

protected:
    FrameBuffer() {}
    void SetContext(cairo_t *c) { m_cairo = c; }
    void SetSurface(cairo_surface_t *s) { m_surface = s; }

public:
    virtual ~FrameBuffer() = 0;

    cairo_t *GetContext() { return m_cairo; }
};

} // namespace ui

#endif // HAVE_CAIRO

#endif
