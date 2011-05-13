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

#include "QSoundPlayer.h"

#include "QXmppArchiveIq.h"
#include "QXmppArchiveManager.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppUtils.h"

#include "application.h"
#include "chat.h"
#include "chat_client.h"
#include "chat_edit.h"
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
    m_state(QXmppMessage::None)
{
}

ChatClient *ChatDialogHelper::client() const
{
    return m_client;
}

void ChatDialogHelper::setClient(ChatClient *client)
{
    if (client != m_client) {
        m_client = client;
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

bool ChatDialogHelper::sendMessage(const QString &body)
{
    if (m_jid.isEmpty() || !m_client || !m_client->isConnected())
        return false;

    // send message
    QXmppMessage message;
    message.setTo(m_jid);
    message.setBody(body);
    message.setState(m_state);
    if (!m_client->sendPacket(message))
        return false;

    // add message to history
    if (m_historyModel) {
        ChatMessage message;
        message.body = body;
        message.date = m_client->serverTime();
        message.jid = m_client->configuration().jid();
        message.received = false;
        m_historyModel->addMessage(message);
    }

    // play sound
    wApp->soundPlayer()->play(wApp->outgoingMessageSound());
    return true;
}

ChatDialog::ChatDialog(ChatClient *xmppClient, ChatRosterModel *chatRosterModel, const QString &jid, QWidget *parent)
    : ChatPanel(parent),
    chatRemoteJid(jid), 
    client(xmppClient),
    joined(false),
    rosterModel(chatRosterModel)
{
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
    historyModel->setRosterModel(rosterModel);

    ChatDialogHelper *helper = new ChatDialogHelper(this);
    helper->setClient(client);
    helper->setJid(jid);
    helper->setHistoryModel(historyModel);

    // header
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setSpacing(0);
    layout->addLayout(headerLayout());

    // chat history
    ChatRosterImageProvider *imageProvider = new ChatRosterImageProvider;
    imageProvider->setRosterModel(rosterModel);

    historyView = new QDeclarativeView;
    QDeclarativeContext *context = historyView->rootContext();
    context->setContextProperty("conversation", helper);
    context->setContextProperty("historyModel", historyModel);
    context->setContextProperty("participantModel", QVariant());
    historyView->engine()->addImageProvider("roster", imageProvider);
    historyView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    historyView->setSource(QUrl("qrc:/conversation.qml"));

    QObject *item = historyView->rootObject()->findChild<QObject*>("historyView");
    Q_ASSERT(item);
    check = connect(historyModel, SIGNAL(bottomChanged()),
                    item, SLOT(onBottomChanged()));

    layout->addWidget(historyView);

    // connect signals
#if 0
    check = connect(chatInput(), SIGNAL(chatStateChanged(int)),
                    this, SLOT(chatStateChanged(int)));
    Q_ASSERT(check);
#endif

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

    check = connect(rosterModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                    this, SLOT(rosterChanged(QModelIndex,QModelIndex)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
                    this, SLOT(leave()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(showPanel()),
                    this, SLOT(join()));
    Q_ASSERT(check);

    // set window title
    updateWindowTitle();
}

void ChatDialog::archiveChatReceived(const QXmppArchiveChat &chat)
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

void ChatDialog::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with()) == chatRemoteJid)
            archiveManager->retrieveCollection(chats[i].with(), chats[i].start());
}

/** When the chat state changes, notify the remote party.
 */
void ChatDialog::chatStateChanged(int state)
{
    foreach (const QString &jid, chatStatesJids) {
        QXmppMessage message;
        message.setTo(jid);
        message.setState(static_cast<QXmppMessage::State>(state));
        client->sendPacket(message);
    }
}

QDeclarativeView* ChatDialog::declarativeView() const
{
    return historyView;
}

void ChatDialog::disconnected()
{
    // FIXME : we should re-join on connect
    // joined = false;
}

/** Leave a two party dialog.
 */
void ChatDialog::leave()
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
void ChatDialog::join()
{
    if (joined)
        return;

    // send initial state
    chatStatesJids = rosterModel->contactFeaturing(chatRemoteJid, ChatRosterModel::ChatStatesFeature);
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
void ChatDialog::messageReceived(const QXmppMessage &msg)
{
    if (msg.type() != QXmppMessage::Chat ||
        jidToBareJid(msg.from()) != chatRemoteJid)
        return;

    // handle chat state
    QString stateName;
    if (msg.state() == QXmppMessage::Composing)
        stateName = tr("is composing a message");
    else if (msg.state() == QXmppMessage::Gone)
        stateName = tr("has closed the conversation");
    setWindowStatus(stateName);

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

/** Handles a roster change.
 *
 * @param topLeft
 * @param bottomRight
 */
void ChatDialog::rosterChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_ASSERT(topLeft.parent() == bottomRight.parent());
    const QModelIndex parent = topLeft.parent();
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        const QString jid = rosterModel->index(i, 0, parent).data(ChatRosterModel::IdRole).toString();
        if (jid == chatRemoteJid) {
            updateWindowTitle();
            break;
        }
    }
}

/** Updates the window title.
 */
void ChatDialog::updateWindowTitle()
{
    QModelIndex index = rosterModel->findItem(chatRemoteJid);
    setWindowTitle(index.data(Qt::DisplayRole).toString());
    setWindowIcon(index.data(Qt::DecorationRole).value<QIcon>());

    const QString remoteDomain = jidToDomain(chatRemoteJid);
    if (client->configuration().domain() == "wifirst.net" &&
        remoteDomain == "wifirst.net")
    {
        // for wifirst accounts, return the wifirst nickname if it is
        // different from the display name
        const QString nickName = index.data(ChatRosterModel::NicknameRole).toString();
        if (nickName != windowTitle())
            setWindowExtra(nickName);
        else
            setWindowExtra(QString());
    } else {
        // for other accounts, return the JID
        setWindowExtra(chatRemoteJid);
    }
}

/** Constructs a new ChatsWatcher, an observer which catches incoming messages
 *  and clicks on the roster and opens conversations as appropriate.
 *
 * @param chatWindow
 */
ChatsWatcher::ChatsWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    connect(chat->client(), SIGNAL(messageReceived(QXmppMessage)),
            this, SLOT(messageReceived(QXmppMessage)));

    // add roster hooks
    connect(chat, SIGNAL(rosterClick(QModelIndex)),
            this, SLOT(rosterClick(QModelIndex)));
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
        ChatDialog *dialog = new ChatDialog(chat->client(), chat->rosterModel(), bareJid);
        chat->addPanel(dialog);
        dialog->messageReceived(msg);
    }
}

/** When the user clicks on a contact in his roster, open a conversation.
 *
 * @param index The roster entry that was clicked.
 */
void ChatsWatcher::rosterClick(const QModelIndex &index)
{
    if (!chat->client()->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // create conversation if necessary
    if (type == ChatRosterModel::Contact && !chat->panel(jid))
        chat->addPanel(new ChatDialog(chat->client(), chat->rosterModel(), jid));
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

