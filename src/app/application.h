/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <QApplication>
#include <QString>
#include <QUrl>

#ifdef USE_SYSTRAY
#include <QSystemTrayIcon>
#endif

class QSoundPlayer;

class ApplicationPrivate;
class ApplicationSettings;
class Notification;

class Application : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(QString organizationName READ organizationName CONSTANT)
    Q_PROPERTY(QString osType READ osType CONSTANT)
    Q_PROPERTY(bool isMobile READ isMobile CONSTANT)
    Q_PROPERTY(QSoundPlayer* soundPlayer READ soundPlayer CONSTANT)

public:
    Application(int &argc, char **argv);
    ~Application();

    static void alert(QWidget *widget);
    static void platformInit();
    void createSystemTrayIcon();
    QSoundPlayer *soundPlayer();

    bool isMobile() const;
    QString osType() const;

signals:
    void showWindows();

public slots:
    QUrl resolvedUrl(const QUrl &url, const QUrl &base);
    Notification *showMessage(const QString &title, const QString &message, const QString &action);

private slots:
#ifdef USE_SYSTRAY
    void trayActivated(QSystemTrayIcon::ActivationReason reason);
    void trayClicked();
#endif

private:
    ApplicationPrivate * const d;
};

extern Application *wApp;

#endif
