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

#include "musicbaselistview.h"

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

#include "commonservice.h"
#include "musicbaseandsonglistmodel.h"
#include "mediameta.h"
#include "player.h"

DGUI_USE_NAMESPACE

MusicBaseListView::MusicBaseListView(QWidget *parent) : DListView(parent)
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

//    connect(this, &MusicBaseListView::triggerEdit,
//    this, [ = ](const QModelIndex & index) {
//        if (DGuiApplicationHelper::instance()->themeType() == 1) {
//            auto curStandardItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(index));
//            curStandardItem->setIcon(QIcon(QString(":/mpimage/light/normal/famous_ballad_normal.svg")));
//        }
//    });
    init();
    connect(this, &MusicBaseListView::clicked, this, [](QModelIndex midx) {
        ListPageSwitchType type = midx.data(Qt::UserRole).value<ListPageSwitchType>();
        emit CommonService::getInstance()->switchToView(type, "");
    });

    connect(Player::instance(), SIGNAL(signalUpdatePlayingIcon()),
            this, SLOT(slotUpdatePlayingIcon()), Qt::DirectConnection);
}

MusicBaseListView::~MusicBaseListView()
{

}

void SetSVGBackColor1(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
{
    if (elem.tagName().compare(strtagname) == 0) {
        QString before_color = elem.attribute(strattr);
        elem.setAttribute(strattr, strattrval);
        QString color = elem.attribute(strattr);
    }
    for (int i = 0; i < elem.childNodes().count(); i++) {
        if (!elem.childNodes().at(i).isElement()) {
            continue;
        }
        SetSVGBackColor1(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
    }
}

void MusicBaseListView::init()
{
    QString rStr;
    if (m_type == 1) {
        rStr = "light";
    } else {
        rStr = "dark";
    }

    QString displayName = tr("Albums");
    auto item = new DStandardItem(QIcon::fromTheme("music_album"), displayName);
    auto itemFont = item->font();
    itemFont.setPixelSize(14);
    item->setFont(itemFont);
    if (m_type == 1) {
        item->setForeground(QColor("#414D68"));
    } else {
        item->setForeground(QColor("#C0C6D4"));
    }
    item->setData(ListPageSwitchType::AlbumType, Qt::UserRole);
    item->setData("album", Qt::UserRole + 2);
    model->appendRow(item);

    displayName = tr("Artist");
    item = new DStandardItem(QIcon::fromTheme("music_singer"), displayName);
    item->setData(ListPageSwitchType::SingerType, Qt::UserRole);
    item->setData("artist", Qt::UserRole + 2);
    model->appendRow(item);

    displayName = tr("All Music");
    item = new DStandardItem(QIcon::fromTheme("music_allmusic"), displayName);
    item->setData(ListPageSwitchType::AllSongListType, Qt::UserRole);
    item->setData("all", Qt::UserRole + 2);
    model->appendRow(item);

    displayName = tr("My favorites");
    item = new DStandardItem(QIcon::fromTheme("music_mycollection"), displayName);
    item->setData(ListPageSwitchType::FavType, Qt::UserRole);
    item->setData("fav", Qt::UserRole + 2);
    model->appendRow(item);

    setMinimumHeight(model->rowCount() * 40);
}

void MusicBaseListView::mousePressEvent(QMouseEvent *event)
{
    DListView::mousePressEvent(event);
}

void MusicBaseListView::dragEnterEvent(QDragEnterEvent *event)
{
    auto t_formats = event->mimeData()->formats();
    qDebug() << t_formats;
    if (event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        qDebug() << "acceptProposedAction" << event;
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    }
}

void MusicBaseListView::dragMoveEvent(QDragMoveEvent *event)
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

void MusicBaseListView::SetAttrRecur(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
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

void MusicBaseListView::slotUpdatePlayingIcon()
{
    for (int i = 0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        DStandardItem *item = dynamic_cast<DStandardItem *>(model->item(i, 0));
        if (item == nullptr) {
            continue;
        }
        QString hash = index.data(Qt::UserRole + 2).value<QString>();
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

void MusicBaseListView::slotTheme(int type)
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
