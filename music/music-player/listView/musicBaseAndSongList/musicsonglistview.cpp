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

#include "musicsonglistview.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>

#include <DMenu>
#include <DDialog>
#include <DScrollBar>
#include <DPalette>
#include <DApplicationHelper>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QPainter>
#include <QShortcut>
#include <QUuid>
#include "mediameta.h"

#include "commonservice.h"
#include "musicbaseandsonglistmodel.h"
#include "databaseservice.h"
#include "player.h"

DGUI_USE_NAMESPACE

MusicSongListView::MusicSongListView(QWidget *parent) : DListView(parent)
{
    model = new MusicBaseAndSonglistModel(this);
    setModel(model);
    delegate = new DStyledItemDelegate(this);
    //delegate->setBackgroundType(DStyledItemDelegate::NoBackground);
    auto delegateMargins = delegate->margins();
    delegateMargins.setLeft(18);
    delegate->setMargins(delegateMargins);
    setItemDelegate(delegate);

    setViewportMargins(8, 0, 8, 0);

    playingPixmap = QPixmap(":/mpimage/light/music1.svg");
    albumPixmap = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    defaultPixmap = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    auto font = this->font();
    font.setFamily("SourceHanSansSC");
    font.setWeight(QFont::Medium);
    font.setPixelSize(14);
    setFont(font);

    setIconSize(QSize(20, 20));
    setItemSize(QSize(40, 40));

    setFrameShape(QFrame::NoFrame);

    DPalette pa = DApplicationHelper::instance()->palette(this);
    pa.setColor(DPalette::ItemBackground, Qt::transparent);
    DApplicationHelper::instance()->setPalette(this, pa);

    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setSelectionMode(QListView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContextMenuPolicy(Qt::CustomContextMenu);

    init();

    connect(this, &MusicSongListView::clicked, this, [](QModelIndex midx) {
        qDebug() << "customize midx.row()" << midx.row();
        emit CommonService::getInstance()->switchToView(CustomType, midx.data(Qt::UserRole).toString());
    });

    connect(Player::instance(), SIGNAL(signalUpdatePlayingIcon()),
            this, SLOT(slotUpdatePlayingIcon()), Qt::DirectConnection);
}

MusicSongListView::~MusicSongListView()
{

}

void MusicSongListView::init()
{
    QList<DataBaseService::PlaylistData> list = DataBaseService::getInstance()->allPlaylistMeta();
    QString rStr;
    if (m_type == 1) {
        rStr = "light";
    } else {
        rStr = "dark";
    }

    for (int i = 0; i < list.size(); i++) {
        DataBaseService::PlaylistData data = list.at(i);
        if (data.uuid == "album" || data.uuid == "artist" || data.uuid == "all" || data.uuid == "fav" ||
                data.uuid == "play" || data.uuid == "musicCand" || data.uuid == "albumCand" || data.uuid == "artistCand" ||
                data.uuid == "musicResult" || data.uuid == "albumResult" || data.uuid == "artistResult" ||
                data.uuid == "search") {
            continue;
        }
        QString displayName = data.displayName;
        auto item = new DStandardItem(QIcon::fromTheme("music_famousballad"), displayName);
        auto itemFont = item->font();
        itemFont.setPixelSize(14);
        item->setFont(itemFont);

        item->setData(data.uuid, Qt::UserRole); //covert to hash
        if (m_type == 1) {
            item->setForeground(QColor("#414D68"));
        } else {
            item->setForeground(QColor("#C0C6D4"));
        }
        model->appendRow(item);
    }

    setMinimumHeight(model->rowCount() * 40);

    //add short cut
    m_newItemShortcut = new QShortcut(this);
    m_newItemShortcut->setKey(QKeySequence(QLatin1String("Ctrl+Shift+N")));
    connect(m_newItemShortcut, SIGNAL(activated()), this, SLOT(newPlayList()));
}

void MusicSongListView::newPlayList()
{
    //close editor
    for (int i = 0; i < model->rowCount(); i++) {
        auto item = model->index(i, 0);
        if (this->isPersistentEditorOpen(item))
            closePersistentEditor(item);
    }
    qDebug() << "new item";
    QIcon icon = QIcon::fromTheme("music_famousballad");

    QString displayName = newDisplayName(); //translation? from playlistmanager
    auto item = new DStandardItem(icon, displayName);
    auto itemFont = item->font();
    itemFont.setPixelSize(14);
    item->setFont(itemFont);
    if (m_type == 1) {
        item->setForeground(QColor("#414D68"));
    } else {
        item->setForeground(QColor("#C0C6D4"));
    }
    model->appendRow(item);
    setMinimumHeight(model->rowCount() * 40);
    setCurrentIndex(model->indexFromItem(item));
    edit(model->indexFromItem(item));
    scrollToBottom();

    //record to db
    DataBaseService::PlaylistData info;
    info.editmode = true;
    info.readonly = false;
    info.uuid = QUuid::createUuid().toString().remove("{").remove("}").remove("-");
    info.displayName = displayName;
    info.sortID = DataBaseService::getInstance()->getPlaylistMaxSortid();
    DataBaseService::getInstance()->addPlaylist(info);
    //切换listpage
    emit CommonService::getInstance()->switchToView(CustomType, info.uuid);
}

void MusicSongListView::slotUpdatePlayingIcon()
{
    for (int i = 0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        DStandardItem *item = dynamic_cast<DStandardItem *>(model->item(i, 0));
        if (item == nullptr) {
            continue;
        }
        QString hash = index.data(Qt::UserRole).value<QString>();
        if (hash == Player::instance()->getCurrentPlayListHash()) {
            QPixmap playingPixmap = QPixmap(QSize(20, 20));
            playingPixmap.fill(Qt::transparent);
            QPainter painter(&playingPixmap);
            DTK_NAMESPACE::Gui::DPalette pa;// = this->palette();
            if (selectedIndexes().size() > 0 && (selectedIndexes().at(0) == index)) {
                painter.setPen(QColor(Qt::white));
            } else {
                painter.setPen(pa.color(QPalette::Active, DTK_NAMESPACE::Gui::DPalette::Highlight));
            }
            Player::instance()->playingIcon().paint(&painter, QRect(0, 0, 20, 20), Qt::AlignCenter, QIcon::Active, QIcon::On);

            QIcon playingIcon(playingPixmap);
            DViewItemActionList actionList = item->actionList(Qt::RightEdge);
            if (!actionList.isEmpty()) {
                actionList.first()->setIcon(playingIcon);
            } else {
                DViewItemActionList  actionList;
                auto viewItemAction = new DViewItemAction(Qt::AlignCenter, QSize(20, 20));
                viewItemAction->setIcon(playingIcon);
                actionList.append(viewItemAction);
                dynamic_cast<DStandardItem *>(item)->setActionList(Qt::RightEdge, actionList);
            }
        } else {
            QIcon playingIcon;
            DViewItemActionList actionList = item->actionList(Qt::RightEdge);
            if (!actionList.isEmpty()) {
                actionList.first()->setIcon(playingIcon);
            } else {
                DViewItemActionList  actionList;
                auto viewItemAction = new DViewItemAction(Qt::AlignCenter, QSize(20, 20));
                viewItemAction->setIcon(playingIcon);
                actionList.append(viewItemAction);
                dynamic_cast<DStandardItem *>(item)->setActionList(Qt::RightEdge, actionList);
            }
        }
    }
    update();
}

void MusicSongListView::mousePressEvent(QMouseEvent *event)
{
    DListView::mousePressEvent(event);
}

void MusicSongListView::dragEnterEvent(QDragEnterEvent *event)
{
    auto t_formats = event->mimeData()->formats();
    qDebug() << t_formats;
    if (event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        qDebug() << "acceptProposedAction" << event;
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    }
}

void MusicSongListView::dragMoveEvent(QDragMoveEvent *event)
{
    auto index = indexAt(event->pos());
    if (index.isValid() && (event->mimeData()->hasFormat("text/uri-list")  || event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))) {
        qDebug() << "acceptProposedAction" << event;
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    } else {
        DListView::dragMoveEvent(event);
    }
}

