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

#include "musiclistdatawidget.h"

#include <QDebug>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QMimeData>
#include <QResizeEvent>
#include <QStandardItemModel>

#include <DThemeManager>
#include <dlabel.h>
#include <dpushbutton.h>

#include "widget/playlistview.h"
#include "widget/musiclistdataview.h"
#include "widget/ddropdown.h"

DWIDGET_USE_NAMESPACE

class MusicListDataWidgetPrivate
{
public:
    MusicListDataWidgetPrivate(MusicListDataWidget *parent) : q_ptr(parent) {}

    void initData(PlaylistPtr playlist);
    void initConntion();
    void showEmptyHits();

    DLabel              *emptyHits      = nullptr;
    QWidget             *actionBar      = nullptr;
    DLabel              *titleLabel     = nullptr;
    DDropdown           *albumDropdown  = nullptr;
    DDropdown           *artistDropdown = nullptr;
    DDropdown           *musicDropdown  = nullptr;
    DPushButton         *btPlayAll      = nullptr;
    DLabel              *infoLabel      = nullptr;
    DPushButton         *btIconMode     = nullptr;
    DPushButton         *btlistMode     = nullptr;
    MusicListDataView   *albumListView = nullptr;
    MusicListDataView   *artistListView = nullptr;
    PlayListView        *musicListView  = nullptr;
    QAction             *customAction   = nullptr;
    PlaylistPtr         curPlaylist     = nullptr;

    MusicListDataWidget *q_ptr;
    Q_DECLARE_PUBLIC(MusicListDataWidget)
};


void MusicListDataWidgetPrivate::initData(PlaylistPtr playlist)
{
    Q_Q(MusicListDataWidget);

    curPlaylist = playlist;
    titleLabel->setText(playlist->displayName());
    DDropdown *t_curDropdown = nullptr;
    if (playlist->id() == AlbumMusicListID) {
        PlayMusicTypePtrList playMusicTypePtrList = playlist->playMusicTypePtrList();
        int musicCount = 0;
        for (auto action : playMusicTypePtrList) {
            musicCount += action->playlistMeta.sortMetas.size();
        }
        QString infoStr;
        if (musicCount == 0) {
            infoStr = MusicListDataWidget::tr("   No songs");
        } else if (musicCount == 1) {
            infoStr = MusicListDataWidget::tr("   1 album-1 song");
        } else {
            infoStr = MusicListDataWidget::tr("   %1 album-%2 songs").arg(playMusicTypePtrList.size()).arg(musicCount);
            if (playMusicTypePtrList.size() == 1) {
                infoStr = MusicListDataWidget::tr("   %1 album-%2 songs").arg(playMusicTypePtrList.size()).arg(musicCount);
            } else {
                infoStr = MusicListDataWidget::tr("   %1 albums-%2 songs").arg(playMusicTypePtrList.size()).arg(musicCount);
            }
        }
        infoLabel->setText(infoStr);

        //update dropdown
        albumDropdown->show();
        artistDropdown->hide();
        musicDropdown->hide();

        albumListView->show();
        artistListView->hide();
        musicListView->hide();

        t_curDropdown = albumDropdown;

        albumListView->onMusiclistChanged(playlist);
    } else if (playlist->id() == ArtistMusicListID) {
        PlayMusicTypePtrList playMusicTypePtrList = playlist->playMusicTypePtrList();
        int musicCount = 0;
        for (auto action : playMusicTypePtrList) {
            musicCount += action->playlistMeta.sortMetas.size();
        }
        QString infoStr;
        if (musicCount == 0) {
            infoStr = MusicListDataWidget::tr("   No songs");
        } else if (musicCount == 1) {
            infoStr = MusicListDataWidget::tr("   1 artist-1 song");
        } else {
            if (playMusicTypePtrList.size() == 1) {
                infoStr = MusicListDataWidget::tr("   %1 artist-%2 songs").arg(playMusicTypePtrList.size()).arg(musicCount);
            } else {
                infoStr = MusicListDataWidget::tr("   %1 artists-%2 songs").arg(playMusicTypePtrList.size()).arg(musicCount);
            }
        }
        infoLabel->setText(infoStr);

        //update dropdown
        albumDropdown->hide();
        artistDropdown->show();
        musicDropdown->hide();

        albumListView->hide();
        artistListView->show();
        musicListView->hide();

        t_curDropdown = artistDropdown;

        artistListView->onMusiclistChanged(playlist);
    } else {
        QString infoStr;
        if (playlist->allmusic().size() == 0) {
            infoStr = MusicListDataWidget::tr("   No songs");
        } else if (playlist->allmusic().size() == 1) {
            infoStr = MusicListDataWidget::tr("   1 song");
        } else {
            infoStr = MusicListDataWidget::tr("   %1 songs").arg(playlist->allmusic().size());
        }
        infoLabel->setText(infoStr);

        //update dropdown
        albumDropdown->hide();
        artistDropdown->hide();
        musicDropdown->show();

        albumListView->hide();
        artistListView->hide();
        musicListView->show();

        t_curDropdown = musicDropdown;

        musicListView->onMusiclistChanged(playlist);
    }

    if (playlist->sortType() == Playlist::SortByCustom) {
        q->setCustomSortType(playlist);
    } else {
        for (auto action : t_curDropdown->actions()) {
            if (action->data().toInt() == playlist->sortType()) {
                t_curDropdown->setCurrentAction(action);
            }
        }
    }
    showEmptyHits();
}

