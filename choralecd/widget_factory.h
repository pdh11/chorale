#ifndef CHORALECD_WIDGET_FACTORY_H
#define CHORALECD_WIDGET_FACTORY_H 1

class QWidget;

namespace choraleqt {

class WidgetFactory
{
public:
    virtual ~WidgetFactory() {}

    virtual void CreateWidgets(QWidget *parent) = 0;
};

}; // namespace choraleqt

#endif
