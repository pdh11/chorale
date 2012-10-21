/* choralecd/resource_widget.h */
#ifndef RESOURCE_WIDGET_H
#define RESOURCE_WIDGET_H

#include <qframe.h>

class QWidget;
class QPixmap;
class QPushButton;
class QLabel;

namespace choraleqt {

/** Abstract base class for all resource widgets in the MainWindow.
 */
class ResourceWidget: public QFrame
{
    Q_OBJECT
    
    std::string m_label;
    QLabel *m_label_widget;
    QLabel *m_icon_widget;
    QPushButton *m_top;
    QPushButton *m_bottom;
    
public:
    ResourceWidget(QWidget *parent, const std::string& label, QPixmap,
		   const char *topbutton, const char *bottombutton,
		   const std::string& tooltip = std::string());
    void EnableTop(bool enabled);
    void EnableBottom(bool enabled);

    void SetLabel(const std::string&);
    void SetResourcePixmap(QPixmap);

public slots:
    virtual void OnTopButton() = 0;
    virtual void OnBottomButton() = 0;
};

} // namespace choraleqt

#endif
