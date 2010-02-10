/*
 * wDesktop
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

#include <QLabel>
#include <QLayout>

#include "qxmpp/QXmppMessage.h"

#include "chat_edit.h"
#include "chat_history.h"
#include "chat_room.h"

ChatRoom::ChatRoom(const QString &jid, QWidget *parent)
    : QWidget(parent), chatRemoteJid(jid)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    /* status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    nameLabel = new QLabel(chatRemoteJid);
    hbox->addSpacing(16);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    layout->addItem(hbox);

    /* chat history */
    chatHistory = new ChatHistory;
    layout->addWidget(chatHistory);

    /* text edit */
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(send()));
    layout->addSpacing(10);
    layout->addWidget(chatInput);

    setFocusProxy(chatInput);
    setLayout(layout);
    setMinimumWidth(300);
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QStringList bits = msg.getFrom().split("/");
    const QString from = bits.size() > 1 ? bits[1] : bits[0].split('@')[0];

    ChatHistoryMessage message;
    message.body = msg.getBody();
    message.from = from;
    message.local = false;
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);
}

void ChatRoom::send()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    chatInput->document()->clear();

    QXmppMessage msg;
    msg.setBody(text);
    msg.setTo(chatRemoteJid);
    msg.setType(QXmppMessage::GroupChat);
    emit sendMessage(msg);
}

