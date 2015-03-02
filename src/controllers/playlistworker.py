#!/usr/bin/python
# -*- coding: utf-8 -*-


import os
import sys
from PyQt5.QtCore import (QObject, pyqtSignal, pyqtSlot, 
    pyqtProperty, QUrl, QFile, QIODevice
    )
from PyQt5.QtMultimedia import QMediaPlaylist, QMediaContent
from .utils import registerContext, contexts
from config.constants import PlaylistPath


class PlaylistWorker(QObject):

    __contextName__ = 'PlaylistWorker'

    nameExisted = pyqtSignal(str)

    @registerContext
    def __init__(self, parent=None):
        super(PlaylistWorker, self).__init__(parent)

        self._playlists = {}
        self._currentPlaylist = None
        self._playlists['temporary'] = QMediaPlaylist()

        self.initPlaylist()


    def initPlaylist(self):
        self.addMediaToTemporary(os.sep.join([os.path.dirname(os.getcwd()), 'music', '1.mp3']))
        self.addMediaToTemporary(os.sep.join(['/usr/share/deepin-sample-music/邓入比_我们的情歌.mp3']))
        self.addMediaToTemporary(os.sep.join(['/usr/share/deepin-sample-music/郭一凡_说走就走的旅行.mp3']))
        self.addMediaToTemporary(os.sep.join(['/usr/share/deepin-sample-music/胡彦斌_依然是你.mp3']))
        self.savePlaylistByName('temporary')

        self.setPlaylistByName('temporary')
        self.currentPlaylist.setCurrentIndex(0)

    def savePlaylistByName(self, name):
        f = QFile(os.sep.join([PlaylistPath, '%s.m3u' % name]))
        flag = f.open(QIODevice.ReadWrite)
        if flag:
            self._playlists[name].save(f, 'm3u')
            f.close()

    @pyqtSlot('QString')
    def setPlaylistByName(self, name):
        mediaPlayer = contexts['MediaPlayer']
        playlist = self.getPlaylistByName(name)
        if playlist:
            mediaPlayer.setPlaylist(playlist)
            self._currentPlaylist = playlist

    @pyqtProperty('QMediaPlaylist')
    def currentPlaylist(self):
        return self._currentPlaylist


    def getPlaylistByName(self, name):
        if name in self._playlists:
            return self._playlists[name]
        else:
            return None

    @pyqtProperty(dict)
    def playlists(self):
        return self._playlists

    @pyqtSlot('QString')
    def addMediaToTemporary(self, url):
        if url.startswith('http://') or url.startswith('https://'):
            _url = QUrl(url)
        else:
            _url = QUrl.fromLocalFile(url)

        self._playlists['temporary'].addMedia(QMediaContent(_url))

    @pyqtSlot('QString', 'QString')
    def addMediaByName(self, name, url):
        if name in self._playlists:
            self._playlists[name].addMedia(QMediaContent(QUrl(unicode(url))))
        else:
            print("the playlist named %s isn't existed" % name)

    @pyqtSlot('QString')
    def createPlaylistByName(self, name):
        if name in self._playlists:
            self.nameExisted.emit(name)
        else:
            self._playlists[name] = QMediaPlaylist()

    @pyqtSlot()
    def previous(self):
        if self._currentPlaylist:
            self._currentPlaylist.previous()

    @pyqtSlot()
    def next(self):
        if self._currentPlaylist:
            self._currentPlaylist.next()
