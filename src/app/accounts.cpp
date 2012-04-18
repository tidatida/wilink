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

#include <QSet>

#include "application.h"
#include "accounts.h"
#include "wallet.h"

#include "QXmppUtils.h"

static QSet<AccountModel*> globalInstances;

class AccountItem : public ChatModelItem
{
public:
    AccountItem(const QString &jid);
    QString type() const;
    QString username() const;

    const QString jid;
    QString changedPassword;
};

static QString realm(const QString &jid)
{
    const QString domain = jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("www.wifirst.net");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("www.google.com");
    else
        return domain;
}

AccountItem::AccountItem(const QString &jid_)
    : jid(jid_)
{
}

QString AccountItem::type() const
{
    const QString domain = jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("wifirst");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("google");
    else
        return domain;
}

QString AccountItem::username() const
{
    return jid;
}

AccountModel::AccountModel(QObject *parent)
    : ChatModel(parent)
{
    // set additionals role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(UsernameRole, "username");
    names.insert(PasswordRole, "password");
    names.insert(TypeRole, "type");
    setRoleNames(names);

    // load accounts
    _q_reload();

    globalInstances.insert(this);
}

AccountModel::~AccountModel()
{
    globalInstances.remove(this);
}

void AccountModel::append(const QVariantMap &obj)
{
    const QString jid = obj.value("jid").toString();
    const QString password = obj.value("password").toString();
    if (jid.isEmpty() || password.isEmpty()) {
        qWarning("JID and password are required to add an account");
        return;
    }

    AccountItem *item = new AccountItem(jid);
    item->changedPassword = password;
    addItem(item, rootItem);
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
    AccountItem *item = static_cast<AccountItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == JidRole) {
        return item->jid;
    } else if (role == PasswordRole) {
        return getPassword(item->jid);
    } else if (role == TypeRole) {
        return item->type();
    } else if (role == UsernameRole) {
        return item->username();
    }

    return QVariant();
}

void AccountModel::remove(int index)
{
    removeRows(index, 1);
}

bool AccountModel::submit()
{
    QStringList newJids;

    // save passwords for new accounts
    foreach (ChatModelItem *ptr, rootItem->children) {
        AccountItem *item = static_cast<AccountItem*>(ptr);
        if (!item->changedPassword.isEmpty()) {
            setPassword(item->jid, item->changedPassword);
            item->changedPassword = QString();
        }
        newJids << item->jid;
    }

    // remove password for removed accounts
    const QStringList oldJids = wApp->settings()->chatAccounts();
    foreach (const QString &jid, oldJids) {
        if (!newJids.contains(jid)) {
            const QString key = realm(jid);
            qDebug("Removing password for %s (%s)", qPrintable(jid), qPrintable(key));
            QNetIO::Wallet::instance()->deleteCredentials(key);
        }
    }

    // save accounts
    wApp->settings()->setChatAccounts(newJids);
    foreach (AccountModel *other, globalInstances) {
        if (other != this)
            other->_q_reload();
    }

    QMetaObject::invokeMethod(wApp, "resetWindows");
    return true;
}

QString AccountModel::getPassword(const QString &jid) const
{
    const QString key = realm(jid);

    if (!key.isEmpty()) {
        QString tmpJid(jid);
        QString tmpPassword;

        if (QNetIO::Wallet::instance()->getCredentials(key, tmpJid, tmpPassword))
            return tmpPassword;
    }
    return QString();
}

bool AccountModel::setPassword(const QString &jid, const QString &password)
{
    const QString key = realm(jid);
    if (key.isEmpty())
        return false;

    qDebug("Setting password for %s (%s)", qPrintable(jid), qPrintable(key));
    return QNetIO::Wallet::instance()->setCredentials(key, jid, password);
}

void AccountModel::_q_reload()
{
    removeRows(0, rootItem->children.size());

    const QStringList chatJids = wApp->settings()->chatAccounts();
    foreach (const QString &jid, chatJids) {
        addItem(new AccountItem(jid), rootItem);
    }
}

