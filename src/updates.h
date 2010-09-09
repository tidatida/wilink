/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UPDATES_H__
#define __UPDATES_H__

#include <QFile>
#include <QMap>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class Release
{
public:
    bool checkHashes(const QByteArray &data) const;
    bool isValid() const;

    QString changes;
    QString package;
    QUrl url;
    QString version;
    QMap<QString, QByteArray> hashes;
};

/** The Updates class handling checking for software updates
 *  and installing them.
 */
class Updates : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        BadHash,
        DownloadFailed,
        InsecureLocation,
        SaveFailed,
    } UpdatesError;

    Updates(QObject *parent);
    void download(const Release &release);
    void install(const QUrl &url);

    QString cacheDirectory() const;
    void setCacheDirectory(const QString &cacheDir);

    static int compareVersions(const QString &v1, const QString v2);

public slots:
    void check();

signals:
    void checkStarted();
    void checkFinished(const Release &release, const QString &errorString);
    void downloadStarted();
    void downloadProgress(qint64 done, qint64 total);
    void updateDownloaded(const QUrl &url);
    void updateFailed(Updates::UpdatesError error, const QString &errorString);

private slots:
    void saveUpdate();
    void processStatus();

private:
    QNetworkAccessManager *network;
    QTimer *timer;
    QFile downloadFile;
    Release downloadRelease;

    QString m_cacheDirectory;
    const QUrl m_updatesUrl;
};

#endif
