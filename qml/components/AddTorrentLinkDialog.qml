/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.tremotesf 1.0

Dialog {
    property alias url: torrentLinkTextField.text

    allowedOrientations: defaultAllowedOrientations
    canAccept: torrentLinkTextField.text && downloadDirectoryItem.text

    onAccepted: rpc.addTorrentLink(torrentLinkTextField.text,
                                   downloadDirectoryItem.text,
                                   priorityComboBox.currentPriority,
                                   startSwitch.checked)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTranslate("tremotesf", "Add Torrent Link")
            }

            FormTextField {
                id: torrentLinkTextField

                width: parent.width
                errorHighlight: !text
                label: qsTranslate("tremotesf", "Torrent Link")
                placeholderText: label
            }

            RemoteDirectorySelectionItem {
                id: downloadDirectoryItem

                text: rpc.serverSettings.downloadDirectory

                onTextChanged: {
                    var path = text.trim()
                    if (rpc.serverSettings.canShowFreeSpaceForPath) {
                        rpc.getFreeSpaceForPath(path)
                    } else {
                        if (path === rpc.serverSettings.downloadDirectory) {
                            rpc.getDownloadDirFreeSpace()
                        } else {
                            freeSpaceLabel.visible = false
                            freeSpaceLabel.text = String()
                        }
                    }
                }
            }

            Label {
                id: freeSpaceLabel

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }

                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor

                Connections {
                    target: rpc
                    onGotDownloadDirFreeSpace: {
                        if (downloadDirectoryItem.text.trim() === rpc.serverSettings.downloadDirectory) {
                            freeSpaceLabel.text = qsTranslate("tremotesf", "Free space: %1").arg(Utils.formatByteSize(bytes))
                            freeSpaceLabel.visible = true
                        }
                    }
                    onGotFreeSpaceForPath: {
                        if (path === downloadDirectoryItem.text.trim()) {
                            if (success) {
                                freeSpaceLabel.text = qsTranslate("tremotesf", "Free space: %1").arg(Utils.formatByteSize(bytes))
                            } else {
                                freeSpaceLabel.text = qsTranslate("tremotesf", "Error getting free space")
                            }
                        }
                    }
                }
            }

            ComboBox {
                id: priorityComboBox

                property int currentPriority: currentItem.itemId

                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: Torrent.HighPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "High")
                    }

                    MenuItemWithId {
                        itemId: Torrent.NormalPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "Normal")
                    }

                    MenuItemWithId {
                        itemId: Torrent.LowPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "Low")
                    }
                }
                currentItem: menu.itemForId(Torrent.NormalPriority)
            }
            TextSwitch {
                id: startSwitch
                text: qsTranslate("tremotesf", "Start downloading after adding")
                Component.onCompleted: checked = rpc.serverSettings.startAddedTorrents
            }
        }

        VerticalScrollDecorator { }
    }
}
