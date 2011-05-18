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

import QtQuick 1.0

Item {
    id: block

    property alias model: view.model
    signal participantClicked(string participant)

    GridView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cellWidth: 80
        cellHeight: 54

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: '#ffffff'
                color: '#ffffff'
                gradient: Gradient {
                    GradientStop { id: stop1; position: 0.0; color: '#ffffff'  }
                    GradientStop { id: stop2; position: 1.0; color: '#ffffff'  }
                }
            }

            Column {
                anchors.fill: parent
                anchors.margins: 5
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    asynchronous: true
                    source: model.avatar
                    height: 32
                    width: 32
                 }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: '#2689d6'
                    elide: Text.ElideRight
                    font.pixelSize: 10
                    text: model.name
                }
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        // hide context menu
                        block.participantClicked(model.name);
                        menu.state = '';
                    } else if (mouse.button == Qt.RightButton) {
                        // show context menu
                        menu.model.clear()
                        if (model.url != '')
                            menu.model.append({
                                'icon': 'diagnostics.png',
                                'text': qsTr('Show profile'),
                                'url':model.url})
                        menu.x = item.x + mouse.x - menu.width + 16;
                        menu.y = item.y + mouse.y - 16;
                        menu.state = 'visible';
                    }
                }
                onEntered: {
                    parent.state = 'hovered'
                }
                onExited: {
                    parent.state = ''
                }
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: itemBackground; border.color: '#b0e2ff' }
                PropertyChanges { target: stop1;  color: '#ffffff' }
                PropertyChanges { target: stop2;  color: '#b0e2ff' }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }

    Menu {
        id: menu
        opacity: 0
    }

    Connections {
        target: menu
        onItemClicked: Qt.openUrlExternally(menu.model.get(index).url)
    }
}
