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

#include "QXmppDiscoveryIq.h"
#include "QXmppLogger.h"
#include "QXmppTransferManager.h"

#include "chat_client.h"

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent)
{
    connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(this, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)),
        this, SLOT(slotDiscoveryIqReceived(const QXmppDiscoveryIq&)));
}

void ChatClient::slotConnected()
{
    /* discover services */
    QXmppDiscoveryIq disco;
    disco.setTo(configuration().domain());
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    discoQueue.clear();
    discoQueue.append(disco.id());
    sendPacket(disco);
}

void ChatClient::slotDiscoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (!discoQueue.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    if (disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == configuration().domain())
    {
        // root items
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            if (!item.jid().isEmpty() && item.node().isEmpty())
            {
                // get info for item
                QXmppDiscoveryIq info;
                info.setQueryType(QXmppDiscoveryIq::InfoQuery);
                info.setTo(item.jid());
                discoQueue.append(info.id());
                sendPacket(info);
            }
        }
    }
    else if (disco.queryType() == QXmppDiscoveryIq::InfoQuery)
    {
        // check if it's a conference server
        foreach (const QXmppDiscoveryIq::Identity &id, disco.identities())
        {
            if (id.category() == "conference" &&
                id.type() == "text")
            {
                emit logMessage(QXmppLogger::InformationMessage, "Found chat room server " + disco.from());
                emit mucServerFound(disco.from());
            }
            else if (id.category() == "proxy" &&
                     id.type() == "bytestreams")
            {
                emit logMessage(QXmppLogger::InformationMessage, "Found bytestream proxy " + disco.from());
                transferManager().setProxy(disco.from());
            }
            else if (id.category() == "store" &&
                     id.type() == "file")
            {
                emit logMessage(QXmppLogger::InformationMessage, "Found share server " + disco.from());
                emit shareServerFound(disco.from());
            }
        }
    }
}

