/* settings_entry.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "settings_entry.h"
#include "settings_window.h"
#include <qpushbutton.h>
#include <q3hbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <QTableWidget>
#include <QHeaderView>

SettingsEntryText::SettingsEntryText(QWidget *parent, Settings *settings,
				     const char *label, 
				     setter_fn setter, getter_fn getter)
    : SettingsEntry(parent),
      m_settings(settings),
      m_setter(setter),
      m_getter(getter)
{
    setMargin(6);
    QLabel *mr = new QLabel(this);
    mr->setText(label);
    Q3HBox *hb = new Q3HBox(this);
    m_line = new QLineEdit(hb);
    QPushButton *br = new QPushButton(hb);
    br->setText("Browse...");
}

void SettingsEntryText::OnShow()
{
    m_line->setText((m_settings->*m_getter)().c_str());
}

void SettingsEntryText::OnOK()
{
    (m_settings->*m_setter)(m_line->text().toUtf8().data());
}

SettingsEntryBool::SettingsEntryBool(QWidget *parent, Settings *settings,
				     const char *label, 
				     setter_fn setter, getter_fn getter)
    : SettingsEntry(parent),
      m_settings(settings),
      m_setter(setter),
      m_getter(getter)
{
    setMargin(6);
    m_cb = new QCheckBox(this);
    m_cb->setText(label);
}

void SettingsEntryBool::OnShow()
{
    m_cb->setChecked((m_settings->*m_getter)());
}

void SettingsEntryBool::OnOK()
{
    (m_settings->*m_setter)(m_cb->isChecked());
}


        /* Endpoint */


SettingsEntryEndpoint::SettingsEntryEndpoint(QWidget *parent, 
					     Settings *settings,
					     setter_fn setter, 
					     getter_fn getter,
					     setter2_fn portsetter, 
					     getter2_fn portgetter)
    : SettingsEntry(parent),
      m_settings(settings),
      m_setter(setter),
      m_getter(getter),
      m_setter2(portsetter),
      m_getter2(portgetter)
{
    setMargin(6);
    Q3HBox *hb = new Q3HBox(this);
    m_line = new QLineEdit(hb);
    hb->setStretchFactor(m_line, 4);
    QLabel *colon = new QLabel(hb);
    colon->setText(":");
    m_portline = new QLineEdit(hb);
    hb->setStretchFactor(m_portline, 1);
}

void SettingsEntryEndpoint::OnShow()
{
    m_line->setText((m_settings->*m_getter)().c_str());
    char port[80];
    sprintf(port, "%u", (m_settings->*m_getter2)());
    m_portline->setText(port);
}

void SettingsEntryEndpoint::OnOK()
{
    (m_settings->*m_setter)(m_line->text().toUtf8().data());
    (m_settings->*m_setter2)(atoi(m_portline->text()));
}


        /* Map */


SettingsEntryMap::SettingsEntryMap(QWidget *parent, 
				   Settings *settings,
				   const char *label,
				   setter_fn setter, 
				   getter_fn getter)
    : SettingsEntry(parent),
      m_settings(settings),
      m_setter(setter),
      m_getter(getter)
{
    setMargin(6);
    QLabel *mr = new QLabel(this);
    mr->setText(label);
    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setRowCount(2);
    m_table->setItem(0,0, new QTableWidgetItem("/dev/pcmC0D0p"));
    m_table->setItem(0,1, new QTableWidgetItem("Analogue"));
    m_table->setItem(1,0, new QTableWidgetItem("/dev/pcmC0D1p"));
    m_table->setItem(1,1, new QTableWidgetItem("Digital"));
    m_table->horizontalHeader()->hide();
    m_table->verticalHeader()->hide();
    m_table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_table->setEnabled(false);
}

void SettingsEntryMap::OnShow()
{
}

void SettingsEntryMap::OnOK()
{
}
