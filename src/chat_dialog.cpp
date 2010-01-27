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

#include <QContextMenuEvent>
#include <QDebug>
#include <QEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextBrowser>

#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "chat_dialog.h"
#include "chat_edit.h"

ChatDialog::ChatDialog(const QString &jid, const QString &name, QWidget *parent)
    : QWidget(parent),
    chatRemoteJid(jid)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    /* status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *nameLabel = new QLabel(chatRemoteJid);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    avatarLabel = new QLabel;
    hbox->addWidget(avatarLabel);
    layout->addItem(hbox);

    /* chat history */
    chatHistory = new ChatHistory;
    chatHistory->setLocalName(tr("Me"));
    chatHistory->setRemoteName(name);
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

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
{
    foreach (const QXmppArchiveMessage &msg, chat.messages)
        chatHistory->addMessage(msg);

    /* scroll to end, but don't touch cursor */
    QScrollBar *scrollBar = chatHistory->verticalScrollBar();
    scrollBar->setSliderPosition(scrollBar->maximum());
}

void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    QXmppArchiveMessage message;
    message.body = msg.getBody();
    message.local = false;
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);
}

void ChatDialog::send()
{
    QString text = chatInput->document()->toPlainText();
    if (text.isEmpty())
        return;

    QXmppArchiveMessage message;
    message.body = text;
    message.local = true;
    message.datetime = QDateTime::currentDateTime();
    chatHistory->addMessage(message);

    /* scroll to end, but don't touch cursor */
    QScrollBar *scrollBar = chatHistory->verticalScrollBar();
    scrollBar->setSliderPosition(scrollBar->maximum());

    chatInput->document()->clear();
    emit sendMessage(chatRemoteJid, text);
}

void ChatDialog::setAvatar(const QPixmap &avatar)
{
    avatarLabel->setPixmap(avatar);
}

ChatHistory::ChatHistory(QWidget *parent)
    : QTextBrowser(parent)
{
    setOpenLinks(false);
    connect(this, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(slotAnchorClicked(const QUrl&)));
}

void ChatHistory::addMessage(const QXmppArchiveMessage &message)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    int i = messages.size() - 1;
    while (i >= 0 && message.datetime > messages.at(i).datetime)
    {
        cursor.movePosition(QTextCursor::PreviousBlock);
        i--;
    }

    /* add message */
    QString html = message.body;
    html.replace(QRegExp("((ftp|http|https)://[^ ]+)"), "<a href=\"\\1\">\\1</a>");
    html = QString(
        "<table cellspacing=\"0\" width=\"100%\">"
        "<tr style=\"background-color: %1\">"
        "  <td>%2</td>"
        "  <td align=\"right\">%3</td>"
        "</tr>"
        "<tr>"
        "  <td colspan=\"2\">%4</td>"
        "</tr>"
        "</table>")
        .arg(message.local ? "#dbdbdb" : "#b6d4ff")
        .arg(message.local ? chatLocalName : chatRemoteName)
        .arg(message.datetime.date() == QDate::currentDate() ? message.datetime.toString("hh:mm") : message.datetime.toString("dd MMM hh:mm"))
        .arg(html);
    cursor.insertHtml(html);
}

void ChatHistory::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    QAction *action = menu->addAction(tr("Clear"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(clear()));
    menu->exec(event->globalPos());
    delete menu;
}

void ChatHistory::setLocalName(const QString &localName)
{
    chatLocalName = localName;
}

void ChatHistory::setRemoteName(const QString &remoteName)
{
    chatRemoteName = remoteName;
}

void ChatHistory::slotAnchorClicked(const QUrl &link)
{
    QDesktopServices::openUrl(link);
}

