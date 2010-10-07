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

#ifndef __WILINK_CHAT_EDIT_H__
#define __WILINK_CHAT_EDIT_H__

#include <QTextEdit>

#include "QXmppMessage.h"

class QKeyEvent;

class ChatEditPrivate;

class ChatEdit : public QTextEdit
{
    Q_OBJECT

public:
    ChatEdit(int maxheight = 80, QWidget* parent = NULL);
    ~ChatEdit();

    void clear();
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    QXmppMessage::State state() const;
    QString text() const;

signals:
    void focused();
    void returnPressed();
    void stateChanged(QXmppMessage::State state);
    void tabPressed();

protected:
    virtual void focusInEvent(QFocusEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void resizeEvent(QResizeEvent *e);

public slots:
    void onTextChanged();

private slots:
    void slotInactive();
    void slotPaused();

private:
    ChatEditPrivate * const d;
};
#endif
