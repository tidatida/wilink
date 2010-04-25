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

#include <QDebug>
#include <QRegExp>

#include "utils.h"

QString indentXml(const QString &xml)
{
    QRegExp expression("<([^>]+)>");
    int level = 0;
    int index = expression.indexIn(xml);
    QString output;
    while (index >= 0) {
        int length = expression.matchedLength();
        QString tagContents = expression.cap(1);

        if (tagContents.startsWith("/"))
            level--;

        if (output.endsWith("\n"))
            output += QString(level * 4, ' ');

        if (!tagContents.startsWith("?") &&
            !tagContents.endsWith("?") &&
            !tagContents.startsWith("/") &&
            !tagContents.endsWith("/"))
            level++;

        output += expression.cap(0);

        // look for next match
        int cursor = index + length;
        index = expression.indexIn(xml, cursor);
        if (index >= cursor)
        {
            QString data = xml.mid(cursor, index - cursor).trimmed();
            if (data.isEmpty())
                output += "\n";
            else
                output += data;
        }
    }
    return output;
}


