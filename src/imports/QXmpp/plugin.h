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

#ifndef __QXMPP_DECLARATIVE_H__
#define __QXMPP_DECLARATIVE_H__

#include <QDeclarativeExtensionPlugin>
#include <QObject>

#include "QXmppMessage.h"
#include "QXmppPresence.h"

class QXmppDeclarativeDataForm : public QObject
{
    Q_OBJECT
    Q_ENUMS(Field)

public:
    enum Field
    {
        BooleanField = QXmppDataForm::Field::BooleanField,
        FixedField = QXmppDataForm::Field::FixedField,
        HiddenField = QXmppDataForm::Field::HiddenField,
        JidMultiField = QXmppDataForm::Field::JidMultiField,
        JidSingleField = QXmppDataForm::Field::JidSingleField,
        ListMultiField = QXmppDataForm::Field::ListMultiField,
        ListSingleField = QXmppDataForm::Field::ListSingleField,
        TextMultiField = QXmppDataForm::Field::TextMultiField,
        TextPrivateField = QXmppDataForm::Field::TextPrivateField,
        TextSingleField = QXmppDataForm::Field::TextSingleField,
    };
};

class QXmppDeclarativeMessage : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State {
        None = QXmppMessage::None,
        Active = QXmppMessage::Active,
        Inactive = QXmppMessage::Inactive,
        Gone = QXmppMessage::Gone,
        Composing = QXmppMessage::Composing,
        Paused = QXmppMessage::Paused,
    };
};

class QXmppDeclarativePresence : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)

public:
    enum Status {
        Offline = QXmppPresence::Status::Offline,
        Online = QXmppPresence::Status::Online,
        Away = QXmppPresence::Status::Away,
        XA = QXmppPresence::Status::XA,
        DND = QXmppPresence::Status::DND,
        Chat = QXmppPresence::Status::Chat,
    };
};

class QXmppDeclarativeMucItem : public QObject
{
    Q_OBJECT
    Q_ENUMS(Affiliation)

public:
    enum Affiliation {
        UnspecifiedAffiliation = QXmppMucItem::UnspecifiedAffiliation,
        OutcastAffiliation = QXmppMucItem::OutcastAffiliation,
        NoAffiliation = QXmppMucItem::NoAffiliation,
        MemberAffiliation = QXmppMucItem::MemberAffiliation,
        AdminAffiliation = QXmppMucItem::AdminAffiliation,
        OwnerAffiliation = QXmppMucItem::OwnerAffiliation,
    };
};

class QXmppPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif
