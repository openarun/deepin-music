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

#include "footerwidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QStackedLayout>
#include <QDBusInterface>
#include <QDBusReply>
#include <QGSettings>
#include <QFrame>
#include <QShortcut>
#include <QFileInfo>

#include <DHiDPIHelper>
#include <DPushButton>
#include <DProgressBar>
#include <DFloatingWidget>
#include <DPalette>
#include <DButtonBox>
#include <DToolTip>
#include <DBackgroundGroup>
#include <DGuiApplicationHelper>

#include <metadetector.h>
#include "../core/musicsettings.h"
#include "../core/util/threadpool.h"
#include "metabufferdetector.h"

#include "../presenter/commonservice.h"
#include "util/global.h"

#include "widget/label.h"
#include "widget/musicimagebutton.h"
#include "widget/musicpixmapbutton.h"
#include "widget/waveform.h"
#include "widget/soundvolume.h"
#include "playlistwidget.h"
#include "databaseservice.h"
#include "ac-desktop-define.h"

static const QString sPlayStatusValuePlaying    = "playing";
static const QString sPlayStatusValuePause      = "pause";
static const QString sPlayStatusValueStop       = "stop";

static const int AnimationDelay = 400; //ms
static const int VolumeStep = 10;

DGUI_USE_NAMESPACE

FooterWidget::FooterWidget(QWidget *parent) :
    DFloatingWidget(parent)
{
    setFocusPolicy(Qt::ClickFocus);
    setObjectName("Footer");
    this->setBlurBackgroundEnabled(true);
    this->blurBackground()->setRadius(30);
    this->blurBackground()->setBlurEnabled(true);
    this->blurBackground()->setMode(DBlurEffectWidget::GaussianBlur);
    QColor backMaskColor(255, 255, 255, 140);
    this->blurBackground()->setMaskColor(backMaskColor);
    initUI(parent);
    slotTheme(DGuiApplicationHelper::instance()->themeType());

    initShortcut();
}

FooterWidget::~FooterWidget()
{

}

void FooterWidget::initUI(QWidget *parent)
{
    auto backLayout = new QVBoxLayout(this);
    backLayout->setSpacing(0);
    backLayout->setContentsMargins(0, 0, 0, 0);

    m_forwardWidget = new DBlurEffectWidget(this);
    m_forwardWidget->setBlurRectXRadius(18);
    m_forwardWidget->setBlurRectYRadius(18);
    m_forwardWidget->setRadius(30);
    m_forwardWidget->setBlurEnabled(true);
    m_forwardWidget->setMode(DBlurEffectWidget::GaussianBlur);
    QColor maskColor(255, 255, 255, 76);
    m_forwardWidget->setMaskColor(maskColor);
    refreshBackground();

//    this->layout()->addWidget(m_forwardWidget);

    auto mainHBoxlayout = new QHBoxLayout(m_forwardWidget);
    mainHBoxlayout->setSpacing(10);
    mainHBoxlayout->setContentsMargins(10, 10, 10, 10);

//    auto downWidget = new DWidget();
//    downWidget->setStyleSheet("background-color:red;");
//    auto layout = new QHBoxLayout(downWidget);
//    layout->setContentsMargins(0, 0, 10, 0);
//    mainVBoxlayout->addWidget(downWidget);

//    m_btPrev = new DButtonBoxButton(QIcon::fromTheme("music_last"), "", this);
    m_btPrev = new DToolButton(this);
    m_btPrev->setIcon(QIcon::fromTheme("music_last"));
    m_btPrev->setIconSize(QSize(36, 36));
    m_btPrev->setObjectName("FooterActionPrev");
    m_btPrev->setFixedSize(40, 50);
    AC_SET_OBJECT_NAME(m_btPrev, AC_Prev);
    AC_SET_ACCESSIBLE_NAME(m_btPrev, AC_Prev);

//    m_btPlay = new DButtonBoxButton(QIcon::fromTheme("music_play"), "", this);
    m_btPlay = new DToolButton(this);
    setPlayProperty(Player::PlaybackStatus::Paused);
//    m_btPlay->setIcon(QIcon::fromTheme("music_play"));
    m_btPlay->setIconSize(QSize(36, 36));
    m_btPlay->setFixedSize(40, 50);

    AC_SET_OBJECT_NAME(m_btPlay, AC_Play);
    AC_SET_ACCESSIBLE_NAME(m_btPlay, AC_Play);

//    m_btNext = new DButtonBoxButton(QIcon::fromTheme("music_next"), "", this);
    m_btNext = new DToolButton(this);
    m_btNext->setIcon(QIcon::fromTheme("music_next"));
    m_btNext->setIconSize(QSize(36, 36));
    m_btNext->setObjectName("FooterActionNext");
    m_btNext->setFixedSize(40, 50);

    AC_SET_OBJECT_NAME(m_btNext, AC_Next);
    AC_SET_ACCESSIBLE_NAME(m_btNext, AC_Next);

    QHBoxLayout *groupHlayout = new QHBoxLayout();
    groupHlayout->setSpacing(0);
    groupHlayout->setContentsMargins(0, 0, 0, 0);
    m_ctlWidget = new DBackgroundGroup(groupHlayout, this);
    m_ctlWidget->setFixedHeight(50);
    m_ctlWidget->setItemSpacing(0);
    m_ctlWidget->setUseWidgetBackground(false);
    QMargins margins(0, 0, 0, 0);
    m_ctlWidget->setItemMargins(margins);
    groupHlayout->addWidget(m_btPrev);
    groupHlayout->addWidget(m_btPlay);
    groupHlayout->addWidget(m_btNext);
    mainHBoxlayout->addWidget(m_ctlWidget, 0);
    //添加封面按钮
    m_btCover = new MusicPixmapButton(this);
    m_btCover->setIcon(QIcon::fromTheme("info_cover"));
    m_btCover->setObjectName("FooterCoverHover");
    m_btCover->setFixedSize(50, 50);
    m_btCover->setIconSize(QSize(50, 50));
    mainHBoxlayout->addWidget(m_btCover, 0);

    AC_SET_OBJECT_NAME(m_btCover, AC_btCover);
    AC_SET_ACCESSIBLE_NAME(m_btCover, AC_btCover);

    //添加歌曲名
    m_title = new Label;
    auto titleFont = m_title->font();
    titleFont.setFamily("SourceHanSansSC");
    titleFont.setWeight(QFont::Normal);
    titleFont.setPixelSize(12);
    m_title->setFont(titleFont);
    m_title->setObjectName("FooterTitle");
    m_title->setMaximumWidth(140);
    m_title->setText(tr("Unknown Title"));
    m_title->setForegroundRole(DPalette::BrightText);
    //添加歌唱者
    m_artist = new Label;
    auto artistFont = m_artist->font();
    artistFont.setFamily("SourceHanSansSC");
    artistFont.setWeight(QFont::Normal);
    artistFont.setPixelSize(11);
    m_artist->setFont(titleFont);
    m_artist->setObjectName("FooterArtist");
    m_artist->setMaximumWidth(140);
    m_artist->setText(tr("Unknown artist"));
    auto artistPl = m_title->palette();
    QColor artistColor = artistPl.color(DPalette::BrightText);
    artistColor.setAlphaF(0.6);
    artistPl.setColor(DPalette::WindowText, artistColor);
    m_artist->setPalette(artistPl);
    m_artist->setForegroundRole(DPalette::WindowText);
    auto musicMetaLayout = new QVBoxLayout;
    musicMetaLayout->setContentsMargins(0, 0, 0, 0);
    musicMetaLayout->setSpacing(0);
    musicMetaLayout->addStretch(100);
    musicMetaLayout->addWidget(m_title);
    musicMetaLayout->addWidget(m_artist);
    musicMetaLayout->addStretch(100);
    mainHBoxlayout->addLayout(musicMetaLayout);
    //添加进度条
    m_waveform = new Waveform(Qt::Horizontal, static_cast<QWidget *>(parent), this);
    m_waveform->setMinimum(0);
    m_waveform->setMaximum(1000);
    m_waveform->setValue(0);
    m_waveform->adjustSize();
    mainHBoxlayout->addWidget(m_waveform, 100);

    AC_SET_OBJECT_NAME(m_waveform, AC_Waveform);
    AC_SET_ACCESSIBLE_NAME(m_waveform, AC_Waveform);

    //添加收藏按钮
    m_btFavorite = new DIconButton(this);
    m_btFavorite->setIcon(QIcon::fromTheme("dcc_collection"));
//    m_btFavorite->setIcon(QIcon::fromTheme("collection1_press"));
    m_btFavorite->setObjectName("FooterActionFavorite");
    m_btFavorite->setShortcut(QKeySequence::fromString("."));
    m_btFavorite->setFixedSize(50, 50);
    m_btFavorite->setIconSize(QSize(36, 36));
    mainHBoxlayout->addWidget(m_btFavorite, 0);

    AC_SET_OBJECT_NAME(m_btFavorite, AC_Favorite);
    AC_SET_ACCESSIBLE_NAME(m_btFavorite, AC_Favorite);

    //添加歌词按钮
    m_btLyric = new DIconButton(this);
    m_btLyric->setIcon(QIcon::fromTheme("lyric"));
    m_btLyric->setObjectName("FooterActionLyric");
    m_btLyric->setFixedSize(50, 50);
    m_btLyric->setIconSize(QSize(36, 36));
    m_btLyric->setCheckable(true);
    mainHBoxlayout->addWidget(m_btLyric, 0);

    AC_SET_OBJECT_NAME(m_btLyric, AC_Lyric);
    AC_SET_ACCESSIBLE_NAME(m_btLyric, AC_Lyric);

    //添加播放模式
    m_btPlayMode = new DIconButton(this);
    m_btPlayMode->setIcon(QIcon::fromTheme("sequential_loop"));
    m_btPlayMode->setObjectName("FooterActionPlayMode");
    m_btPlayMode->setFixedSize(50, 50);
    m_btPlayMode->setIconSize(QSize(36, 36));
    m_btPlayMode->setProperty("playModel", QVariant(0));
    mainHBoxlayout->addWidget(m_btPlayMode, 0);

    AC_SET_OBJECT_NAME(m_btPlayMode, AC_PlayMode);
    AC_SET_ACCESSIBLE_NAME(m_btPlayMode, AC_PlayMode);
    //添加音量调节按钮
    m_btSound = new DIconButton(this);
    m_btSound->setIcon(QIcon::fromTheme("volume_mid"));
    m_btSound->setObjectName("FooterActionSound");
    m_btSound->setFixedSize(50, 50);
    m_btSound->setProperty("volume", "mid");
    m_btSound->setCheckable(true);
    m_btSound->setIconSize(QSize(36, 36));
    mainHBoxlayout->addWidget(m_btSound, 0);

    AC_SET_OBJECT_NAME(m_btSound, AC_Sound);
    AC_SET_ACCESSIBLE_NAME(m_btSound, AC_Sound);
    //添加歌曲列表按钮
    m_btPlayList = new DIconButton(this);
    m_btPlayList->setIcon(QIcon::fromTheme("playlist"));
    m_btPlayList->setObjectName("FooterActionPlayList");
    m_btPlayList->setFixedSize(50, 50);
    m_btPlayList->setCheckable(true);
    m_btPlayList->setIconSize(QSize(36, 36));
    mainHBoxlayout->addWidget(m_btPlayList, 0);

    AC_SET_OBJECT_NAME(m_btPlayList, AC_PlayList);
    AC_SET_ACCESSIBLE_NAME(m_btPlayList, AC_PlayList);

    // 音量控件
    m_volSlider = new SoundVolume(this->parentWidget());
    m_volSlider->hide();
    m_volSlider->setProperty("DelayHide", true);
    m_volSlider->setProperty("NoDelayShow", true);

    AC_SET_OBJECT_NAME(m_volSlider, AC_VolSlider);
    AC_SET_ACCESSIBLE_NAME(m_volSlider, AC_VolSlider);

    m_metaBufferDetector = new MetaBufferDetector(nullptr);
    connect(m_metaBufferDetector, SIGNAL(metaBuffer(const QVector<float> &, const QString &)),
            m_waveform, SLOT(onAudioBuffer(const QVector<float> &, const QString &)));

//    connect(m_metaBufferDetector, SIGNAL(metaBuffer(bool)), this, SLOT(slotPlaylistClick(bool)));

    connect(m_btPlayList, SIGNAL(clicked(bool)), this, SLOT(slotPlaylistClick(bool)));
    connect(m_btLyric, SIGNAL(clicked(bool)), this, SLOT(slotLrcClick(bool)));
    connect(m_btPlayMode, SIGNAL(clicked(bool)), this, SLOT(slotPlayModeClick(bool)));
    connect(m_btCover, SIGNAL(clicked(bool)), this, SLOT(slotCoverClick(bool)));
    connect(m_btPlay, SIGNAL(clicked(bool)), this, SLOT(slotPlayClick(bool)));
    connect(m_btNext, SIGNAL(clicked(bool)), this, SLOT(slotNextClick(bool)));
    connect(m_btPrev, SIGNAL(clicked(bool)), this, SLOT(slotPreClick(bool)));
    connect(m_btSound, &DIconButton::clicked, this, &FooterWidget::slotSoundClick);

    connect(Player::instance(), &Player::signalPlaybackStatusChanged,
            this, &FooterWidget::slotPlaybackStatusChanged);
    connect(Player::instance(), &Player::signalMediaMetaChanged,
            this, &FooterWidget::slotMediaMetaChanged);

    connect(m_btFavorite, &DIconButton::clicked, this, &FooterWidget::slotFavoriteClick);
    connect(CommonService::getInstance(), &CommonService::favoriteMusic, this, &FooterWidget::favoriteMusic);
}

