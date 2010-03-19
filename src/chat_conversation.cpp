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

#include <QDateTime>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>

#include "chat_conversation.h"
#include "chat_edit.h"
#include "chat_history.h"

ChatConversation::ChatConversation(const QString &jid, QWidget *parent)
    : ChatPanel(parent),
    chatRemoteJid(jid), chatLocalState(QXmppMessage::None)
{
    setWindowTitle(chatRemoteJid);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    /* status bar */
    layout->addItem(statusBar());

    /* chat history */
    chatHistory = new ChatHistory;
    connect(chatHistory, SIGNAL(focused()), this, SLOT(slotFocused()));
    layout->addWidget(chatHistory);

    /* text edit */
    chatInput = new ChatEdit(80);
    connect(chatInput, SIGNAL(focused()), this, SLOT(slotFocused()));
    connect(chatInput, SIGNAL(returnPressed()), this, SLOT(slotSend()));
    connect(chatInput, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
    layout->addSpacing(10);
    layout->addWidget(chatInput);

    setFocusProxy(chatInput);
    setLayout(layout);
    setMinimumWidth(300);

    /* timers */
    pausedTimer = new QTimer(this);
    pausedTimer->setInterval(30000);
    pausedTimer->setSingleShot(true);
    connect(pausedTimer, SIGNAL(timeout()), this, SLOT(slotPaused()));

    inactiveTimer = new QTimer(this);
    inactiveTimer->setInterval(120000);
    inactiveTimer->setSingleShot(true);
    connect(inactiveTimer, SIGNAL(timeout()), this, SLOT(slotInactive()));

    /* shortcuts for new line */
    connect(new QShortcut(QKeySequence(Qt::AltModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));
    connect(new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Return), this),
        SIGNAL(activated()), this, SLOT(slotNewLine()));

    /* start inactivity timer */
    inactiveTimer->start();
    setRemoteState(QXmppMessage::None);
}

void ChatConversation::join()
{
}

void ChatConversation::leave()
{
    chatLocalState = QXmppMessage::Gone;
    emit localStateChanged(chatLocalState);
}

QXmppMessage::State ChatConversation::localState() const
{
    return chatLocalState;
}

void ChatConversation::setLocalName(const QString &name)
{
    chatLocalName = name;
}

void ChatConversation::setRemoteName(const QString &name)
{
    chatRemoteName = name;
    setWindowTitle(name);
}

void ChatConversation::setRemoteState(QXmppMessage::State state)
{
    QString stateName;
    if (state == QXmppMessage::Composing)
        stateName = tr("is composing a message");
    else if (state == QXmppMessage::Gone)
        stateName = tr("has closed the conversation");

    if (!stateName.isEmpty())
        stateName = QString(" %1").arg(stateName);

    setWindowExtra(QString("%1<br/>%2")
        .arg(stateName)
        .arg(chatRemoteJid));
}

void ChatConversation::slotFocused()
{
    if (chatLocalState != QXmppMessage::Active &&
        chatLocalState != QXmppMessage::Composing &&
        chatLocalState != QXmppMessage::Paused)
    {
        chatLocalState = QXmppMessage::Active;
        emit localStateChanged(chatLocalState);
    }
    // reset inactivity timer
    inactiveTimer->stop();
    inactiveTimer->start();
}

void ChatConversation::slotInactive()
{
    if (chatLocalState != QXmppMessage::Inactive)
    {
        chatLocalState = QXmppMessage::Inactive;
        emit localStateChanged(chatLocalState);
    }
}

void ChatConversation::slotNewLine()
{
    chatInput->append("");
}

void ChatConversation::slotPaused()
{
    if (chatLocalState == QXmppMessage::Composing)
    {
        chatLocalState = QXmppMessage::Paused;
        emit localStateChanged(chatLocalState);
    }
}

void ChatConversation::slotSend()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    chatLocalState = QXmppMessage::Active;
    chatInput->document()->clear();
    sendMessage(text);
}

void ChatConversation::slotTextChanged()
{
    QString text = chatInput->document()->toPlainText();
    if (!text.isEmpty())
    {
        if (chatLocalState != QXmppMessage::Composing)
        {
            chatLocalState = QXmppMessage::Composing;
            emit localStateChanged(chatLocalState);
        }
        pausedTimer->start();
    } else {
        if (chatLocalState != QXmppMessage::Active)
        {
            chatLocalState = QXmppMessage::Active;
            emit localStateChanged(chatLocalState);
        }
        pausedTimer->stop();
    }
    // reset inactivity timer
    inactiveTimer->stop();
    inactiveTimer->start();
}

