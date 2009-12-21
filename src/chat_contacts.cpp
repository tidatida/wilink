/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <QContextMenuEvent>
#include <QDebug>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_contacts.h"

RosterModel::RosterModel(QXmppRoster *roster)
    : modelRoster(roster)
{
    connect(modelRoster, SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rosterKeys.size())
        return QVariant();

    const QXmppRoster::QXmppRosterEntry &entry = modelRoster->getRosterEntry(rosterKeys.at(index.row()));
    if (role == Qt::DisplayRole)
    {
        QString name = entry.getName();
        if (name.isEmpty())
            name = entry.getBareJid().split("@").first();
        return name;
    }
    return QVariant();
}

void RosterModel::rosterReceived()
{
    rosterKeys = modelRoster->getRosterBareJids();
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    return rosterKeys.size();
}

ContactsList::ContactsList(QXmppRoster *roster, QXmppVCardManager *cardManager, QWidget *parent)
    : QListWidget(parent),
    showOffline(true),
    vcardManager(cardManager),
    xmppRoster(roster)
{
    QAction *action;
    contextMenu = new QMenu(this);
    action = contextMenu->addAction(QIcon(":/chat.png"), tr("Start chat"));
    connect(action, SIGNAL(triggered()), this, SLOT(startChat()));
    action = contextMenu->addAction(QIcon(":/remove.png"), tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

    /* connect to XMPP events */
    connect(xmppRoster, SIGNAL(rosterChanged(const QString&)), this, SLOT(rosterChanged(const QString&)));
    connect(xmppRoster, SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
    connect(vcardManager, SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));

    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(startChat()));
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setMinimumSize(QSize(140, 140));
}

void ContactsList::addEntry(const QXmppRoster::QXmppRosterEntry &entry)
{
    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setIcon(QIcon(":/contact-offline.png"));
    QString jid = entry.getBareJid();
    newItem->setData(Qt::UserRole, jid);
    QString name = entry.getName();
    if (name.isEmpty())
        name = jid.split("@")[0];
    newItem->setText(name);
    addItem(newItem);
    if (!showOffline)
        setItemHidden(newItem, true);
    vcardManager->requestVCard(jid);
}

void ContactsList::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

    contextMenu->popup(event->globalPos());
}

void ContactsList::presenceReceived(const QXmppPresence &presence)
{
    QString suffix;
    bool offline;
    switch (presence.getType())
    {
    case QXmppPresence::Available:
        switch(presence.getStatus().getType())
        {
            case QXmppPresence::Status::Online:
                suffix = "available";
                break;
            default:
                suffix = "busy";
                break;
        }
        offline = false;
        break;
    case QXmppPresence::Unavailable:
        suffix = "offline";
        offline = true;
        break;
    default:
        return;
    }
    QString bareJid = presence.getFrom().split("/").first();
    for (int i = 0; i < count(); i++)
    {
        QListWidgetItem *entry = item(i);
        if (entry->data(Qt::UserRole).toString() == bareJid)
        {
            entry->setIcon(QIcon(QString(":/contact-%1.png").arg(suffix)));
            bool hidden = !showOffline && offline;
            setItemHidden(entry, hidden);
            break;
        }
    }
}

void ContactsList::removeContact()
{
    QListWidgetItem *item = currentItem();
    if (!item)
        return;

    const QString jid = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        emit removeContact(jid);
    }
}

void ContactsList::rosterChanged(const QString &jid)
{
    QXmppRoster::QXmppRosterEntry entry = xmppRoster->getRosterEntry(jid);
    int itemIndex = -1;
    for (int i = 0; i < count(); i++)
    {
        QListWidgetItem *rosterItem = item(i);
        if (rosterItem->data(Qt::UserRole).toString() == jid)
        {
            itemIndex = i;
            break;
        }
    }
    switch (entry.getSubscriptionType())
    {
        case QXmppRosterIq::Item::Remove:
            if (itemIndex >= 0)
                takeItem(itemIndex);
            break;
        default:
            if (itemIndex < 0)
                addEntry(entry);
    }
}

void ContactsList::rosterReceived()
{
    QMap<QString, QXmppRoster::QXmppRosterEntry> entries = xmppRoster->getRosterEntries();
    clear();
    foreach (const QString &key, entries.keys())
        addEntry(entries[key]);
}

void ContactsList::setShowOffline(bool show)
{
    // FIXME: refresh list
    showOffline = show;
}

void ContactsList::startChat()
{
    QListWidgetItem *item = currentItem();
    if (!item)
        return;

    const QString jid = item->data(Qt::UserRole).toString();
    emit chatContact(jid);
}

void ContactsList::vCardReceived(const QXmppVCard& vcard)
{
    QImage image = vcard.getPhotoAsImage();
    const QString bareJid = vcard.getFrom().split("/").first();
    for (int i = 0; i < count(); i++)
    {
        QListWidgetItem *entry = item(i);
        if (entry->data(Qt::UserRole).toString() == bareJid)
        {
            entry->setIcon(QIcon(QPixmap::fromImage(image)));
            if(!vcard.getFullName().isEmpty())
                entry->setText(vcard.getFullName());
            break;
        }
    }
}


