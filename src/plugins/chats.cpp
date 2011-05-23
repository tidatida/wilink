/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>

#include "QSoundPlayer.h"

#include "QXmppArchiveIq.h"
#include "QXmppArchiveManager.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppUtils.h"

#include "application.h"
#include "chat.h"
#include "chat_client.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "chats.h"

#ifdef WILINK_EMBEDDED
#define HISTORY_DAYS 7
#else
#define HISTORY_DAYS 14
#endif

ChatDialogHelper::ChatDialogHelper(QObject *parent)
    : QObject(parent),
    m_client(0),
    m_historyModel(0),
    m_localState(QXmppMessage::None),
    m_remoteState(QXmppMessage::None)
{
}

ChatClient *ChatDialogHelper::client() const
{
    return m_client;
}

void ChatDialogHelper::setClient(ChatClient *client)
{
    bool check;

    if (client != m_client) {
        m_client = client;

        check = connect(client, SIGNAL(messageReceived(QXmppMessage)),
                        this, SLOT(messageReceived(QXmppMessage)));
        Q_ASSERT(check);

        emit clientChanged(client);
    }
}
ChatHistoryModel *ChatDialogHelper::historyModel() const
{
    return m_historyModel;
}

void ChatDialogHelper::setHistoryModel(ChatHistoryModel *historyModel)
{
    if (historyModel != m_historyModel) {
        m_historyModel = historyModel;
        emit historyModelChanged(historyModel);
    }
}

QString ChatDialogHelper::jid() const
{
    return m_jid;
}

void ChatDialogHelper::setJid(const QString &jid)
{
    if (jid != m_jid) {
        m_jid = jid;
        emit jidChanged(jid);
    }
}

int ChatDialogHelper::localState() const
{
    return m_localState;
}

void ChatDialogHelper::setLocalState(int state)
{
    if (state != m_localState) {
        m_localState = static_cast<QXmppMessage::State>(state);

        // notify state change
        QXmppMessage message;
        message.setTo(m_jid);
        message.setState(m_localState);
        m_client->sendPacket(message);

        emit localStateChanged(m_localState);
    }
}

int ChatDialogHelper::remoteState() const
{
    return m_remoteState;
}

void ChatDialogHelper::messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Chat ||
        jidToBareJid(msg.from()) != m_jid)
        return;

    // handle chat state
    if (msg.state() != m_remoteState) {
        m_remoteState = msg.state();
        emit remoteStateChanged(m_remoteState);
    }

    // handle message body
    if (msg.body().isEmpty())
        return;
}

bool ChatDialogHelper::sendMessage(const QString &body)
{
    if (m_jid.isEmpty() || !m_client || !m_client->isConnected())
        return false;

    // send message
    QXmppMessage message;
    message.setTo(m_jid);
    message.setBody(body);
    message.setState(QXmppMessage::Active);
    if (!m_client->sendPacket(message))
        return false;

    // update state
    if (m_localState != QXmppMessage::Active) {
        m_localState = QXmppMessage::Active;
        emit localStateChanged(m_localState);
    }

    // add message to history
    if (m_historyModel) {
        ChatMessage message;
        message.body = body;
        message.date = m_client->serverTime();
        message.jid = m_client->configuration().jidBare();
        message.received = false;
        m_historyModel->addMessage(message);
    }

    // play sound
    wApp->soundPlayer()->play(wApp->outgoingMessageSound());
    return true;
}