void MusicListDataWidgetPrivate::initConntion()
{
    Q_Q(MusicListDataWidget);

    q->connect(albumDropdown, &DDropdown::triggered,
    q, [ = ](QAction * action) {
        albumDropdown->setCurrentAction(action);
        Q_EMIT q->resort(musicListView->playlist(), action->data().value<Playlist::SortType>());
    });
    q->connect(artistDropdown, &DDropdown::triggered,
    q, [ = ](QAction * action) {
        artistDropdown->setCurrentAction(action);
        Q_EMIT q->resort(musicListView->playlist(), action->data().value<Playlist::SortType>());
    });
    q->connect(musicDropdown, &DDropdown::triggered,
    q, [ = ](QAction * action) {
        musicDropdown->setCurrentAction(action);
        Q_EMIT q->resort(musicListView->playlist(), action->data().value<Playlist::SortType>());
    });

    q->connect(btPlayAll, &QPushButton::clicked,
    q, [ = ](bool) {
        if (albumListView->isVisible()) {
            PlaylistPtr curPlayList = albumListView->playlist();
            if (curPlayList) {
                curPlayList->playMusicTypeToMeta();
                curPlayList->play(curPlayList->first());
                Q_EMIT q->playall(curPlayList);
                initData(curPlayList);
            }
        } else if (artistListView->isVisible()) {
            PlaylistPtr curPlayList = artistListView->playlist();
            if (curPlayList) {
                curPlayList->playMusicTypeToMeta();
                curPlayList->play(curPlayList->first());
                Q_EMIT q->playall(curPlayList);
                initData(curPlayList);
            }
        } else {
            if (musicListView->playlist()) {
                PlaylistPtr curPlayList = musicListView->playlist();
                curPlayList->play(curPlayList->first());
                Q_EMIT q->playall(curPlayList);
                initData(curPlayList);
            }
        }
    });

    q->connect(musicListView, &PlayListView::playMedia,
    q, [ = ](const MetaPtr meta) {
        PlaylistPtr curPlayList = musicListView->playlist();
        curPlayList->play(meta);
        initData(curPlayList);
        Q_EMIT q->playMedia(musicListView->playlist(), meta);
    });

    q->connect(btIconMode, &QPushButton::clicked,
    q, [ = ](bool) {
        if (albumListView->isVisible()) {
            albumListView->setViewModeFlag(QListView::IconMode);
        } else if (artistListView->isVisible()) {
            artistListView->setViewModeFlag(QListView::IconMode);
        }
    });
    q->connect(btlistMode, &QPushButton::clicked,
    q, [ = ](bool) {
        if (albumListView->isVisible()) {
            albumListView->setViewModeFlag(QListView::ListMode);
        } else if (artistListView->isVisible()) {
            artistListView->setViewModeFlag(QListView::ListMode);
        }
    });
}

