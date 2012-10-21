#ifndef SETTINGS_ENTRY_H
#define SETTINGS_ENTRY_H

#include <map>
#include "settings.h"
#include <QObject>

class QBoxLayout;
class QLineEdit;
class QCheckBox;
class QTableWidget;

namespace choraleqt {

class SettingsEntry: public QObject
{
public:
    virtual ~SettingsEntry() {}

    virtual void OnShow() = 0;
    virtual void OnOK() = 0;
};

/** A string config item */
class SettingsEntryText: public SettingsEntry
{
    Q_OBJECT

public:
    typedef void (Settings::*setter_fn)(const std::string&);
    typedef std::string (Settings::*getter_fn)() const;

private:
    Settings *m_settings;
    setter_fn m_setter;
    getter_fn m_getter;
    QLineEdit *m_line;

public:
    SettingsEntryText(QBoxLayout *parent, Settings *settings,
		      const char *label, setter_fn setter, getter_fn getter);
    void OnShow();
    void OnOK();

private slots:
    void OnBrowse();
};

/** A bool config item */
class SettingsEntryBool: public SettingsEntry
{
public:
    typedef void (Settings::*setter_fn)(bool);
    typedef bool (Settings::*getter_fn)() const;

private:
    Settings *m_settings;
    setter_fn m_setter;
    getter_fn m_getter;
    QCheckBox *m_cb;

public:
    SettingsEntryBool(QBoxLayout *parent, Settings *settings,
		      const char *label, setter_fn setter, getter_fn getter);
    void OnShow();
    void OnOK();
};

/** A hostname and port */
class SettingsEntryEndpoint: public SettingsEntry
{
public:
    typedef void (Settings::*setter_fn)(const std::string&);
    typedef std::string (Settings::*getter_fn)() const;
    typedef void (Settings::*setter2_fn)(unsigned short);
    typedef unsigned short (Settings::*getter2_fn)() const;

private:
    Settings *m_settings;
    setter_fn m_setter;
    getter_fn m_getter;
    setter2_fn m_setter2;
    getter2_fn m_getter2;
    QLineEdit *m_line;
    QLineEdit *m_portline;

public:
    SettingsEntryEndpoint(QBoxLayout *parent, Settings *settings,
			  setter_fn setter, getter_fn getter,
			  setter2_fn portsetter, getter2_fn portgetter);
    void OnShow();
    void OnOK();
};

/** A map from strings to strings */
class SettingsEntryMap: public SettingsEntry
{
public:
    typedef std::map<std::string, std::string> map_t;
    typedef void (Settings::*setter_fn)(const map_t&);
    typedef map_t (Settings::*getter_fn)() const;

private:
    Settings *m_settings;
    setter_fn m_setter;
    getter_fn m_getter;
    QTableWidget *m_table;

public:
    SettingsEntryMap(QBoxLayout *parent, Settings *settings, const char *label,
		     setter_fn setter, getter_fn getter);
    void OnShow();
    void OnOK();
};

} // namespace choraleqt

#endif