void MusicSongListView::SetAttrRecur(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
{
    // if it has the tagname then overwritte desired attribute
    if (elem.tagName().compare(strtagname) == 0) {
        elem.setAttribute(strattr, strattrval);
    }
    // loop all children
    for (int i = 0; i < elem.childNodes().count(); i++) {
        if (!elem.childNodes().at(i).isElement()) {
            continue;
        }
        this->SetAttrRecur(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
    }
}

QString MusicSongListView::newDisplayName()
{
    QStringList existNames;
    for (DataBaseService::PlaylistData &playlistData : DataBaseService::getInstance()->allPlaylistMeta()) {
        existNames.append(playlistData.displayName);
    }

    QString temp = tr("New playlist");
    if (!existNames.contains(temp)) {
        return temp;
    }

    int i = 1;
    for (i = 1; i < existNames.size() + 1; ++i) {
        QString newName = QString("%1 %2").arg(temp).arg(i);
        if (!existNames.contains(newName)) {
            return newName;
        }
    }
    return QString("%1 %2").arg(temp).arg(i);
}

void MusicSongListView::slotTheme(int type)
{
    m_type = type;

    for (int i = 0; i < model->rowCount(); i++) {
        auto curIndex = model->index(i, 0);
        auto curStandardItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(curIndex));
        auto curItemRow = curStandardItem->row();
//        if (curItemRow < 0 || curItemRow >= allPlaylists.size())
//            continue;

        if (m_type == 1) {
            curStandardItem->setForeground(QColor("#414D68"));
        } else {
            curStandardItem->setForeground(QColor("#C0C6D4"));
        }
    }
}
