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

#pragma once

#include <DListView>

#include "commonservice.h"
#include "databaseservice.h"

DWIDGET_USE_NAMESPACE

class AlbumDataModel;
class AlbumDataDelegate;
class MusicListDialog;
class AlbumListView : public DListView
{
    Q_OBJECT
public:
    explicit AlbumListView(QString hash, QWidget *parent = Q_NULLPTR);
    ~AlbumListView();
    /**
     * @brief setAlbumListData set album data from DataBaseService to delegate
     * @param listinfo all album data
     */
    void setAlbumListData(const QList<AlbumInfo> &listinfo);
    //展示专辑名包含str的专辑
    void resetAlbumListDataByStr(const QString &searchWord);
    //展示包含列表中歌曲的专辑
    void resetAlbumListDataBySongName(const QList<MediaMeta> &mediaMetas);
    //展示包含列表中歌手的专辑
    void resetAlbumListDataBySinger(const QList<SingerInfo> &singerInfos);

    QList<AlbumInfo> getAlbumListData() const;

    int rowCount();

    void setViewModeFlag(QListView::ViewMode mode);
    QListView::ViewMode getViewMode();

    MediaMeta playing() const;
    MediaMeta hoverin() const;
    bool  playingState()const;

    void setThemeType(int type);
    int getThemeType() const;

    void setPlayPixmap(QPixmap pixmap, QPixmap sidebarPixmap, QPixmap albumPixmap);
    QPixmap getPlayPixmap() const;
    QPixmap getSidebarPixmap() const;
    QPixmap getAlbumPixmap() const;

    void updateList();

    //排序
    DataBaseService::ListSortType getSortType();
    void setSortType(DataBaseService::ListSortType sortType);
signals:
    void requestCustomContextMenu(const QPoint &pos);
    void modeChanged(int);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void slotCoverUpdate(const MediaMeta &meta);

private:
    int                     musicTheme     = 1; //light theme
    AlbumDataModel          *albumModel    = nullptr;
    AlbumDataDelegate       *albumDelegate = nullptr;
    MediaMeta                 playingMeta;
    MediaMeta                 hoverinMeta;
    QPixmap                 playingPix = QPixmap(":/mpimage/light/music1.svg");
    QPixmap                 sidebarPix = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    QPixmap                 albumPix   = QPixmap(":/mpimage/light/music_white_album_cover/music1.svg");
    MusicListDialog        *musciListDialog = nullptr;
    QList<AlbumInfo>        m_albumInfoList; //all album data
    QString                 m_hash;
    QListView::ViewMode     m_viewModel = QListView::ListMode;
    QIcon                   m_defaultIcon = QIcon(":/common/image/cover_max.svg");
};
