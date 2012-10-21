/* settings_window.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "settings_window.h"
#include "settings_entry.h"
#include "libutil/trace.h"
#include <qlayout.h>
#include <QScrollArea>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <QScrollBar>
#include <QGroupBox>

namespace choraleqt {

SettingsWindow::SettingsWindow(Settings *settings)
    : QDialog(),
      m_settings(settings),
      m_gb(NULL)
{
    setCaption("Settings for choralecd");

    QVBoxLayout *vert = new QVBoxLayout(this, 0, 6);

    QScrollArea *view = new QScrollArea(this);

    QWidget *scrolled = new QWidget;

    QVBoxLayout *box = new QVBoxLayout;

    box->setSizeConstraint(QLayout::SetMinimumSize);
    
    m_entries.push_back(new SettingsEntryText(box, m_settings, "MP3 folder",
					      &Settings::SetMP3Root,
					      &Settings::GetMP3Root));
    
    m_entries.push_back(new SettingsEntryText(box, m_settings, "FLAC folder (leave blank for none)",
					      &Settings::SetFlacRoot,
					      &Settings::GetFlacRoot));
    
    m_entries.push_back(new SettingsEntryBool(box, m_settings,
					      "Use CDDB",
					      &Settings::SetUseCDDB,
					      &Settings::GetUseCDDB));

    m_gb = new QGroupBox("Use HTTP proxy (for CDDB)");
    m_gb->setCheckable(true);
    QVBoxLayout *innerbox = new QVBoxLayout;
    m_gb->setChecked(m_settings->GetUseHttpProxy());
    box->addWidget(m_gb);

    m_entries.push_back(new SettingsEntryEndpoint(innerbox, m_settings, 
						  &Settings::SetHttpProxyHost,
						  &Settings::GetHttpProxyHost, 
						  &Settings::SetHttpProxyPort,
						  &Settings::GetHttpProxyPort));
    m_gb->setLayout(innerbox);

#if 0
    m_entries.push_back(new SettingsEntryMap(box, m_settings,
					     "Aliases for local resources",
					     NULL, NULL));

    m_entries.push_back(new SettingsEntryMap(box, m_settings,
					     "Aliases for network resources",
					     NULL, NULL));
#endif

    vert->addWidget(view);
    
    QHBoxLayout *horz = new QHBoxLayout(vert, 12);
    horz->addStretch();

    QPushButton *cancel = new QPushButton(this);
    cancel->setText("Cancel");
    horz->addWidget(cancel);
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    ok->setDefault(true);
    horz->addWidget(ok);    
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));

    horz->addSpacing(20); // clear of the resize handle

    vert->addSpacing(6);

    scrolled->setLayout(box);
    view->setWidget(scrolled);
    view->setWidgetResizable(true);
    setSizeGripEnabled(true);

    for (list_t::iterator i = m_entries.begin(); i != m_entries.end(); ++i)
	(*i)->OnShow();

    show();
    setMinimumWidth(box->sizeHint().width()
		    + view->verticalScrollBar()->sizeHint().width() + 6);
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::accept()
{
    for (list_t::iterator i = m_entries.begin(); i != m_entries.end(); ++i)
	(*i)->OnOK();

    m_settings->SetUseHttpProxy(m_gb->isChecked());

    QDialog::accept();
}

} // namespace choraleqt
