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

#include <QDir>
#include <QSqlDatabase>
#include <QThread>

#include "qxmpp/QXmppShareIq.h"

class QTimer;

class ChatSharesDatabase : public QObject
{
    Q_OBJECT

public:
    ChatSharesDatabase(const QString &path, QObject *parent = 0);
    QString locate(const QString &publishId);
    void search(const QXmppShareSearchIq &requestIq);

signals:
    void searchFinished(const QXmppShareSearchIq &packet);

private slots:
    void index();

private:
    QTimer *indexTimer;
    QSqlDatabase sharesDb;
    QDir sharesDir;
};

class IndexThread : public QThread
{
public:
    IndexThread(const QSqlDatabase &database, const QDir &dir, QObject *parent = 0);
    void run();

private:
    void scanDir(const QDir &dir);

    QHash<QString, int> scanOld;
    qint64 scanAdded;
    qint64 scanUpdated;
    QSqlDatabase sharesDb;
    QDir sharesDir;
};

class SearchThread : public QThread
{
    Q_OBJECT

public:
    SearchThread(const QSqlDatabase &database, const QDir &dir, const QXmppShareSearchIq &request, QObject *parent = 0);
    void run();

signals:
    void searchFinished(const QXmppShareSearchIq &packet);

private:
    void search(QXmppShareItem &rootCollection, const QString &basePrefix, const QString &queryString, int maxDepth);
    bool updateFile(QXmppShareItem &shareFile, const QSqlQuery &selectQuery, bool updateHash);

    QXmppShareSearchIq requestIq;
    QSqlDatabase sharesDb;
    QDir sharesDir;
};


