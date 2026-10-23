import QtQuick 2.14
import QtGraphicalEffects 1.0

Item {
    id: root
    property int coverSize: Number(cfg.value("cover.size"))
    property bool rotateEnabled: Boolean(cfg.value("cover.rotate"))
    property bool vinylEnabled: Boolean(cfg.value("cover.vinyl"))
    property bool reduceMotion: Boolean(cfg.value("perf.reduceMotion"))

    // Fixed black vinyl palette — accent color is used only for the desktop
    // backdrop, never for the cover / vinyl / rings.
    readonly property color vinylBlack: "#0a0a0a"
    readonly property color vinylRing: Qt.rgba(255, 255, 255, 0.05)
    readonly property color vinylBorder: Qt.rgba(255, 255, 255, 0.12)
    readonly property color vinylShadow: Qt.rgba(0, 0, 0, 0.45)

    width: coverSize
    height: coverSize

    Connections {
        target: cfg
        function onChanged(key) {
            if (key === "cover.size")       root.coverSize = Number(cfg.value(key))
            else if (key === "cover.rotate") root.rotateEnabled = Boolean(cfg.value(key))
            else if (key === "cover.vinyl") root.vinylEnabled = Boolean(cfg.value(key))
            else if (key === "perf.reduceMotion") root.reduceMotion = Boolean(cfg.value(key))
        }
    }

    // ---- Outer DropShadow for depth ----
    Rectangle {
        id: shadowHost
        anchors.centerIn: parent
        width: root.coverSize + 24
        height: root.coverSize + 24
        radius: width / 2
        color: "transparent"
        layer.enabled: true
        layer.effect: DropShadow {
            anchors.fill: parent
            cached: true
            horizontalOffset: 0
            verticalOffset: 8
            radius: 16
            samples: 24
            color: root.vinylShadow
            source: shadowHost
        }
    }

    // ---- Vinyl concentric rings (behind artwork) ----
    Repeater {
        visible: root.vinylEnabled && art.status === Image.Ready
        model: root.vinylRingCount
        delegate: Rectangle {
            property real indexShift: index
            anchors.centerIn: parent
            width: {
                var step = root.coverSize / (root.vinylRingCount + 1)
                return width = root.coverSize - (index + 1) * step * 0.6
            }
            height: width
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: root.vinylRing
            opacity: 0.7
            rotation: artLayer.rotation * 0.9
        }
    }

    // ---- Rotating circular artwork layer ----
    Rectangle {
        id: artLayer
        anchors.centerIn: parent
        width: root.coverSize
        height: root.coverSize
        radius: width / 2
        color: root.vinylBlack
        layer.enabled: true
        layer.effect: OpacityMask { maskSource: circleMask }
        opacity: player.playing ? 1 : 0.75

        Image {
            id: art
            anchors.fill: parent
            source: player.coverUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: false
            cache: false
            opacity: 0
            onStatusChanged: {
                if (status === Image.Ready) opacity = 1
                else if (status === Image.Error || status === Image.Null) opacity = 0
            }
            Behavior on opacity {
                NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
            }
        }
    }

    // ---- Vinyl center sticker ----
    Rectangle {
        visible: root.vinylEnabled && art.status === Image.Ready
        anchors.centerIn: artLayer
        width: root.coverSize * 0.18
        height: width
        radius: width / 2
        color: root.vinylBlack
        border.width: 2
        border.color: root.vinylBorder
    }

    // ---- Mask outside the rotating layer (fixed circle) ----
    Rectangle {
        id: circleMask
        anchors.fill: artLayer
        radius: width / 2
        visible: false
        color: "white"
    }

    // ---- Outer ring ----
    Rectangle {
        anchors.centerIn: parent
        width: root.coverSize
        height: root.coverSize
        radius: width / 2
        color: "transparent"
        border.width: 2
        border.color: root.vinylBorder
    }

    // ---- Gradient placeholder (replaces plain ♪) ----
    Rectangle {
        visible: art.status !== Image.Ready
        anchors.centerIn: parent
        width: root.coverSize * 0.92
        height: width
        radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(40, 40, 45, 0.95) }
            GradientStop { position: 1.0; color: Qt.rgba(15, 15, 18, 0.98) }
        }

        Text {
            anchors.centerIn: parent
            text: "\u266B"
            font.family: "Segoe UI Symbol"
            font.pixelSize: root.coverSize * 0.38
            color: Qt.rgba(255, 255, 255, 0.5)
        }
    }

    // ---- Pause indicator dot ----
    Rectangle {
        visible: !player.playing && art.status === Image.Ready
        anchors.right: artLayer.right
        anchors.bottom: artLayer.bottom
        width: 12
        height: 12
        radius: width / 2
        color: Qt.rgba(255, 255, 255, 0.8)

        NumberAnimation {
            target: parent; property: "opacity"
            from: 0.5; to: 1.0; duration: 900
            loops: Animation.Infinite; easing.type: Easing.InOutQuad
        }
    }

    NumberAnimation {
        target: artLayer
        property: "rotation"
        from: 0; to: 360
        duration: 24000
        loops: Animation.Infinite
        easing.type: Easing.Linear
        running: player.playing && root.rotateEnabled && !root.reduceMotion
    }

    readonly property int vinylRingCount: 6
}
