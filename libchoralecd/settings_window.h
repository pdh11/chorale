#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <qdialog.h>
#include <list>

class QGroupBox;

namespace choraleqt {

class Settings;
class SettingsEntry;

class SettingsWindow: public QDialog
{
    Q_OBJECT

    Settings *m_settings;

    typedef std::list<SettingsEntry*> list_t;
    list_t m_entries;

    QGroupBox *m_gb;

public:
    explicit SettingsWindow(Settings*);
    ~SettingsWindow();

public slots:
    void accept();
};

} // namespace choraleqt

#endif
