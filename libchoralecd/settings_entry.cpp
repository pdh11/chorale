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
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <stdio.h>
#include <QTableWidget>
#include <QHeaderView>
#include <QBoxLayout>
#include <QFileDialog>

namespace choraleqt {

SettingsEntryText::SettingsEntryText(QBoxLayout *parent, Settings *settings,
				     const char *label, 
				     setter_fn setter, getter_fn getter)
    : m_settings(settings),
      m_setter(setter),
      m_getter(getter),
      m_line(NULL)
{
    parent->addWidget(new QLabel(label));

    QHBoxLayout *hb = new QHBoxLayout;
    m_line = new QLineEdit;
    hb->addWidget(m_line);

    QPushButton *br = new QPushButton;
    br->setText("Browse...");
    hb->addWidget(br);
    connect(br, SIGNAL(clicked()), this, SLOT(OnBrowse()));

    parent->addLayout(hb);
}

void SettingsEntryText::OnShow()
{
    m_line->setText((m_settings->*m_getter)().c_str());
}

void SettingsEntryText::OnOK()
{
    (m_settings->*m_setter)(m_line->text().toUtf8().data());
}

void SettingsEntryText::OnBrowse()
{
    QString qs = QFileDialog::getExistingDirectory(m_line);
    m_line->setText(qs);
}

SettingsEntryBool::SettingsEntryBool(QBoxLayout *parent, Settings *settings,
				     const char *label, 
				     setter_fn setter, getter_fn getter)
    : m_settings(settings),
      m_setter(setter),
      m_getter(getter),
      m_cb(NULL)
{
    m_cb = new QCheckBox(label);
    parent->addWidget(m_cb);
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


SettingsEntryEndpoint::SettingsEntryEndpoint(QBoxLayout *parent, 
					     Settings *settings,
					     setter_fn setter, 
					     getter_fn getter,
					     setter2_fn portsetter, 
					     getter2_fn portgetter)
    : m_settings(settings),
      m_setter(setter),
      m_getter(getter),
      m_setter2(portsetter),
      m_getter2(portgetter),
      m_line(NULL),
      m_portline(NULL)
{
    QHBoxLayout *hb = new QHBoxLayout;
    m_line = new QLineEdit;
    hb->addWidget(m_line, 4);
    hb->addWidget(new QLabel(":"));
    m_portline = new QLineEdit;
    hb->addWidget(m_portline, 1);
    parent->addLayout(hb);
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
    (m_settings->*m_setter2)((unsigned short)strtoul(m_portline->text().toUtf8(), NULL, 10));
}


        /* Map */


SettingsEntryMap::SettingsEntryMap(QBoxLayout *parent,
				   Settings*,
				   const char *label,
				   setter_fn,
				   getter_fn)
    : //m_settings(settings),
      //m_setter(setter),
      //m_getter(getter),
      m_table(NULL)
{
    parent->addWidget(new QLabel(label));
    m_table = new QTableWidget;
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
    parent->addWidget(m_table);
}

void SettingsEntryMap::OnShow()
{
}

void SettingsEntryMap::OnOK()
{
}

} // namespace choraleqt
