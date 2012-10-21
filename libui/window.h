#ifndef LIBUI_WINDOW_H
#define LIBUI_WINDOW_H 1

namespace ui {

class FrameBuffer;

class Window
{
public:
    Window();
    ~Window();

    void Render(FrameBuffer*);
};

} // namespace ui

#endif