void MusicListDataWidgetPrivate::showEmptyHits()
{
    if (albumListView->isVisible()) {
        auto playlist = artistListView->playlist();
        if (playlist.isNull() || playlist->id() != SearchMusicListID) {
            emptyHits->setText(MusicListDataWidget::tr("No Music"));
        } else {
            emptyHits->setText(MusicListDataWidget::tr("No result found"));
        }
        bool empty = false;//playlist.isNull();
        actionBar->setVisible(!empty);
        albumListView->setVisible(!empty);
        artistListView->setVisible(false);
        musicListView->setVisible(false);
        emptyHits->setVisible(empty);
    } else if (artistListView->isVisible()) {
        auto playlist = artistListView->playlist();
        if (playlist.isNull() || playlist->id() != SearchMusicListID) {
            emptyHits->setText(MusicListDataWidget::tr("No Music"));
        } else {
            emptyHits->setText(MusicListDataWidget::tr("No result found"));
        }
        bool empty = false;//playlist.isNull();
        actionBar->setVisible(!empty);
        artistListView->setVisible(!empty);
        albumListView->setVisible(false);
        musicListView->setVisible(false);
        emptyHits->setVisible(empty);
    } else {
        auto playlist = musicListView->playlist();
        if (playlist.isNull() || playlist->id() != SearchMusicListID) {
            emptyHits->setText(MusicListDataWidget::tr("No Music"));
        } else {
            emptyHits->setText(MusicListDataWidget::tr("No result found"));
        }
        bool empty = playlist.isNull();
        actionBar->setVisible(!empty);
        artistListView->setVisible(false);
        musicListView->setVisible(!empty);
        emptyHits->setVisible(empty);
    }
}

