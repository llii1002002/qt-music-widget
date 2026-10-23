import QtQuick 2.14

Item {
    id: root

    property string fontFamily: String(cfg.value("lyric.fontFamily"))
    property int fontSize: Number(cfg.value("lyric.fontSize"))
    property string lyricAlign: String(cfg.value("lyric.align"))
    property double lineHeight: Number(cfg.value("lyric.lineHeight"))
    property double lyricOpacity: Number(cfg.value("lyric.opacity"))
    property double currentScale: Number(cfg.value("lyric.scale"))

    readonly property real pitch: fontSize * lineHeight

    Connections {
        target: cfg
        function onChanged(key) {
            if (key === "lyric.fontFamily")  root.fontFamily = String(cfg.value(key))
            else if (key === "lyric.fontSize") root.fontSize = Number(cfg.value(key))
            else if (key === "lyric.align")  root.lyricAlign = String(cfg.value(key))
            else if (key === "lyric.lineHeight") root.lineHeight = Number(cfg.value(key))
            else if (key === "lyric.opacity") root.lyricOpacity = Number(cfg.value(key))
            else if (key === "lyric.scale")   root.currentScale = Number(cfg.value(key))
        }
    }

    Repeater {
        model: player.lyricTexts
        delegate: Item {
            id: line
            width: root.width
            height: root.fontSize * 1.6

            readonly property real dist: Math.abs(index - player.lyricScroll)
            readonly property bool isCurrent: index === player.currentLine

            x: 0
            y: root.height / 2 - height / 2
               + (index - player.lyricScroll) * root.pitch

            Behavior on y {
                SmoothedAnimation {
                    velocity: 520
                    reversingMode: SmoothedAnimation.Sync
                    easing.type: Easing.OutQuad
                }
            }

            // Scale container for current line emphasis
            Item {
                id: scaleContainer
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                transformOrigin: Item.Center
                scale: line.isCurrent ? root.currentScale : 1.0

                Behavior on scale {
                    NumberAnimation {
                        duration: 280
                        easing.type: Easing.OutCubic
                    }
                }

                Text {
                    anchors.centerIn: parent
                    width: parent.width
                    height: parent.height
                    text: modelData
                    font.family: root.fontFamily
                    font.pixelSize: root.fontSize
                    font.bold: line.isCurrent
                    horizontalAlignment: root.lyricAlign === "left" ? Text.AlignLeft
                                         : root.lyricAlign === "right" ? Text.AlignRight
                                                                       : Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    style: Text.Outline
                    styleColor: Qt.rgba(0, 0, 0, 0.28)

                    color: {
                        if (line.isCurrent)
                            return Qt.rgba(player.textColor.r, player.textColor.g,
                                           player.textColor.b, root.lyricOpacity)
                        var a = Math.max(0, 1.0 - line.dist * 0.18) * root.lyricOpacity * 0.5
                        return Qt.rgba(player.textColor.r, player.textColor.g,
                                       player.textColor.b, Math.max(0, Math.min(1, a)))
                    }
                }
            }
        }
    }

    // ---- Fallback: static song info when lyrics are missing ----
    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: player.noLyrics

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.width
            horizontalAlignment: Text.AlignHCenter
            text: player.hasSession && player.title.length
                  ? player.title : qsTr("网易云音乐未播放")
            font.family: root.fontFamily
            font.pixelSize: root.fontSize + 8
            font.bold: true
            color: Qt.rgba(player.textColor.r, player.textColor.g, player.textColor.b,
                           root.lyricOpacity)
            style: Text.Outline
            styleColor: Qt.rgba(0, 0, 0, 0.35)
            elide: Text.ElideRight
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.width
            horizontalAlignment: Text.AlignHCenter
            text: player.artist
            font.family: root.fontFamily
            font.pixelSize: root.fontSize - 8
            color: Qt.rgba(player.textColor.r, player.textColor.g, player.textColor.b,
                           root.lyricOpacity * 0.55)
            elide: Text.ElideRight
        }
    }
}
