#ifndef SETTINGS_ENTRY_H
#define SETTINGS_ENTRY_H

#include <q3vbox.h>
#include <map>

class Settings;
class QLineEdit;
class QCheckBox;
class QTableWidget;

class SettingsEntry: public Q3VBox
{
    Q_OBJECT

public:
    SettingsEntry(QWidget *parent) : Q3VBox(parent) {}

    virtual void OnShow() = 0;
    virtual void OnOK() = 0;
};

/** A string config item */
class SettingsEntryText: public SettingsEntry
{
public:
    typedef void (Settings::*setter_fn)(const std::string&);
    typedef std::string (Settings::*getter_fn)() const;

private:
    Settings *m_settings;
    setter_fn m_setter;
    getter_fn m_getter;
    QLineEdit *m_line;

public:
    SettingsEntryText(QWidget *parent, Settings *settings,
		      const char *label, setter_fn setter, getter_fn getter);
    void OnShow();
    void OnOK();
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
    SettingsEntryBool(QWidget *parent, Settings *settings,
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
    SettingsEntryEndpoint(QWidget *parent, Settings *settings,
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
    SettingsEntryMap(QWidget *parent, Settings *settings, const char *label,
		     setter_fn setter, getter_fn getter);
    void OnShow();
    void OnOK();
};

#endif
