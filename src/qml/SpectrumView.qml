import QtQuick 2.14

Item {
    id: root

    property int barCount: Number(cfg.value("spectrum.bars"))
    property string styleName: String(cfg.value("spectrum.style"))     // "bars" | "wave"
    property string modeName: String(cfg.value("spectrum.mode"))       // "normal" | "mirror"
    property bool followTheme: Boolean(cfg.value("spectrum.followTheme"))
    property color customColor: String(cfg.value("spectrum.color"))
    property color activeColor: followTheme ? player.accent : customColor

    // Whole-block vertical offset (static, user-adjustable "float"), px.
    // Values clamped defensively — ConfigStore validates too (±20000 allows
    // full-screen travel on any display), this guards the render layer
    // against corrupted INI / NaN.
    property int offsetY: clampInt(cfg.value("spectrum.offsetY"), -20000, 20000, 0)

    // Mirror offset: how far each half retracts from the centre line (px).
    // The gap between the two halves equals 2 * mirrorOffset.
    property int mirrorOffset: clampInt(cfg.value("spectrum.mirrorOffset"), 0, 50, 0)

    readonly property var values: player.spectrum
    readonly property bool mirrored: modeName === "mirror"

    // Derived: half height minus the retract offset, used by both renderers
    readonly property real halfH: Math.max(10, height / 2 - root.mirrorOffset)

    // ---- Defensive numeric clamps ----
    function clampReal(raw, lo, hi, def) {
        var n = Number(raw)
        if (isNaN(n) || n === undefined) return def
        return Math.max(lo, Math.min(hi, n))
    }
    function clampInt(raw, lo, hi, def) {
        var n = Math.round(Number(raw))
        if (isNaN(n) || n === undefined) return def
        return Math.max(lo, Math.min(hi, n))
    }

    // Peak-hold state for bars
    property var peakHold: []
    property var peakY: []

    // Idle breathing
    property real breathPhase: 0
    NumberAnimation {
        target: root; property: "breathPhase"; from: 0; to: Math.PI * 2
        duration: 4000; loops: Animation.Infinite; running: !player.playing
    }

    // Whole-block vertical offset applied as a transform so layout is not
    // affected (static, no animation — the user sets the position directly).
    transform: Translate { id: blockTranslate; y: root.offsetY }

    Connections {
        target: cfg
        function onChanged(key) {
            if (key === "spectrum.bars")        root.barCount = Number(cfg.value(key))
            else if (key === "spectrum.style")  root.styleName = String(cfg.value(key))
            else if (key === "spectrum.mode")   root.modeName = String(cfg.value(key))
            else if (key === "spectrum.followTheme") root.followTheme = Boolean(cfg.value(key))
            else if (key === "spectrum.color")  root.customColor = String(cfg.value(key))
            else if (key === "spectrum.offsetY")    root.offsetY = clampInt(cfg.value(key), -20000, 20000, 0)
            else if (key === "spectrum.mirrorOffset") root.mirrorOffset = clampInt(cfg.value(key), 0, 50, 0)
        }
    }

    Connections {
        target: player
        function onSpectrumChanged() {
            // Decay peak holds
            for (var i = 0; i < root.peakHold.length; ++i) {
                var v = root.values[i] || 0
                if (v > root.peakHold[i])
                    root.peakHold[i] = v
                else
                    root.peakHold[i] *= 0.90
            }
            if (root.styleName === "wave" && wave1.visible)
                wave1.requestPaint()
        }
    }

    onBarCountChanged: { peakHold = new Array(barCount).fill(0); peakY = new Array(barCount).fill(0) }
    onStyleNameChanged: if (styleName === "wave" && wave1.visible) { wave1.requestPaint(); wave2.requestPaint() }
    onModeNameChanged: if (styleName === "wave") { wave1.requestPaint(); wave2.requestPaint() }
    onWidthChanged: if (styleName === "wave") { wave1.requestPaint(); wave2.requestPaint() }
    onHeightChanged: if (styleName === "wave") { wave1.requestPaint(); wave2.requestPaint() }
    onMirrorOffsetChanged: if (styleName === "wave") { wave1.requestPaint(); wave2.requestPaint() }

    // ---- Dark backdrop so bars/waves always contrast with the wallpaper ----
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 10
        color: Qt.rgba(0, 0, 0, 0.35)
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.06)
    }

    // ---- Bars with peak-hold ----
    Repeater {
        model: root.barCount
        delegate: Item {
            visible: root.styleName === "bars"
            width: (root.width - (root.barCount - 1) * 2) / root.barCount
            height: root.height
            x: index * width + index * 2
            y: 0

            Rectangle {
                id: bar
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: {
                    var v = root.values[index]
                    if (v === undefined) v = 0
                    if (!player.playing && root.breathPhase > 0)
                        v = Math.sin(root.breathPhase + index * 0.3) * 0.04 + 0.03
                    if (root.mirrored)
                        v = v * 0.75
                    return Math.max(2.0, v * (root.mirrored ? root.halfH - 6 : root.height - 6))
                }
                radius: Math.min(3, width / 2)
                gradient: Gradient {
                    GradientStop { position: 0.0
                        color: Qt.rgba(root.activeColor.r, root.activeColor.g, root.activeColor.b, 1.0) }
                    GradientStop { position: 1.0
                        color: Qt.rgba(root.activeColor.r, root.activeColor.g, root.activeColor.b, 0.45) }
                }
            }

            // Peak-hold bright cap
            Rectangle {
                anchors.bottom: bar.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: 2
                color: Qt.rgba(root.activeColor.r, root.activeColor.g,
                               root.activeColor.b, 0.95)
                opacity: root.peakHold[index] > 0.05 ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 150 } }
                y: -1
            }
        }
    }

    // Mirror bars (top half, mirrored) — each half retracts by mirrorOffset
    // from the centre line, so the centre gap equals 2 * mirrorOffset
    Repeater {
        model: root.barCount
        delegate: Item {
            visible: root.styleName === "bars" && root.mirrored
            width: (root.width - (root.barCount - 1) * 2) / root.barCount
            height: root.halfH
            x: index * width + index * 2
            y: 0

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                height: {
                    var v = root.values[index]
                    if (v === undefined) v = 0
                    if (!player.playing && root.breathPhase > 0)
                        v = Math.sin(root.breathPhase + index * 0.3) * 0.04 + 0.03
                    return Math.max(2.0, v * 0.75 * (root.halfH - 6))
                }
                radius: Math.min(3, width / 2)
                gradient: Gradient {
                    GradientStop { position: 0.0
                        color: Qt.rgba(root.activeColor.r, root.activeColor.g, root.activeColor.b, 0.45) }
                    GradientStop { position: 1.0
                        color: Qt.rgba(root.activeColor.r, root.activeColor.g, root.activeColor.b, 1.0) }
                }
            }
        }
    }

    // ---- Waveform: dark thick line + bright thin line ----
    Canvas {
        id: wave2
        visible: root.styleName === "wave"
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            drawWave(ctx, 6, Qt.rgba(root.activeColor.r, root.activeColor.g,
                                      root.activeColor.b, 0.22),
                     root.mirrored ? root.halfH * 2 : height)
        }
    }

    Canvas {
        id: wave1
        visible: root.styleName === "wave"
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            drawWave(ctx, 2, Qt.rgba(root.activeColor.r, root.activeColor.g,
                                      root.activeColor.b, 0.95),
                     root.mirrored ? root.halfH * 2 : height)
        }
    }

    function drawWave(ctx, lw, color, h) {
        var n = root.values.length
        if (n < 2) return
        ctx.strokeStyle = color
        ctx.lineWidth = lw
        ctx.lineJoin = "round"
        ctx.lineCap = "round"

        // Mirror mode: the top half sits on the base line halfH, the bottom
        // half on its mirror (height - halfH), so the centre gap equals
        // 2 * mirrorOffset and the waveform truly "opens" from the middle.
        // Amplitude space stays at half the height regardless of the offset —
        // the base lines move apart while the waveform keeps its full size.
        var base = root.mirrored ? root.halfH : h / 2
        var drawH = root.mirrored ? root.height / 2 : h / 2
        var breath = !player.playing ? Math.sin(root.breathPhase) * 0.06 : 0

        // Quadratic-bezier smoothing through midpoints — lineTo would render
        // a jagged polyline across sparse FFT bins.
        function strokeHalf(baseY, sign) {
            var pts = []
            for (var i = 0; i < n; ++i) {
                var v = root.values[i] || 0
                v = Math.max(0.02, v + breath)
                pts.push({ x: width * i / (n - 1), y: baseY + sign * v * (drawH - 4) })
            }
            ctx.beginPath()
            ctx.moveTo(pts[0].x, pts[0].y)
            for (var j = 1; j < n - 1; ++j) {
                var mx = (pts[j].x + pts[j + 1].x) / 2
                var my = (pts[j].y + pts[j + 1].y) / 2
                ctx.quadraticCurveTo(pts[j].x, pts[j].y, mx, my)
            }
            ctx.lineTo(pts[n - 1].x, pts[n - 1].y)
            ctx.lineTo(width, baseY)
            ctx.stroke()
        }

        strokeHalf(base, -1)

        if (root.mirrored)
            strokeHalf(root.height - root.halfH, 1)
    }

    Component.onCompleted: {
        peakHold = new Array(root.barCount).fill(0)
        peakY = new Array(root.barCount).fill(0)
    }
}
