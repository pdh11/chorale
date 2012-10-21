/* choralecd/resource_widget.h */
#ifndef RESOURCE_WIDGET_H
#define RESOURCE_WIDGET_H

#include <qframe.h>

class QWidget;
class QPixmap;
class QPushButton;

namespace choraleqt {

/** Abstract base class for all resource widgets in the MainWindow.
 */
class ResourceWidget: public QFrame
{
    Q_OBJECT
    
    std::string m_label;
    QPushButton *m_top;
    QPushButton *m_bottom;
    
public:
    ResourceWidget(QWidget *parent, const std::string& label, QPixmap,
		   const char *topbutton, const char *bottombutton);
    void EnableTop(bool enabled);
    void EnableBottom(bool enabled);

public slots:
    virtual void OnTopButton() = 0;
    virtual void OnBottomButton() = 0;
};

} // namespace choraleqt

#endif
