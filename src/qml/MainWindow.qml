import QtQuick 2.14
import QtQuick.Window 2.14
import QtGraphicalEffects 1.0

Window {
    id: root
    objectName: "WallpaperWindow"

    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"
    x: 0
    y: 0
    width: desktopSize.width
    height: desktopSize.height

    // ---- local config mirrors (updated on ConfigStore::changed) ----
    property double bgOpacity: Number(cfg.value("window.bgOpacity"))
    property int lyricOffsetX: Number(cfg.value("lyric.offsetX"))
    property int lyricOffsetY: Number(cfg.value("lyric.offsetY"))
    property int coverOffsetX: Number(cfg.value("cover.offsetX"))
    property int coverOffsetY: Number(cfg.value("cover.offsetY"))

    Connections {
        target: cfg
        function onChanged(key) {
            if (key === "window.bgOpacity")
                root.bgOpacity = Number(cfg.value(key))
            else if (key === "lyric.offsetX")
                root.lyricOffsetX = Number(cfg.value(key))
            else if (key === "lyric.offsetY")
                root.lyricOffsetY = Number(cfg.value(key))
            else if (key === "cover.offsetX")
                root.coverOffsetX = Number(cfg.value(key))
            else if (key === "cover.offsetY")
                root.coverOffsetY = Number(cfg.value(key))
        }
    }

    // ---- subtle theme-colored backdrop, stays mostly transparent ----
    LinearGradient {
        anchors.fill: parent
        start: Qt.point(0, 0)
        end: Qt.point(root.width, root.height)
        gradient: Gradient {
            GradientStop { position: 0.0; color: player.bgColor1 }
            GradientStop { position: 1.0; color: player.bgColor2 }
        }
        opacity: root.bgOpacity
    }

    // ---- lyrics block: independently positionable ----
    LyricView {
        width: 920
        height: 230
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenterOffset: root.lyricOffsetX
        anchors.verticalCenterOffset: root.lyricOffsetY
    }

    // ---- album cover: independently positionable ----
    Item {
        width: 920
        height: album.coverSize
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenterOffset: root.coverOffsetX
        anchors.verticalCenterOffset: root.coverOffsetY

        AlbumCover {
            id: album
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // ---- spectrum along the bottom edge ----
    SpectrumView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 70
        anchors.rightMargin: 70
        // Sit above the taskbar rather than underneath it.
        anchors.bottomMargin: root.height - workArea.bottom + 12
        height: 120
    }
}
