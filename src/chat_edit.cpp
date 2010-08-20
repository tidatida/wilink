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

#include <QApplication>
#include <QKeyEvent>

#include "chat_edit.h"

ChatEdit::ChatEdit(int maxheight, QWidget* parent)
    : QTextEdit(parent), maxHeight(maxheight)
{
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    setAcceptRichText(false);
    setSizePolicy(sizePolicy);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void ChatEdit::focusInEvent(QFocusEvent *e)
{
    QTextEdit::focusInEvent(e);
    emit focused();
}

void ChatEdit::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if (QApplication::keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier))
        {
            QTextCursor cursor = textCursor();
            cursor.insertBlock();
        } else
            emit returnPressed();
    }
    else if (e->key() == Qt::Key_Tab)
        emit tabPressed();
    else
        QTextEdit::keyPressEvent(e);
}

QSize ChatEdit::minimumSizeHint() const
{
    QSize sizeHint = QTextEdit::minimumSizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, maxHeight));
    return sizeHint;
}

void ChatEdit::onTextChanged()
{
    static int oldHeight = 0;
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    if (myHeight != oldHeight)
        updateGeometry();
    oldHeight = myHeight;
}

void ChatEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    updateGeometry();
}

QSize ChatEdit::sizeHint() const
{
    QSize sizeHint = QTextEdit::sizeHint();
    int myHeight = document()->size().toSize().height() + (width() - viewport()->width());
    sizeHint.setHeight(qMin(myHeight, maxHeight));
    return sizeHint;
}


