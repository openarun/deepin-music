/*
 * Copyright (C) 2016 ~ 2018 Wuhan Deepin Technology Co., Ltd.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "importwidget.h"

#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QStandardPaths>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include <DFileDialog>
#include <DLabel>
#include <DPushButton>
#include <DHiDPIHelper>
#include <DPalette>
#include <DGuiApplicationHelper>

#include "musicsettings.h"
#include "databaseservice.h"

#include "ac-desktop-define.h"
DGUI_USE_NAMESPACE

const QString linkTemplate = "<a href='%1'>%2</a>";
static constexpr int AnimationDelay = 200; //ms

ImportWidget::ImportWidget(QWidget *parent) : DFrame(parent)
{
    setObjectName("ImportWidget");

    setAcceptDrops(true);
    setAutoFillBackground(true);

    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);

    m_logo = new QLabel;
    m_logo->setFixedSize(128, 128);
    m_logo->setObjectName("ImportViewLogo");
    m_logo->setPixmap(DHiDPIHelper::loadNxPixmap(":/mpimage/light/import_music.svg"));

    m_importButton = new DPushButton;
    auto importButtonFont = m_importButton->font();
    importButtonFont.setFamily("SourceHanSansSC");
    importButtonFont.setWeight(QFont::Normal);
    importButtonFont.setPixelSize(14);
    m_importButton->setFont(importButtonFont);
    auto pl = m_importButton->palette();
    pl.setColor(DPalette::Dark, QColor("#0098FF"));
    pl.setColor(DPalette::Light, QColor("#25B7FF"));
    pl.setColor(DPalette::ButtonText, QColor("#FFFFFF"));
    QColor sbcolor("#000000");
    sbcolor.setAlphaF(0);
    pl.setColor(DPalette::Shadow, sbcolor);
//    d->importButton->setPalette(pl);
    m_importButton->setObjectName("ImportViewImportButton");
    m_importButton->setFixedSize(302, 36);
    m_importButton->setText(tr("Open Folder"));
    m_importButton->setFocusPolicy(Qt::TabFocus);
    m_importButton->setDefault(true);
    m_importButton->installEventFilter(this);

    m_addMusicButton = new DPushButton;
    m_addMusicButton->setFont(importButtonFont);
//    d->addMusicButton->setPalette(pl);
    m_addMusicButton->setObjectName("ImportViewImportButton");
    m_addMusicButton->setFixedSize(302, 36);
    m_addMusicButton->setText(tr("Add Music"));
    m_addMusicButton->setFocusPolicy(Qt::TabFocus);
    m_addMusicButton->setDefault(true);
    m_addMusicButton->installEventFilter(this);

    m_text = new QLabel;
    m_text->setObjectName("ImportViewText");
    auto textFont = m_text->font();
    textFont.setFamily("SourceHanSansSC");
    textFont.setWeight(QFont::Normal);
    textFont.setPixelSize(12);
    m_text->setFont(textFont);
    m_text->setFixedHeight(18);
    QString linkText = QString(linkTemplate).arg(tr("Scan")).arg(tr("Scan"));
    m_text->setText(tr("%1 music directory or drag music files here").arg(linkText));
    m_text->setFocusPolicy(Qt::TabFocus);
    m_text->installEventFilter(this);

    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(m_logo, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(m_addMusicButton, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(m_importButton, 0, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addWidget(m_text, 0, Qt::AlignCenter);
    layout->addStretch();


    AC_SET_OBJECT_NAME(m_text, AC_importLinkText);
    AC_SET_ACCESSIBLE_NAME(m_text, AC_importLinkText);

    connect(m_addMusicButton, &DPushButton::clicked,
    this, [ = ] {
        Q_EMIT this->importFiles();
    });

    connect(m_importButton, &DPushButton::clicked, this,  &ImportWidget::slotImportButtonClicked);

    connect(m_text, &DLabel::linkActivated, this, &ImportWidget::slotLinkActivated);

    int themeType = DGuiApplicationHelper::instance()->themeType();
    slotTheme(themeType);
}

ImportWidget::~ImportWidget()
{

}

void ImportWidget::closeAnimationToDown(const QSize &size)
{
    this->resize(size);

    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->setDuration(AnimationDelay);
    animation->setStartValue(QPoint(0, 0));
    animation->setEndValue(QPoint(0, height()));

    animation->connect(animation, &QPropertyAnimation::finished,
                       animation, &QPropertyAnimation::deleteLater);
    animation->connect(animation, &QPropertyAnimation::finished,
                       this, &ImportWidget::hide);

    animation->start();
}

const QString ImportWidget::getLastImportPath() const
{
    QString lastImportPath = MusicSettings::value("base.play.last_import_path").toString();

    QDir lastImportDir = QDir(lastImportPath);
    if (!lastImportDir.exists() || lastImportPath.isEmpty()) {
        lastImportPath =  QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();
    } else {
        // blumia: always use the path from QDir, QDir also accept relative path string and it will pass
        //         the `QDir::exists()` checking
        lastImportPath = lastImportDir.absolutePath();
    }

    return lastImportPath;
}

void ImportWidget::showImportHint()
{
    m_importButton->setDisabled(false);
    m_importButton->show();
    m_addMusicButton->show();
    QString linkText = QString(linkTemplate).arg(tr("Scan")).arg(tr("Scan"));
    m_text->setText(tr("%1 music directory or drag music files here").arg(linkText));
}

void ImportWidget::slotLinkActivated(const QString &link)
{
    Q_UNUSED(link)
    showWaitHint();
    QStringList musicDir =  QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
    DataBaseService::getInstance()->importMedias(musicDir);
}

void ImportWidget::slotImportButtonClicked()
{
    DFileDialog fileDlg(this);
    QString lastImportPath = getLastImportPath();
    fileDlg.setDirectory(lastImportPath);
    fileDlg.setViewMode(DFileDialog::Detail);
    fileDlg.setFileMode(DFileDialog::Directory);
    if (DFileDialog::Accepted == fileDlg.exec()) {
        showWaitHint();
        MusicSettings::setOption("base.play.last_import_path",  fileDlg.directory().path());
        DataBaseService::getInstance()->importMedias(fileDlg.selectedFiles());
    }
}

bool ImportWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_text) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *event = static_cast<QKeyEvent *>(e);
            if (event->key() == Qt::Key_Return) {

                Q_EMIT this->scanMusicDirectory();

                auto pe =  m_text->palette();
                pe.setColor(QPalette::WindowText, QColor("#696969"));
                m_text->setPalette(pe);
            }
        } else if (e->type() == QEvent::FocusIn) {

            auto pe =  m_text->palette();
            pe.setColor(QPalette::WindowText, QColor("#A9A9A9"));
            m_text->setPalette(pe);

        } else if (e->type() == QEvent::FocusOut) {

            auto pe =  m_text->palette();
            pe.setColor(QPalette::WindowText, QColor("#696969"));
            m_text->setPalette(pe);
        }
    }

    return QWidget::eventFilter(o, e);
}


void ImportWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
        return;
    }

    DFrame::dragEnterEvent(event);
}

void ImportWidget::dragMoveEvent(QDragMoveEvent *)
{

}

void ImportWidget::dragLeaveEvent(QDragLeaveEvent *)
{

}

void ImportWidget::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat("text/uri-list")) {
        return;
    }

    auto urls = event->mimeData()->urls();
    QStringList localpaths;
    for (auto &url : urls) {
        localpaths << url.toLocalFile();
    }

    if (!localpaths.isEmpty()) {
        Q_EMIT importSelectFiles(localpaths);
    }
}
void ImportWidget::slotTheme(int type)
{
    QString rStr;
    if (type == 1) {
        rStr = "light";
        auto pl = m_importButton->palette();
        pl.setColor(DPalette::Dark, QColor("#0098FF"));
        pl.setColor(DPalette::Light, QColor("#25B7FF"));
        pl.setColor(DPalette::ButtonText, QColor("#FFFFFF"));
        QColor sbcolor("#000000");
        sbcolor.setAlphaF(0);
        pl.setColor(DPalette::Shadow, sbcolor);
//        d->importButton->setPalette(pl);

//        d->addMusicButton->setPalette(pl);

        QPalette pa = m_text->palette();
        pa.setColor(QPalette::WindowText, "#777777");
//        d->text->setPalette(pa);
        m_text->setForegroundRole(QPalette::WindowText);
//        d->text->setForegroundRole(DPalette::TextTips);
    } else {
        rStr = "dark";
        auto pl = m_importButton->palette();
        pl.setColor(DPalette::Dark, QColor("#0056C1"));
        pl.setColor(DPalette::Light, QColor("#004C9C"));
        pl.setColor(DPalette::ButtonText, QColor("#B8D3FF"));
        QColor sbcolor("#0091FF");
        sbcolor.setAlphaF(0.1);
        pl.setColor(DPalette::Shadow, sbcolor);
//        d->importButton->setPalette(pl);

//        d->addMusicButton->setPalette(pl);

        QPalette pa = m_text->palette();
        pa.setColor(QPalette::WindowText, "#798190");
//        d->text->setPalette(pa);
        m_text->setForegroundRole(QPalette::WindowText);
//        d->text->setForegroundRole(DPalette::TextTips);
    }
    m_logo->setPixmap(DHiDPIHelper::loadNxPixmap(QString(":/mpimage/%1/import_music.svg").arg(rStr)));
}

void ImportWidget::showWaitHint()
{
    m_importButton->setDisabled(true);
    m_importButton->hide();
    m_addMusicButton->hide();
    m_text->setText(tr("Loading music, please wait..."));
}
