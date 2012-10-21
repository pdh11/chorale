#include <QApplication>
#include "libuiqt/frame_buffer_window.h"
#include "libuiqt/frame_buffer.h"
#include "libui/window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ui::qt::FrameBufferWindow fbw(480,320);
    fbw.show();
    ui::Window w;
    w.Render(fbw.GetBuffer());
    return app.exec();
}
