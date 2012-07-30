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

import QtQuick 1.1

Label {
    property string style

    font.family: appStyle.icon.fontFamily
    text: {
        switch (style) {
        case 'icon-remove':
            return '\uF00D';
        case 'icon-stop':
            return '\uF04D';
        case 'icon-eject':
            return '\uF052';
        case 'icon-chevron-left':
            return '\uF053';
        case 'icon-chevron-right':
            return '\uF054';
        case 'icon-plus':
            return '\uF067';
        case 'icon-minus':
            return '\uF068';
        case 'icon-phone':
            return '\uF095';
        case 'icon-refresh':
            return '\uF0E2';
        default:
            return '';
        }
    }
}