MusicListDataWidget::MusicListDataWidget(QWidget *parent) :
    QFrame(parent), d_ptr(new MusicListDataWidgetPrivate(this))
{
    Q_D(MusicListDataWidget);

    DThemeManager::instance()->registerWidget(this);

    setObjectName("MusicListDataWidget");
    setAcceptDrops(true);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(0);

    d->actionBar = new QFrame;
    d->actionBar->setFixedHeight(80);
    d->actionBar->setObjectName("MusicListDataActionBar");

    auto actionBarLayout = new QVBoxLayout(d->actionBar);
    actionBarLayout->setContentsMargins(10, 0, 8, 0);
    actionBarLayout->setSpacing(0);

    auto actionTileBarLayout = new QHBoxLayout();
    actionTileBarLayout->setContentsMargins(10, 0, 8, 0);
    actionTileBarLayout->setSpacing(0);

    d->titleLabel = new DLabel;
    d->titleLabel->setObjectName("MusicListDataTitle");
    d->titleLabel->setText(tr("All Music"));

    d->albumDropdown = new DDropdown;
    d->albumDropdown->setFixedHeight(28);
    d->albumDropdown->setMinimumWidth(130);
    d->albumDropdown->setObjectName("MusicListAlbumDataSort");
    d->albumDropdown->addAction(tr("Time added"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByAddTime));
    d->albumDropdown->addAction(tr("Album name"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByTitle));
    d->albumDropdown->hide();

    d->artistDropdown = new DDropdown;
    d->artistDropdown->setFixedHeight(28);
    d->artistDropdown->setMinimumWidth(130);
    d->artistDropdown->setObjectName("MusicListArtistDataSort");
    d->artistDropdown->addAction(tr("Time added"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByAddTime));
    d->artistDropdown->addAction(tr("Artist"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByArtist));
    d->artistDropdown->hide();

    d->musicDropdown = new DDropdown;
    d->musicDropdown->setFixedHeight(28);
    d->musicDropdown->setMinimumWidth(130);
    d->musicDropdown->setObjectName("MusicListMusicDataSort");
    d->musicDropdown->addAction(tr("Time added"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByAddTime));
    d->musicDropdown->addAction(tr("Title"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByTitle));
    d->musicDropdown->addAction(tr("Artist"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByArtist));
    d->musicDropdown->addAction(tr("Album name"), QVariant::fromValue<Playlist::SortType>(Playlist::SortByAblum));

    actionTileBarLayout->addWidget(d->titleLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    actionTileBarLayout->addStretch();
    actionTileBarLayout->addWidget(d->albumDropdown, 0, Qt::AlignCenter);
    actionTileBarLayout->addWidget(d->artistDropdown, 0, Qt::AlignCenter);
    actionTileBarLayout->addWidget(d->musicDropdown, 0, Qt::AlignCenter);

    auto actionInfoBarLayout = new QHBoxLayout();
    actionInfoBarLayout->setContentsMargins(10, 0, 8, 0);
    actionInfoBarLayout->setSpacing(0);

    d->btPlayAll = new DPushButton;
    d->btPlayAll->setObjectName("MusicListDataPlayAll");
    d->btPlayAll->setText(tr("Play All"));
    d->btPlayAll->setFixedHeight(36);
    d->btPlayAll->setFocusPolicy(Qt::NoFocus);

    d->infoLabel = new DLabel;
    d->infoLabel->setObjectName("MusicListDataTitle");
    d->infoLabel->setText(tr("All Music"));

    d->btIconMode = new DPushButton(":/common/image/picturelist_normal.svg", ":/common/image/picturelist_hover.svg",
                                    ":/common/image/picturelist_press.svg");
    d->btIconMode->setFixedSize(36, 36);
    d->btIconMode->setObjectName("MusicListDataWidgetIconMode");
    d->btIconMode->setCheckable(true);
    d->btIconMode->setChecked(true);

    d->btlistMode = new DPushButton(":/common/image/textlist_normal.svg", ":/common/image/textlist_hover.svg",
                                    ":/common/image/textlist_press.svg");
    d->btlistMode->setFixedSize(36, 36);
    d->btlistMode->setObjectName("MusicListDataWidgetListMode");
    d->btlistMode->setCheckable(true);

    actionInfoBarLayout->addWidget(d->btPlayAll, 0, Qt::AlignVCenter);
    actionInfoBarLayout->addWidget(d->infoLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    actionInfoBarLayout->addStretch();
    actionInfoBarLayout->addWidget(d->btIconMode, 0, Qt::AlignCenter);
    actionInfoBarLayout->addWidget(d->btlistMode, 0, Qt::AlignCenter);

    actionBarLayout->addLayout(actionTileBarLayout);
    actionBarLayout->addLayout(actionInfoBarLayout);

    d->emptyHits = new DLabel();
    d->emptyHits->setObjectName("MusicListDataEmptyHits");
    d->emptyHits->hide();

    d->albumListView = new MusicListDataView;
    d->artistListView = new MusicListDataView;
    d->musicListView = new PlayListView;
    d->musicListView->hide();

    layout->addWidget(d->actionBar, 0, Qt::AlignTop);
    layout->addWidget(d->albumListView, 100);
    layout->addStretch();
    layout->addWidget(d->artistListView, 100);
    layout->addStretch();
    layout->addWidget(d->musicListView, 100);
    layout->addStretch();
    layout->addWidget(d->emptyHits, 0, Qt::AlignCenter);
    layout->addStretch();

    d->initConntion();
}

MusicListDataWidget::~MusicListDataWidget()
{
}

void MusicListDataWidget::setCustomSortType(PlaylistPtr playlist)
{
    Q_D(MusicListDataWidget);

    DDropdown *t_curDropdown = nullptr;
    if (playlist->id() == AlbumMusicListID) {
        t_curDropdown = d->albumDropdown;
    } else if (playlist->id() == ArtistMusicListID) {
        t_curDropdown = d->artistDropdown;
    } else {
        t_curDropdown = d->musicDropdown;
    }
    t_curDropdown->setCurrentAction(nullptr);
    t_curDropdown->setText(tr("Custom"));
}

void MusicListDataWidget::onMusiclistChanged(PlaylistPtr playlist)
{
    if (playlist.isNull()) {
        qWarning() << "can not change to emptry playlist";
        return;
    }

    Q_D(MusicListDataWidget);

    d->initData(playlist);
}

void MusicListDataWidget::onMusiclistUpdate()
{
    Q_D(MusicListDataWidget);
    d->initData(d->curPlaylist);
}

