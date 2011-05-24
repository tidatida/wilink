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
import wiLink 1.2
import 'utils.js' as Utils

Item {
    id: root

    ListModel {
        id: panels
    }

    function propEquals(a, b) {
        if (a.length != b.length)
            return false;
        for (var key in a) {
            if (a[key] != b[key])
                return false;
        }
        return true;
    }

    function showConversation(jid) {
        showPanel('ConversationPanel.qml', {'jid': Utils.jidToBareJid(jid)});
    }

    function showPanel(source, properties) {
        var found = false;
        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).source == source &&
                propEquals(panels.get(i).properties, properties)) {
                panels.get(i).panel.z = 1;
                found = true;
            } else {
                panels.get(i).panel.z = 0;
            }
        }
        if (found)
            return;

        // create panel
        console.log("creating panel " + source + " " + properties);
        var component = Qt.createComponent(source);
        var panel = component.createObject(swapper, properties);
        // FIXME: why doesn't createObject assign the properties??
        for (var key in properties) {
            panel[key] = properties[key];
        }

        panel.close.connect(function() {
            for (var i = 0; i < panels.count; i += 1) {
                if (panels.get(i).panel == panel) {
                    console.log("removing panel " + panels.get(i).source + " " + panels.get(i).properties);
                    panels.remove(i);
                    panel.destroy();
                    break;
                }
            }
        })
        panels.append({'source': source, 'properties': properties, 'panel': panel});
        return panel;
    }

    Item {
        id: left

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 220

        Rectangle {
            id: toolbar

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 46
            gradient: Gradient {
                GradientStop { position: 0; color: '#6ea1f1' }
                GradientStop { position: 1; color: '#567dbc' }
            }

            Row {
                anchors.top: parent.top
                anchors.left: parent.left

                ToolButton {
                    text: qsTr('Diagnostics')
                    icon: 'diagnostics.png'
                    onClicked: showPanel('DiagnosticPanel.qml')
                }

/*
                ToolButton {
                    text: 'Debugging'
                    icon: 'options.png'
                    onClicked: showPanel('LogPanel.qml')
                }

                ToolButton {
                    text: 'Discovery'
                    icon: 'options.png'
                    onClicked: showPanel('DiscoveryPanel.qml')
                }

                ToolButton {
                    text: 'Media'
                    icon: 'start.png'
                    onClicked: showPanel('PlayerPanel.qml')
                }
*/

                ToolButton {
                    text: qsTr('Phone')
                    icon: 'phone.png'
                    onClicked: Qt.openUrlExternally('sip://')
                }

                ToolButton {

                    text: qsTr('Photos')
                    icon: 'photos.png'
                    onClicked: {
                        var domain = Utils.jidToDomain(window.client.jid);
                        if (domain == 'wifirst.net')
                            showPanel('PhotoPanel.qml', {'url': 'wifirst://www.wifirst.net/w'});
                        else if (domain == 'gmail.com')
                            showPanel('PhotoPanel.qml', {'url': 'picasa://default'});
                    }
                }

                ToolButton {
                    text: qsTr('Shares')
                    icon: 'share.png'
                    visible: window.client.shareServer != ''
                }
            }
        }

        Loader {
            id: dialog
            z: 10
        }

        RosterView {
            id: rooms

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: toolbar.bottom
            enabled: window.client.mucServer != ''
            model: roomModel
            title: qsTr('My rooms')
            height: 150

            Connections {
                onAddClicked: {
                    dialog.source = 'RoomJoinDialog.qml';
                    dialog.item.show();
                }
                onItemClicked: showPanel('RoomPanel.qml', {'jid': model.jid})
            }
        }

        Rectangle {
            id: splitter

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: rooms.bottom
            color: '#567dbc'
            height: 5

            MouseArea {
                property int mousePressY
                property int roomsPressHeight

                anchors.fill: parent

                hoverEnabled: true
                onEntered: parent.state = 'hovered'
                onExited: parent.state = ''

                onPressed: {
                    mousePressY = mapToItem(left, mouse.x, mouse.y).y
                    roomsPressHeight = rooms.height
                }

                onPositionChanged: {
                    if (mouse.buttons & Qt.LeftButton) {
                        var position =  roomsPressHeight + mapToItem(left, mouse.x, mouse.y).y - mousePressY
                        position = Math.max(position, 0)
                        position = Math.min(position, left.height - splitter.height)
                        rooms.height = position
                    }
                }
            }
            states: State {
                name: 'hovered'
                PropertyChanges{ target: splitter; color: '#97b0d9' }
            }
        }

        RosterView {
            id: contacts

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: parent.bottom
            model: contactModel
            title: qsTr('My contacts')

            Menu {
                id: menu
                opacity: 0
            }

            Connections {
                onAddClicked: {
                    var domain = Utils.jidToDomain(window.client.jid);
                    var tip = (domain == 'wifirst.net') ? '<p>' + qsTr('<b>Tip</b>: your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>').replace('%1', 'https://www.wifirst.net/w/friends?from=wiLink') + '</p>' : '';

                    var dialog = window.inputDialog();
                    dialog.windowTitle = qsTr('Add a contact');
                    dialog.labelText = tip + '<p>' + qsTr('Enter the address of the contact you want to add.') + '</p>';
                    dialog.textValue = '@' + domain;

                    var jid = '';
                    while (!jid.match(/^[^@/]+@[^@/]+$/)) {
                        if (!dialog.exec())
                            return;
                        jid = dialog.textValue;
                    }
                    console.log("add " + jid);
                    window.client.rosterManager.subscribe(jid);
                }

                onItemClicked: {
                    menu.hide();
                    showConversation(model.jid);
                }

                onItemContextMenu: {
                    menu.model.clear()
                    if (model.url != undefined && model.url != '') {
                        menu.model.append({
                            'action': 'profile',
                            'icon': 'diagnostics.png',
                            'text': qsTr('Show profile'),
                            'url': model.url});
                    }
                    menu.model.append({
                        'action': 'rename',
                        'icon': 'options.png',
                        'name': model.name,
                        'text': qsTr('Rename contact'),
                        'jid': model.jid});
                    menu.model.append({
                        'action': 'remove',
                        'icon': 'remove.png',
                        'name': model.name,
                        'text': qsTr('Remove contact'),
                        'jid': model.jid});
                    menu.show(16, point.y - 16);
                }
            }

            Connections {
                target: window.client.rosterManager
                onSubscriptionReceived: {
                    var box = window.messageBox();
                    box.windowTitle = qsTr('Invitation from %1').replace('%1', bareJid);
                    box.text = qsTr('%1 has asked to add you to his or her contact list.\n\nDo you accept?').replace('%1', bareJid);
                    box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                    if (box.exec() == QMessageBox.Yes) {
                        // accept subscription
                        window.client.rosterManager.acceptSubscription(bareJid);

                        // request reciprocal subscription
                        window.client.rosterManager.subscribe(bareJid);
                    } else {
                        // refuse subscription
                        window.client.rosterManager.refuseSubscription(bareJid);
                    }
                }
            }

            Connections {
                target: menu
                onItemClicked: {
                    var item = menu.model.get(index);
                    if (item.action == 'profile') {
                        Qt.openUrlExternally(item.url);
                    } else if (item.action == 'rename') {
                        var dialog = window.inputDialog();
                        dialog.windowTitle = qsTr('Rename contact');
                        dialog.labelText = qsTr("Enter the name for this contact.");
                        dialog.textValue = item.name;
                        if (dialog.exec()) {
                            console.log("rename " + item.jid + ": " + dialog.textValue);
                            window.client.rosterManager.renameItem(item.jid, dialog.textValue);
                        }
                    } else if (item.action == 'remove') {
                        var box = window.messageBox();
                        box.windowTitle = qsTr("Remove contact");
                        box.text = qsTr('Do you want to remove %1 from your contact list?').replace('%1', item.name);
                        box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                        if (box.exec() == QMessageBox.Yes) {
                            console.log("remove " + item.jid);
                            window.client.rosterManager.removeItem(item.jid);
                        }
                    }
                }
            }
        }

        Connections {
            target: window.client.callManager
            onCallReceived: {
                var contactName = call.jid;
                console.log("Call received: " + contactName);

                // start incoming tone
                //int soundId = wApp->soundPlayer()->play(":/call-incoming.ogg", true);
                //m_callQueue.insert(call, soundId);

                // prompt user
                var box = window.messageBox();
                box.windowTitle = qsTr('Call from %1').replace('%1', contactName);
                box.text = qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', contactName);
                box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                if (box.exec()) {
                    showConversation(call.jid);
                    call.accept();
                } else {
                    call.hangup();
                }
            }
        }

        Connections {
            target: window.client.transferManager
            onFileReceived: {
                var contactName = job.jid;
                console.log("File received: " + contactName);

                // prompt user
                var box = window.messageBox();
                box.windowTitle = qsTr('File from %1').replace('%1', contactName);
                box.text = qsTr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?").replace('%1', contactName).replace('%2', job.fileName).replace('%3', job.fileSize);
                box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                if (box.exec()) {
                    showConversation(job.jid);
                    job.accept();
                } else {
                    job.abort();
                }
            }
        }
    }

    Item {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: left.right
        anchors.right: parent.right
    }
}