#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include "settings.h"
#include <qdialog.h>
#include <list>

class SettingsEntry;
class Q3GroupBox;

class SettingsWindow: public QDialog
{
    Q_OBJECT

    Settings *m_settings;

    typedef std::list<SettingsEntry*> list_t;
    list_t m_entries;

    Q3GroupBox *m_gb;

public:
    explicit SettingsWindow(Settings*);
    ~SettingsWindow();

public slots:
    void accept();
};

#endif
