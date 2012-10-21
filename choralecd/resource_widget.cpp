/* resource_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "resource_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>

namespace choraleqt {

ResourceWidget::ResourceWidget(QWidget *parent, const std::string& label,
			       QPixmap pm,
			       const char *topbutton, const char *bottombutton,
			       const std::string& tooltip)
    : QFrame(parent),
      m_label(label)
{
    setFrameStyle( Panel | Sunken );
    setLineWidth(2);
    setMidLineWidth(0);

    QHBoxLayout *hl = new QHBoxLayout(this);

    QVBoxLayout *left = new QVBoxLayout();
    hl->addStretch(10);
    hl->addLayout(left);
    hl->addStretch(10);

    left->addStretch(1);

    QLabel *cdlabel = new QLabel(this);
    cdlabel->setPixmap(pm);
    left->addWidget(cdlabel, 1, Qt::AlignCenter);
    if (!tooltip.empty())
	cdlabel->setToolTip(QString::fromUtf8(tooltip.c_str()));

    QLabel *cdlabel2 = new QLabel(this);
    cdlabel2->setText(QString::fromUtf8(label.c_str()));
    left->addWidget(cdlabel2, 1, Qt::AlignCenter);

    left->addStretch(1);
    
    QVBoxLayout *right = new QVBoxLayout();
    hl->addLayout(right);

    right->addStretch(1);

    m_top = new QPushButton(this);
    m_top->setText(topbutton);
    m_top->setAutoDefault(false);
    right->addWidget(m_top, 1, Qt::AlignCenter);

    connect(m_top, SIGNAL(clicked()), this, SLOT(OnTopButton()));
    
    m_bottom = new QPushButton(this);
    m_bottom->setText(bottombutton);
    m_bottom->setAutoDefault(false);
    right->addWidget(m_bottom, 1, Qt::AlignCenter);

    connect(m_bottom, SIGNAL(clicked()), this, SLOT(OnBottomButton()));

    right->addStretch(1);

    QPalette palette;
    palette.setColor(backgroundRole(), Qt::white);
    setPalette(palette);
    setAutoFillBackground(true);

    show();
}

void ResourceWidget::EnableTop(bool whether)
{
    m_top->setEnabled(whether);
}

void ResourceWidget::EnableBottom(bool whether)
{
    m_bottom->setEnabled(whether);
}

} // namespace choraleqt
