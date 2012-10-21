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
#include <q3vbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <q3groupbox.h>
#include <QScrollBar>

SettingsWindow::SettingsWindow(Settings *settings)
    : QDialog(),
      m_settings(settings)
{
    setCaption("Settings for choralecd");

    QVBoxLayout *vert = new QVBoxLayout(this, 0, 6);

    QScrollArea *view = new QScrollArea(this);

    Q3VBox *box = new Q3VBox;
    view->setWidget(box);
    view->setWidgetResizable(true);
    box->show();
    view->show();

/*
    QLabel *mr = new QLabel(box);
    mr->setText("MP3 folder");
    QHBox *hb = new QHBox(box);
    QLineEdit *le = new QLineEdit(hb);
    le->setText(settings->GetMP3Root());
    QPushButton *br = new QPushButton(hb);
    br->setText("Browse...");
*/
    
    m_entries.push_back(new SettingsEntryText(box, m_settings, "MP3 folder",
					      &Settings::SetMP3Root,
					      &Settings::GetMP3Root));
    
    m_entries.push_back(new SettingsEntryText(box, m_settings, "FLAC folder",
					      &Settings::SetFlacRoot,
					      &Settings::GetFlacRoot));
    
    m_entries.push_back(new SettingsEntryBool(box, m_settings,
					      "Use CDDB",
					      &Settings::SetUseCDDB,
					      &Settings::GetUseCDDB));

    m_gb = new Q3GroupBox(1, Qt::Horizontal, "Use HTTP proxy (for CDDB)",
			  box);
    m_gb->setCheckable(true);
    m_gb->setInsideMargin(3);
    m_gb->setChecked(m_settings->GetUseHttpProxy());

    m_entries.push_back(new SettingsEntryEndpoint(m_gb, m_settings, 
						  &Settings::SetHttpProxyHost,
						  &Settings::GetHttpProxyHost, 
						  &Settings::SetHttpProxyPort,
						  &Settings::GetHttpProxyPort));

/*
    m_entries.push_back(new SettingsEntryMap(box, m_settings,
					     "Aliases for local resources",
					     NULL, NULL));
*/

    QWidget *blank = new QWidget(box);
    box->setStretchFactor(blank, 100);

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

//    view->setResizePolicy(Q3ScrollView::AutoOneFit);
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
