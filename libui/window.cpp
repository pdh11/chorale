#include "window.h"
#include "frame_buffer.h"

#if HAVE_CAIRO

#include <cairo.h>
#include <math.h>

namespace ui {

Window::Window()
{
}

Window::~Window()
{
}

void Window::Render(FrameBuffer *fb)
{
    cairo_t *cr = fb->GetContext();

    cairo_set_line_width(cr, 6);
    cairo_set_source_rgb(cr, 255,255,255);

    unsigned int sel = 2;

    for (unsigned int i=1; i < sel; ++i)
    {
	cairo_move_to(cr, 0, i*64);
	cairo_line_to(cr, 48, i*64);
	cairo_arc_negative(cr, 48, i*64-16, 16, 0.5*M_PI, 0.0);
    }
    
    for (unsigned int i=sel+2; i<5; ++i)
    {
	cairo_move_to(cr, 0, i*64);
	cairo_line_to(cr, 64, i*64);
    }

    if (sel > 0)
    {
	cairo_move_to(cr, 64, 0);
	cairo_line_to(cr, 64, sel*64-16);
	cairo_arc(cr, 48, sel*64-16, 16, 0.0, 0.5*M_PI);
	cairo_line_to(cr, 0, sel*64);
    }

    if (sel < 4)
    {
	cairo_move_to(cr, 0, sel*64+64);
	cairo_line_to(cr, 48, sel*64+64);
	cairo_arc(cr, 48, sel*64+80, 16, -0.5*M_PI, 0.0);
	cairo_line_to(cr, 64, 320);
    }

    cairo_stroke(cr);
}

} // namespace ui

#endif // HAVE_CAIRO

#ifdef TEST
int main() { return 0; }
#endif