ChatDialogPanel::ChatDialogPanel(Chat *chatWindow, const QString &jid)
    : ChatPanel(chatWindow),
    chatRemoteJid(jid), 
    joined(false),
    m_window(chatWindow)
{
    client = chatWindow->client();

    bool check;
    setObjectName(jid);

    // load archive manager
    archiveManager = client->findExtension<QXmppArchiveManager>();
    if (!archiveManager) {
        archiveManager = new QXmppArchiveManager;
        client->addExtension(archiveManager);
    }

    // prepare models
    historyModel = new ChatHistoryModel(this);
    historyModel->setParticipantModel(m_window->rosterModel());

    ChatDialogHelper *helper = new ChatDialogHelper(this);
    helper->setClient(client);
    helper->setJid(jid);
    helper->setHistoryModel(historyModel);

    // header
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // chat history
    historyView = new QDeclarativeView;
    QDeclarativeContext *context = historyView->rootContext();
    context->setContextProperty("client", m_window->client());
    context->setContextProperty("conversation", helper);
    context->setContextProperty("historyModel", historyModel);
    context->setContextProperty("window", m_window);
    historyView->engine()->addImageProvider("roster", new ChatRosterImageProvider);
    historyView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    historyView->setSource(QUrl("qrc:/ConversationPanel.qml"));
    layout->addWidget(historyView);
    setFocusProxy(historyView);

    // connect signals
    check = connect(historyView->rootObject(), SIGNAL(close()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(connected()),
                    this, SLOT(join()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(messageReceived(QXmppMessage)),
                    this, SLOT(messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    check = connect(archiveManager, SIGNAL(archiveChatReceived(QXmppArchiveChat)),
                    this, SLOT(archiveChatReceived(QXmppArchiveChat)));
    Q_ASSERT(check);

    check = connect(archiveManager, SIGNAL(archiveListReceived(QList<QXmppArchiveChat>)),
                    this, SLOT(archiveListReceived(QList<QXmppArchiveChat>)));
    Q_ASSERT(check);

    check = connect(m_window->rosterModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                    this, SLOT(rosterChanged(QModelIndex,QModelIndex)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
                    this, SLOT(leave()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(showPanel()),
                    this, SLOT(join()));
    Q_ASSERT(check);
}

void ChatDialogPanel::archiveChatReceived(const QXmppArchiveChat &chat)
{
    if (jidToBareJid(chat.with()) != chatRemoteJid)
        return;

    foreach (const QXmppArchiveMessage &msg, chat.messages())
    {
        ChatMessage message;
        message.archived = true;
        message.body = msg.body();
        message.date = msg.date();
        message.jid = msg.isReceived() ? chatRemoteJid : client->configuration().jidBare();
        message.received = msg.isReceived();
        historyModel->addMessage(message);
    }
}

void ChatDialogPanel::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with()) == chatRemoteJid)
            archiveManager->retrieveCollection(chats[i].with(), chats[i].start());
}

/** When the chat state changes, notify the remote party.
 */
void ChatDialogPanel::chatStateChanged(int state)
{
    foreach (const QString &jid, chatStatesJids) {
        QXmppMessage message;
        message.setTo(jid);
        message.setState(static_cast<QXmppMessage::State>(state));
        client->sendPacket(message);
    }
}

QDeclarativeView* ChatDialogPanel::declarativeView() const
{
    return historyView;
}

void ChatDialogPanel::disconnected()
{
    // FIXME : we should re-join on connect
    // joined = false;
}

/** Leave a two party dialog.
 */
void ChatDialogPanel::leave()
{
    if (joined)
    {
        chatStateChanged(QXmppMessage::Gone);
        joined = false;
    }
    deleteLater();
}

/** Start a two party dialog.
 */
void ChatDialogPanel::join()
{
    if (joined)
        return;

    // send initial state
    chatStatesJids = m_window->rosterModel()->contactFeaturing(chatRemoteJid, ChatRosterModel::ChatStatesFeature);
    //chatStateChanged(chatInput()->state());

    // FIXME : we need to check whether archives are supported
    // to clear the display appropriately
    archiveManager->listCollections(chatRemoteJid,
        client->serverTime().addDays(-HISTORY_DAYS));

    joined = true;
}

/** Handles an incoming chat message.
 *
 * @param msg The received message.
 */
void ChatDialogPanel::messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Chat ||
        jidToBareJid(msg.from()) != chatRemoteJid)
        return;

    // handle message body
    if (msg.body().isEmpty())
        return;

    ChatMessage message;
    message.body = msg.body();
    message.date = msg.stamp();
    if (!message.date.isValid())
        message.date = client->serverTime();
    message.jid = chatRemoteJid;
    message.received = true;
    historyModel->addMessage(message);

    // queue notification
    queueNotification(message.body);

    // play sound
    wApp->soundPlayer()->play(wApp->incomingMessageSound());
}

/** Constructs a new ChatsWatcher, an observer which catches incoming messages
 *  and clicks on the roster and opens conversations as appropriate.
 *
 * @param chatWindow
 */
ChatsWatcher::ChatsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    bool check;

    check = connect(chat->client(), SIGNAL(messageReceived(QXmppMessage)),
                    this, SLOT(messageReceived(QXmppMessage)));
    Q_ASSERT(check);

    // add roster hooks
    check = connect(chat, SIGNAL(urlClick(QUrl)),
                    this, SLOT(urlClick(QUrl)));
    Q_ASSERT(check);
}

/** When a chat message is received, if we do not have an open conversation
 *  with the sender, create one.
 *
 * @param msg The received message.
 */
void ChatsWatcher::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.from());

    if (msg.type() == QXmppMessage::Chat && !chat->panel(bareJid) && !msg.body().isEmpty())
    {
        ChatDialogPanel *dialog = new ChatDialogPanel(chat, bareJid);
        chat->addPanel(dialog);
        dialog->messageReceived(msg);
    }
}

/** Open a XMPP URI if it refers to a conversation.
 */
void ChatsWatcher::urlClick(const QUrl &url)
{
    if (url.scheme() == "xmpp" && url.hasQueryItem("message")) {
        QString jid = url.path();
        if (jid.startsWith("/"))
            jid.remove(0, 1);
        ChatPanel *panel = chat->panel(jid);
        if (!panel) {
            panel = new ChatDialogPanel(chat, jid);
            chat->addPanel(panel);
        }
        QTimer::singleShot(0, panel, SIGNAL(showPanel()));
    }
}

// PLUGIN

class ChatsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Person-to-person chat"; };
};

bool ChatsPlugin::initialize(Chat *chat)
{
    new ChatsWatcher(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(chats, ChatsPlugin)