void FooterWidget::initShortcut()
{
    playPauseShortcut = new QShortcut(this);
    playPauseShortcut->setKey(QKeySequence(MusicSettings::value("shortcuts.all.play_pause").toString()));

    volUpShortcut = new QShortcut(this);
    volUpShortcut->setKey(QKeySequence(MusicSettings::value("shortcuts.all.volume_up").toString()));

    volDownShortcut = new QShortcut(this);
    volDownShortcut->setKey(QKeySequence(MusicSettings::value("shortcuts.all.volume_down").toString()));

    nextShortcut = new QShortcut(this);
    nextShortcut->setKey(QKeySequence(MusicSettings::value("shortcuts.all.next").toString()));

    previousShortcut = new QShortcut(this);
    previousShortcut->setKey(QKeySequence(MusicSettings::value("shortcuts.all.previous").toString()));

    muteShortcut = new QShortcut(this);
    muteShortcut->setKey(QKeySequence(QLatin1String("M")));
    //connect(muteShortcut, &QShortcut::activated, presenter, &Presenter::onLocalToggleMute);

    connect(playPauseShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    connect(volUpShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    connect(volDownShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    connect(nextShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    connect(previousShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    connect(muteShortcut, SIGNAL(activated()), this, SLOT(slotShortCutTriggered()));
    //dbus
    connect(Player::instance()->getMpris(), SIGNAL(volumeChanged()), this, SLOT(onVolumeChanged()));
}

void FooterWidget::updateShortcut()
{
    //it will be invoked when settings closed
    auto play_pauseStr = MusicSettings::value("shortcuts.all.play_pause").toString();
    if (play_pauseStr.isEmpty())
        playPauseShortcut->setEnabled(false);
    else {
        playPauseShortcut->setEnabled(true);
        playPauseShortcut->setKey(QKeySequence(play_pauseStr));
    }
    auto volume_upStr = MusicSettings::value("shortcuts.all.volume_up").toString();
    if (volume_upStr.isEmpty())
        volUpShortcut->setEnabled(false);
    else {
        volUpShortcut->setEnabled(true);
        volUpShortcut->setKey(QKeySequence(volume_upStr));
    }
    auto volume_downStr = MusicSettings::value("shortcuts.all.volume_down").toString();
    if (volume_downStr.isEmpty())
        volDownShortcut->setEnabled(false);
    else {
        volDownShortcut->setEnabled(true);
        volDownShortcut->setKey(QKeySequence(volume_downStr));
    }
    auto nextStr = MusicSettings::value("shortcuts.all.next").toString();
    if (nextStr.isEmpty())
        nextShortcut->setEnabled(false);
    else {
        nextShortcut->setEnabled(true);
        nextShortcut->setKey(QKeySequence(nextStr));
    }
    auto previousStr = MusicSettings::value("shortcuts.all.previous").toString();
    if (previousStr.isEmpty())
        previousShortcut->setEnabled(false);
    else {
        previousShortcut->setEnabled(true);
        previousShortcut->setKey(QKeySequence(previousStr));
    }
}
//设置播放按钮播放图标
void FooterWidget::setPlayProperty(Player::PlaybackStatus status)
{
    m_btPlay->setProperty("playstatue", status);
    if (status == Player::PlaybackStatus::Playing) {
        m_btPlay->setIcon(QIcon::fromTheme("suspend"));
    } else {
        m_btPlay->setIcon(QIcon::fromTheme("music_play"));
    }
}

void FooterWidget::slotPlayClick(bool click)
{
    Q_UNUSED(click)
    Player::PlaybackStatus statue = m_btPlay->property("playstatue").value<Player::PlaybackStatus>();
    if (statue == Player::PlaybackStatus::Playing) {
        Player::instance()->pause();
    } else if (statue == Player::PlaybackStatus::Paused) {
//        m_btPlay->setIcon(QIcon::fromTheme("music_play"));
        Player::instance()->resume();
    }
}

void FooterWidget::slotLrcClick(bool click)
{
    Q_UNUSED(click)
    emit lyricClicked();
}

void FooterWidget::slotPlayModeClick(bool click)
{
    Q_UNUSED(click)
    int playModel = m_btPlayMode->property("playModel").toInt();
    if (++playModel == 3)
        playModel = 0;

    switch (playModel) {
    case 0:
        m_btPlayMode->setIcon(QIcon::fromTheme("sequential_loop"));
        break;
    case 1:
        m_btPlayMode->setIcon(QIcon::fromTheme("single_tune_circulation"));
        break;
    case 2:
        m_btPlayMode->setIcon(QIcon::fromTheme("cross_cycling"));
        break;
    default:
        break;
    }

    m_btPlayMode->setProperty("playModel", QVariant(playModel));
    Player::instance()->setMode(static_cast<Player::PlaybackMode>(playModel));
}

void FooterWidget::slotCoverClick(bool click)
{
    Q_UNUSED(click)
    m_btLyric->setChecked(!m_btLyric->isChecked());
    emit lyricClicked();
}

void FooterWidget::slotNextClick(bool click)
{
    Q_UNUSED(click)
    Player::instance()->playNextMeta();
}

void FooterWidget::slotPreClick(bool click)
{
    Q_UNUSED(click)
    Player::instance()->playPreMeta();
}

void FooterWidget::slotFavoriteClick(bool click)
{
    Q_UNUSED(click)
    favoriteMusic(Player::instance()->activeMeta());
}

void FooterWidget::favoriteMusic(const MediaMeta meta)
{
    DataBaseService::getInstance()->favoriteMusic(meta);

    if (CommonService::getInstance()->getPlayClassification() == CommonService::PlayClassification::Music_My_Collection)
        emit CommonService::getInstance()->switchToView(FavType, "fav");


    if (DataBaseService::getInstance()->favoriteExist(Player::instance()->activeMeta())) {
        m_btFavorite->setIcon(QIcon::fromTheme("collection1_press"));
    } else {
        m_btFavorite->setIcon(QIcon::fromTheme("dcc_collection"));
    }
}

void FooterWidget::slotSoundClick(bool click)
{
    Q_UNUSED(click)
    if (m_volSlider->isVisible()) {
        m_volSlider->hide();
    } else {
        auto centerPos = m_btSound->mapToGlobal(m_btSound->rect().center());
        m_volSlider->show();
        m_volSlider->adjustSize();
        auto sz = m_volSlider->size();
        centerPos.setX(centerPos.x()  - sz.width() / 2);
        centerPos.setY(centerPos.y() - 32 - sz.height());
        centerPos = m_volSlider->mapFromGlobal(centerPos);
        centerPos = m_volSlider->mapToParent(centerPos);
        m_volSlider->move(centerPos);
        m_volSlider->raise();
    }
}

void FooterWidget::slotPlaybackStatusChanged(Player::PlaybackStatus statue)
{
    setPlayProperty(statue);
}

void FooterWidget::slotMediaMetaChanged()
{
    MediaMeta meta = Player::instance()->activeMeta();
    //替换封面按钮与背景图片
    QString imagesDirPath = Global::cacheDir() + "/images/" + meta.hash + ".jpg";
    QFileInfo file(imagesDirPath);
    QIcon icon;
    if (file.exists()) {
        icon = QIcon(imagesDirPath);
        m_btCover->setIcon(icon);
    } else {
        m_btCover->setIcon(QIcon::fromTheme("info_cover"));
    }
    refreshBackground();
    m_title->setText(meta.title.isEmpty() ? tr("Unknown Title") : meta.title);
    m_artist->setText(meta.singer.isEmpty() ? tr("Unknown artist") : meta.singer);
    m_metaBufferDetector->onClearBufferDetector();
    m_metaBufferDetector->onBufferDetector(meta.localPath, meta.hash);

    if (DataBaseService::getInstance()->favoriteExist(Player::instance()->activeMeta())) {
        m_btFavorite->setIcon(QIcon::fromTheme("collection1_press"));
    } else {
        m_btFavorite->setIcon(QIcon::fromTheme("dcc_collection"));
    }
}

void FooterWidget::onVolumeChanged() //from dbus set
{
    //need to sync volume to dbus
//    if (d->volumeMonitoring.needSyncLocalFlag(1)) {
//        d->volumeMonitoring.stop();
//        d->volumeMonitoring.timeoutSlot();
//        d->volumeMonitoring.start();
//    }
    //get dbus volume
    double mprivol = Player::instance()->getMpris()->volume();
    int volume = int(mprivol * 100);
    QString status = "mid";
    if (volume > 77) {
        status = "high";
        m_btSound->setIcon(QIcon::fromTheme("volume"));

    } else if (volume > 33) {
        status = "mid";
        m_btSound->setIcon(QIcon::fromTheme("volume_mid"));

    } else {
        status = "low";
        m_btSound->setIcon(QIcon::fromTheme("volume_low"));
    }

    if (m_mute || volume == 0) {
        m_btSound->setProperty("volume", "mute");
        m_btSound->update();
        m_btSound->setIcon(QIcon::fromTheme("mute"));
        m_volSlider->syncMute(true);
    } else {
        m_btSound->setProperty("volume", status);
        m_btSound->update();
    }

    if (!m_mute && volume > 0) {
        m_volSlider->syncMute(false);
    }

    //m_Volume = volume;
    MusicSettings::setOption("base.play.volume", volume);
    // 音量变化为0，设置为静音
    MusicSettings::setOption("base.play.mute", m_mute);
    m_volSlider->setVolumeFromExternal(volume);
}

void FooterWidget::slotDelayAutoHide()
{
    m_btSound->setChecked(false);
}

void FooterWidget::slotShortCutTriggered()
{
    QShortcut *objCut =   dynamic_cast<QShortcut *>(sender()) ;
    Q_ASSERT(objCut);

    if (objCut == volUpShortcut) {
        //dbus volume up
        int volup = Player::instance()->volume() + VolumeStep;
        if (volup > 100)//max volume
            volup = 100;
        Player::instance()->setVolume(volup); //system volume

        Player::instance()->getMpris()->setVolume(static_cast<double>(volup) / 100);
    }

    if (objCut == volDownShortcut) {
        //dbus volume up
        int voldown = Player::instance()->volume() - VolumeStep;
        if (voldown < 0)//mini volume
            voldown = 0;
        Player::instance()->setVolume(voldown); //system volume

        Player::instance()->getMpris()->setVolume(static_cast<double>(voldown) / 100);
    }

    if (objCut == nextShortcut) {
        Player::instance()->playNextMeta();
    }

    if (objCut == playPauseShortcut) { //pause
        slotPlayClick(true);
    }

    if (objCut == previousShortcut) {
        Player::instance()->playPreMeta();
    }

    if (objCut == muteShortcut) {
        Player::instance()->setMuted(true);
        m_volSlider->syncMute(true);
    }
}

void FooterWidget::slotPlaylistClick(bool click)
{
    Q_UNUSED(click)
    qDebug() << "zy------FooterWidget::onPlaylistClick";
    int height = 0;
    int width = 0;
    if (static_cast<QWidget *>(parent())) {
        height = static_cast<QWidget *>(parent())->height();
        width = static_cast<QWidget *>(parent())->width();
    }
    if (m_playListWidget == nullptr) {
        m_playListWidget = new PlayListWidget(this);
        // 初始化播放列表数据
        m_playListWidget->slotPlayListChanged();
        connect(Player::instance(), &Player::signalPlayListChanged, m_playListWidget, &PlayListWidget::slotPlayListChanged);
        m_playListWidget->hide();
    }

    if (m_playListWidget->isHidden()) {
        QRect start2(0, height - 85, width, 80);
        QRect end2(0, height - 450, width, 445);
        QPropertyAnimation *animation2 = new QPropertyAnimation(this, "geometry");
        animation2->setEasingCurve(QEasingCurve::InCurve);
        animation2->setDuration(AnimationDelay);
        animation2->setStartValue(start2);
        animation2->setEndValue(end2);
        animation2->start();
        m_playListWidget->show();
        animation2->connect(animation2, &QPropertyAnimation::finished,
                            animation2, &QPropertyAnimation::deleteLater);

        m_forwardWidget->setSourceImage(QImage());
    } else if (m_playListWidget->isVisible()) {
        QRect start2(0, height - 85, width, 80);
        QRect end2(0, height - 450, width, 445);
        QPropertyAnimation *animation2 = new QPropertyAnimation(this, "geometry");
        animation2->setEasingCurve(QEasingCurve::InCurve);
        animation2->setDuration(AnimationDelay);
        animation2->setStartValue(end2);
        animation2->setEndValue(start2);
        animation2->start();
        animation2->connect(animation2, &QPropertyAnimation::finished,
                            animation2, &QPropertyAnimation::deleteLater);
        animation2->connect(animation2, &QPropertyAnimation::finished, m_playListWidget, [ = ]() {
            m_playListWidget->hide();
            refreshBackground();
        });
    }
}

void FooterWidget::showEvent(QShowEvent *event)
{
    if (m_playListWidget) {
        m_playListWidget->setFixedWidth(width());
    }
    if (m_forwardWidget) {
        m_forwardWidget->setGeometry(6, height() - 75, width() - 12, 70);
    }
}

void FooterWidget::mousePressEvent(QMouseEvent *event)
{
}

void FooterWidget::mouseReleaseEvent(QMouseEvent *event)
{
}

void FooterWidget::mouseMoveEvent(QMouseEvent *event)
{
}

//bool FooterWidget::eventFilter(QObject *obj, QEvent *event)
//{
//    return true;
//}

void FooterWidget::resizeEvent(QResizeEvent *event)
{
    if (m_forwardWidget) {
        m_forwardWidget->setGeometry(6, height() - 75, width() - 12, 70);
    }
    if (m_playListWidget) {
        m_playListWidget->setGeometry(0, 0, width(), height() - 80);
    }
    DWidget::resizeEvent(event);
}

void FooterWidget::refreshBackground()
{
    QImage cover = QImage(":/icons/deepin/builtin/actions/info_cover_142px.svg");
    QString imagesDirPath = Global::cacheDir() + "/images/" + Player::instance()->activeMeta().hash + ".jpg";
    QFileInfo file(imagesDirPath);
    if (file.exists()) {
        cover = QImage(Global::cacheDir() + "/images/" + Player::instance()->activeMeta().hash + ".jpg");
    }

    //cut image
//    qDebug() << "-------cover.width() = " << cover.width();
    double windowScale = (width() * 1.0) / height();

//    qDebug() << "-------windowScale = " << windowScale;
    int imageWidth = static_cast<int>(cover.height() * windowScale);
//    qDebug() << "-------imageWidth = " << imageWidth;
    QImage coverImage;
    if (m_playListWidget && m_playListWidget->isVisible()) {
        coverImage.fill(QColor(255, 255, 255));
    } else {
        if (imageWidth > cover.width()) {
            int imageheight = static_cast<int>(cover.width() / windowScale);
            coverImage = cover.copy(0, (cover.height() - imageheight) / 2, cover.width(), imageheight);
        } else {
            int imageheight = cover.height();
            coverImage = cover.copy((cover.width() - imageWidth) / 2, 0, imageWidth, imageheight);
        }
    }

    m_forwardWidget->setSourceImage(coverImage);
    m_forwardWidget->update();
}

void FooterWidget::slotTheme(int type)
{
    QString rStr;
    if (type == 1) {
        QColor backMaskColor(255, 255, 255, 140);
        this->blurBackground()->setMaskColor(backMaskColor);
        QColor maskColor(255, 255, 255, 76);
        m_forwardWidget->setMaskColor(maskColor);
        rStr = "light";

        auto artistPl = m_artist->palette();
        QColor artistColor = artistPl.color(DPalette::BrightText);
        artistColor.setAlphaF(0.4);
        artistPl.setColor(DPalette::WindowText, artistColor);
        m_artist->setPalette(artistPl);

        DPalette pa;
        pa = m_btFavorite->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btFavorite->setPalette(pa);

        pa = m_btPlay->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btPlay->setPalette(pa);

        pa = m_btLyric->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btLyric->setPalette(pa);

        pa = m_btPlayMode->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btPlayMode->setPalette(pa);

        pa = m_btSound->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btSound->setPalette(pa);

        pa = m_btPlayList->palette();
        pa.setColor(DPalette::Light, QColor("#FFFFFF"));
        pa.setColor(DPalette::Dark, QColor("#FFFFFF"));
        m_btPlayList->setPalette(pa);
    } else {
        QColor backMaskColor(37, 37, 37, 140);
        blurBackground()->setMaskColor(backMaskColor);
        QColor maskColor(37, 37, 37, 76);
        m_forwardWidget->setMaskColor(maskColor);
        rStr = "dark";

        auto artistPl = m_artist->palette();
        QColor artistColor = artistPl.color(DPalette::BrightText);
        artistColor.setAlphaF(0.6);
        artistPl.setColor(DPalette::WindowText, artistColor);
        m_artist->setPalette(artistPl);

        DPalette pa;
        pa = m_btFavorite->palette();
        pa.setColor(DPalette::Light, QColor("#444444"));
        pa.setColor(DPalette::Dark, QColor("#444444"));
        m_btFavorite->setPalette(pa);

        pa = m_btLyric->palette();
        pa.setColor(DPalette::Light, QColor("#444444"));
        pa.setColor(DPalette::Dark, QColor("#444444"));
        m_btLyric->setPalette(pa);

        pa = m_btPlayMode->palette();
        pa.setColor(DPalette::Light, QColor("#444444"));
        pa.setColor(DPalette::Dark, QColor("#444444"));
        m_btPlayMode->setPalette(pa);

        pa = m_btSound->palette();
        pa.setColor(DPalette::Light, QColor("#444444"));
        pa.setColor(DPalette::Dark, QColor("#444444"));
        m_btSound->setPalette(pa);

        pa = m_btPlayList->palette();
        pa.setColor(DPalette::Light, QColor("#444444"));
        pa.setColor(DPalette::Dark, QColor("#444444"));
        m_btPlayList->setPalette(pa);
    }


    m_volSlider->slotTheme(type);
}
