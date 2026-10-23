import QtQuick 2.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Window {
    id: panel
    objectName: "SettingsPanel"
    title: qsTr("MusicDesktop 设置")
    width: Math.min(760, Math.max(600, Screen.width * 0.82))
    height: Math.min(840, Math.max(600, Screen.height * 0.82))
    minimumWidth: 600
    minimumHeight: 600
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2
    color: "#1a1d26"
    flags: Qt.Dialog

    property bool syncing: false
    property color accent: Boolean(cfg.value("theme.auto")) === false
            ? String(cfg.value("theme.accent"))
            : "#4f8cff"

    // Whole-block vertical offset travels across the full screen height
    readonly property int offsetRange: Math.max(200, Math.round(Screen.height))

    readonly property color bgDeep: "#14161e"
    readonly property color bgPanel: "#222531"
    readonly property color bgRaised: "#2c2f3c"
    readonly property color textPrimary: "#e4e8f1"
    readonly property color textSecondary: "#9aa3b8"
    readonly property color textMuted: "#6b7285"
    readonly property color borderSoft: "#3a3d4d"

    function write(key, value) {
        if (!panel.syncing) cfg.setValue(key, value)
    }

    function showToast(msg) { toast.show(msg) }

    function applyColor(textField) {
        var raw = textField.text.trimmed()
        if (!raw.startsWith("#")) raw = "#" + raw
        if (!/^#[0-9a-fA-F]{6}$/.test(raw)) return false
        return true
    }

    function syncAll() {
        panel.syncing = true
        clickSwitch.checked = cfg.value("window.clickThrough")
        bgSlider.value = cfg.value("window.bgOpacity")
        fontCombo.currentIndex = Math.max(0, fontCombo.model.indexOf(cfg.value("lyric.fontFamily")))
        sizeSlider.value = cfg.value("lyric.fontSize")
        alignCombo.currentIndex = ["left", "center", "right"].indexOf(cfg.value("lyric.align"))
        lineSlider.value = cfg.value("lyric.lineHeight")
        offsetSlider.value = cfg.value("lyric.offsetY")
        offsetXSlider.value = cfg.value("lyric.offsetX")
        coverOffsetXSlider.value = cfg.value("cover.offsetX")
        coverOffsetYSlider.value = cfg.value("cover.offsetY")
        opaSlider.value = cfg.value("lyric.opacity")
        scaleSlider.value = cfg.value("lyric.scale")
        coverSlider.value = cfg.value("cover.size")
        coverRotateSwitch.checked = cfg.value("cover.rotate")
        coverVinylSwitch.checked = cfg.value("cover.vinyl")
        styleSeg.currentIndex = String(cfg.value("spectrum.style")) === "wave" ? 1 : 0
        modeSeg.currentIndex = String(cfg.value("spectrum.mode")) === "mirror" ? 1 : 0
        barsSeg.currentIndex = [24, 32, 48, 64].indexOf(Number(cfg.value("spectrum.bars")))
        fpsSeg.currentIndex = Number(cfg.value("spectrum.fps")) === 30 ? 0 : 1
        sensSlider.value = Number(cfg.value("spectrum.sensitivity"))
        sensSpin.value = Math.round(Number(cfg.value("spectrum.sensitivity")) * 10)
        followSwitch.checked = Boolean(cfg.value("spectrum.followTheme"))
        colorField.text = String(cfg.value("spectrum.color"))
        barsSpin.value = Number(cfg.value("spectrum.bars"))
        fpsSpin.value = Number(cfg.value("spectrum.fps"))
        offsetYSlider.value = Number(cfg.value("spectrum.offsetY"))
        offsetYSpin.value = Number(cfg.value("spectrum.offsetY"))
        mirrorOffsetSlider.value = Number(cfg.value("spectrum.mirrorOffset"))
        mirrorOffsetSpin.value = Number(cfg.value("spectrum.mirrorOffset"))
        autoSwitch.checked = cfg.value("theme.auto")
        accentField.text = cfg.value("theme.accent")
        fftCombo.currentIndex = ["512", "1024", "2048"].indexOf(String(cfg.value("perf.fftSize")))
        motionSwitch.checked = cfg.value("perf.reduceMotion")
        onlyCloudSwitch.checked = cfg.value("media.onlyCloudMusic")
        panel.syncing = false
    }

    Component.onCompleted: syncAll()

    Connections {
        target: cfg
        function onChanged(key) { panel.syncAll() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        TabBar {
            id: tabs
            Layout.fillWidth: true
            background: Rectangle {
                color: bgDeep; radius: 8
                border.width: 1; border.color: borderSoft
            }

            TabButton {
                text: qsTr("窗口"); height: 36
                contentItem: Text { text: parent.text; font: parent.font
                    color: parent.selected ? accent : textSecondary
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.selected ? bgRaised : "transparent"; radius: 6 }
            }
            TabButton {
                text: qsTr("歌词"); height: 36
                contentItem: Text { text: parent.text; font: parent.font
                    color: parent.selected ? accent : textSecondary
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.selected ? bgRaised : "transparent"; radius: 6 }
            }
            TabButton {
                text: qsTr("外观"); height: 36
                contentItem: Text { text: parent.text; font: parent.font
                    color: parent.selected ? accent : textSecondary
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.selected ? bgRaised : "transparent"; radius: 6 }
            }
            TabButton {
                text: qsTr("律动"); height: 36
                contentItem: Text { text: parent.text; font: parent.font
                    color: parent.selected ? accent : textSecondary
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.selected ? bgRaised : "transparent"; radius: 6 }
            }
            TabButton {
                text: qsTr("性能"); height: 36
                contentItem: Text { text: parent.text; font: parent.font
                    color: parent.selected ? accent : textSecondary
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.selected ? bgRaised : "transparent"; radius: 6 }
            }
        }

        // Tab indicator underline
        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "transparent"
            Rectangle {
                height: 2
                width: parent.width / tabs.count
                color: accent
                radius: 1
                x: tabs.position * parent.width
                Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            // Restrict the internal Flickable to vertical-only flicking.
            // Horizontal drags pass through to Sliders; wheel scrolling
            // works natively.
            Component.onCompleted: {
                if (contentItem)
                    contentItem.flickableDirection = Flickable.VerticalFlick
            }

            StackLayout {
                id: stack
                width: parent.width
                currentIndex: tabs.currentIndex

            // ---- Window Tab ----
            GridLayout {
                Layout.fillWidth: true
                columns: 2; columnSpacing: 20; rowSpacing: 16

                Label { text: qsTr("鼠标穿透"); color: textPrimary }
                Switch { id: clickSwitch; onCheckedChanged: panel.write("window.clickThrough", checked) }

                Label { text: qsTr("背景浓度"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: bgSlider; from: 0; to: 1; stepSize: 0.02; Layout.fillWidth: true
                             onValueChanged: panel.write("window.bgOpacity", value) }
                    Label { text: Math.round(bgSlider.value * 100) + "%"; color: textSecondary; Layout.preferredWidth: 44 }
                }
            }

            // ---- Lyrics Tab ----
            GridLayout {
                Layout.fillWidth: true
                columns: 2; columnSpacing: 20; rowSpacing: 16

                Label { text: qsTr("字体"); color: textPrimary }
                ComboBox { id: fontCombo; implicitWidth: 220; background: null
                    model: ["Microsoft YaHei UI", "Microsoft YaHei", "SimSun",
                            "SimHei", "KaiTi", "DengXian", "Segoe UI", "Arial"]
                    onActivated: panel.write("lyric.fontFamily", displayText) }

                Label { text: qsTr("字号"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: sizeSlider; from: 14; to: 72; stepSize: 1; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.fontSize", value) }
                    Label { text: sizeSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 52 }
                }

                Label { text: qsTr("对齐"); color: textPrimary }
                ComboBox { id: alignCombo; implicitWidth: 140; background: null
                    model: [qsTr("左对齐"), qsTr("居中"), qsTr("右对齐")]
                    onActivated: panel.write("lyric.align", ["left", "center", "right"][index]) }

                Label { text: qsTr("行距"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: lineSlider; from: 1.2; to: 3.4; stepSize: 0.1; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.lineHeight", value) }
                    Label { text: lineSlider.value.toFixed(1); color: textSecondary; Layout.preferredWidth: 52 }
                }

                Label { text: qsTr("垂直偏移"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: offsetSlider; from: -1000; to: 1000; stepSize: 5; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.offsetY", value) }
                    Label { text: offsetSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 56 }
                }

                Label { text: qsTr("水平偏移"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: offsetXSlider; from: -1000; to: 1000; stepSize: 5; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.offsetX", value) }
                    Label { text: offsetXSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 56 }
                }

                Label { text: qsTr("不透明度"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: opaSlider; from: 0.1; to: 1; stepSize: 0.05; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.opacity", value) }
                    Label { text: Math.round(opaSlider.value * 100) + "%"; color: textSecondary; Layout.preferredWidth: 52 }
                }

                Label { text: qsTr("当前行放大"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: scaleSlider; from: 1.0; to: 1.25; stepSize: 0.01; Layout.fillWidth: true
                             onValueChanged: panel.write("lyric.scale", value) }
                    Label { text: scaleSlider.value.toFixed(2) + "x"; color: textSecondary; Layout.preferredWidth: 48 }
                }
            }

            // ---- Appearance Tab (merged cover + theme) ----
            GridLayout {
                Layout.fillWidth: true
                columns: 2; columnSpacing: 20; rowSpacing: 16

                Label { text: qsTr("封面尺寸"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: coverSlider; from: 120; to: 400; stepSize: 10; Layout.fillWidth: true
                             onValueChanged: panel.write("cover.size", value) }
                    Label { text: coverSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 52 }
                }

                Label { text: qsTr("封面水平偏移"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: coverOffsetXSlider; from: -1000; to: 1000; stepSize: 5; Layout.fillWidth: true
                             onValueChanged: panel.write("cover.offsetX", value) }
                    Label { text: coverOffsetXSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 56 }
                }

                Label { text: qsTr("封面垂直偏移"); color: textPrimary }
                RowLayout {
                    Layout.fillWidth: true
                    Slider { id: coverOffsetYSlider; from: -1000; to: 1000; stepSize: 5; Layout.fillWidth: true
                             onValueChanged: panel.write("cover.offsetY", value) }
                    Label { text: coverOffsetYSlider.value + "px"; color: textSecondary; Layout.preferredWidth: 56 }
                }

                Label { text: qsTr("播放时旋转"); color: textPrimary }
                Switch { id: coverRotateSwitch; onCheckedChanged: panel.write("cover.rotate", checked) }

                Label { text: qsTr("黑胶纹理"); color: textPrimary }
                Switch { id: coverVinylSwitch; onCheckedChanged: panel.write("cover.vinyl", checked) }

                Label { text: qsTr("封面自动取色"); color: textPrimary }
                Switch { id: autoSwitch; onCheckedChanged: panel.write("theme.auto", checked) }

                Label { text: qsTr("手动主题色"); color: textPrimary; enabled: !autoSwitch.checked }
                RowLayout { enabled: !autoSwitch.checked
                    Layout.fillWidth: true
                    TextField { id: accentField; implicitWidth: 140; placeholderText: "#4f8cff"
                                onEditingFinished: {
                                    if (panel.applyColor(this)) panel.write("theme.accent", text)
                                }
                    }
                    Button { text: qsTr("选色"); implicitWidth: 56; onClicked: colorPickerPopup.open() }
                }

                Label { text: qsTr("颜色预设"); color: textPrimary; Layout.alignment: Qt.AlignTop }
                Flow { Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: ["#4f8cff","#ff6b9d","#00d4aa","#ff9f43",
                                "#a55eea","#1dd1a1","#ff5a5a","#54a0ff",
                                "#feca57","#5f27cd","#01a3a4","#ee5253"]
                        Button { text: ""; implicitWidth: 24; implicitHeight: 24
                            background: Rectangle {
                                color: modelData; radius: 4
                                border.width: 2; border.color: accentField.text === modelData ? "#ffffff" : "transparent"
                            }
                            onClicked: { accentField.text = modelData; panel.write("theme.accent", modelData) }
                        }
                    }
                }
            }

            // ---- Spectrum Tab ----
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                // Live "applied" feedback whenever a spectrum key changes.
                Connections {
                    target: cfg
                    function onChanged(key) {
                        if (!panel.syncing && key.indexOf("spectrum.") === 0)
                            panel.showToast(qsTr("律动设置已实时应用"))
                    }
                }

                // ===== Card: display style =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: styleCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    ColumnLayout {
                        id: styleCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("显示样式"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("形态与方向"); color: textMuted; font.pixelSize: 11 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 6
                            Label { text: qsTr("形态"); color: textSecondary; font.pixelSize: 12 }
                            Item {
                                id: styleSeg
                                Layout.fillWidth: true; height: 38
                                property int currentIndex: 0
                                property int segCount: 2
                                signal picked(int idx)
                                Accessible.name: qsTr("律动形态")

                                Rectangle { anchors.fill: parent; radius: 8; color: bgDeep
                                            border.width: 1; border.color: borderSoft }
                                Rectangle {
                                    id: styleHl
                                    x: 4 + styleSeg.currentIndex * (width + 4)
                                    y: 4
                                    width: (styleSeg.width - 8 - 4 * (styleSeg.segCount - 1)) / styleSeg.segCount
                                    height: styleSeg.height - 8
                                    radius: 6
                                    color: accent
                                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                                }
                                Row { anchors.fill: parent; anchors.margins: 4; spacing: 4
                                    Repeater {
                                        model: [qsTr("柱状条"), qsTr("波形线")]
                                        Button {
                                            id: sbtn
                                            height: styleSeg.height - 8
                                            width: (parent.width - 4) / 2
                                            text: modelData
                                            opacity: pressed ? 0.8 : 1
                                            Behavior on opacity { NumberAnimation { duration: 80 } }
                                            background: Rectangle {
                                                radius: 6; color: "transparent"
                                                border.width: sbtn.focus ? 1.5 : 0
                                                border.color: "#ffffff"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font.pixelSize: 13
                                                color: styleSeg.currentIndex === index ? "#ffffff"
                                                      : (sbtn.hovered ? textPrimary : textSecondary)
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            Accessible.role: Accessible.RadioButton
                                            Accessible.name: modelData
                                            Accessible.checked: styleSeg.currentIndex === index
                                            onClicked: styleSeg.currentIndex = index
                                        }
                                    }
                                }
                                onCurrentIndexChanged: picked(currentIndex)
                                onPicked: panel.write("spectrum.style", idx === 1 ? "wave" : "bars")
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 6
                            Label { text: qsTr("方向"); color: textSecondary; font.pixelSize: 12 }
                            Item {
                                id: modeSeg
                                Layout.fillWidth: true; height: 38
                                property int currentIndex: 0
                                property int segCount: 2
                                signal picked(int idx)
                                Accessible.name: qsTr("律动方向")

                                Rectangle { anchors.fill: parent; radius: 8; color: bgDeep
                                            border.width: 1; border.color: borderSoft }
                                Rectangle {
                                    id: modeHl
                                    x: 4 + modeSeg.currentIndex * (width + 4)
                                    y: 4
                                    width: (modeSeg.width - 8 - 4 * (modeSeg.segCount - 1)) / modeSeg.segCount
                                    height: modeSeg.height - 8
                                    radius: 6
                                    color: accent
                                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                                }
                                Row { anchors.fill: parent; anchors.margins: 4; spacing: 4
                                    Repeater {
                                        model: [qsTr("单侧"), qsTr("镜像")]
                                        Button {
                                            id: mbtn
                                            height: modeSeg.height - 8
                                            width: (parent.width - 4) / 2
                                            text: modelData
                                            opacity: pressed ? 0.8 : 1
                                            Behavior on opacity { NumberAnimation { duration: 80 } }
                                            background: Rectangle {
                                                radius: 6; color: "transparent"
                                                border.width: mbtn.focus ? 1.5 : 0
                                                border.color: "#ffffff"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font.pixelSize: 13
                                                color: modeSeg.currentIndex === index ? "#ffffff"
                                                      : (mbtn.hovered ? textPrimary : textSecondary)
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            Accessible.role: Accessible.RadioButton
                                            Accessible.name: modelData
                                            Accessible.checked: modeSeg.currentIndex === index
                                            onClicked: modeSeg.currentIndex = index
                                        }
                                    }
                                }
                                onCurrentIndexChanged: picked(currentIndex)
                                onPicked: panel.write("spectrum.mode", idx === 1 ? "mirror" : "normal")
                            }
                            Label { text: qsTr("镜像模式会同时向上、向下展开律动"); color: textMuted; font.pixelSize: 11 }
                        }
                    }
                }

                // ===== Card: resolution =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: resCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    ColumnLayout {
                        id: resCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("精细度"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("条带数与刷新率"); color: textMuted; font.pixelSize: 11 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 6
                            Label { text: qsTr("条带数"); color: textSecondary; font.pixelSize: 12 }
                            Item {
                                id: barsSeg
                                Layout.fillWidth: true; height: 38
                                property int currentIndex: 1
                                property int segCount: 4
                                signal picked(int idx)
                                Accessible.name: qsTr("律动条带数")

                                Rectangle { anchors.fill: parent; radius: 8; color: bgDeep
                                            border.width: 1; border.color: borderSoft }
                                Rectangle {
                                    id: barsHl
                                    x: 4 + barsSeg.currentIndex * (width + 4)
                                    y: 4
                                    width: (barsSeg.width - 8 - 4 * (barsSeg.segCount - 1)) / barsSeg.segCount
                                    height: barsSeg.height - 8
                                    radius: 6
                                    color: accent
                                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                                }
                                Row { anchors.fill: parent; anchors.margins: 4; spacing: 4
                                    Repeater {
                                        model: ["24", "32", "48", "64"]
                                        Button {
                                            id: bbtn
                                            height: barsSeg.height - 8
                                            width: (parent.width - 12) / 4
                                            text: modelData
                                            opacity: pressed ? 0.8 : 1
                                            Behavior on opacity { NumberAnimation { duration: 80 } }
                                            background: Rectangle {
                                                radius: 6; color: "transparent"
                                                border.width: bbtn.focus ? 1.5 : 0
                                                border.color: "#ffffff"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font.pixelSize: 13
                                                color: barsSeg.currentIndex === index ? "#ffffff"
                                                      : (bbtn.hovered ? textPrimary : textSecondary)
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            Accessible.role: Accessible.RadioButton
                                            Accessible.name: modelData + qsTr(" 条")
                                            Accessible.checked: barsSeg.currentIndex === index
                                            onClicked: barsSeg.currentIndex = index
                                        }
                                    }
                                }
                                onCurrentIndexChanged: picked(currentIndex)
                                onPicked: panel.write("spectrum.bars", [24, 32, 48, 64][idx])
                            }
                            NumSpin {
                                id: barsSpin
                                Layout.fillWidth: true
                                from: 24; to: 64; stepSize: 8
                                Accessible.name: qsTr("律动条带数数值输入")
                                Accessible.description: qsTr("直接输入条带数，自动取 24/32/48/64 中最接近的值")
                                textFromValue: function(v, locale) { return Number(v).toFixed(0) }
                                valueFromText: function(text, locale) {
                                    var n = parseFloat(text)
                                    if (isNaN(n)) return barsSpin.value
                                    var opts = [24, 32, 48, 64]
                                    var best = 48
                                    for (var i = 0; i < opts.length; ++i)
                                        if (Math.abs(opts[i] - n) < Math.abs(best - n)) best = opts[i]
                                    return best
                                }
                                onValueChanged: {
                                    var idx = [24, 32, 48, 64].indexOf(value)
                                    if (barsSeg.currentIndex !== idx) barsSeg.currentIndex = idx
                                    panel.write("spectrum.bars", value)
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 6
                            Label { text: qsTr("刷新率"); color: textSecondary; font.pixelSize: 12 }
                            Item {
                                id: fpsSeg
                                Layout.fillWidth: true; height: 38
                                property int currentIndex: 1
                                property int segCount: 2
                                signal picked(int idx)
                                Accessible.name: qsTr("律动刷新率")

                                Rectangle { anchors.fill: parent; radius: 8; color: bgDeep
                                            border.width: 1; border.color: borderSoft }
                                Rectangle {
                                    id: fpsHl
                                    x: 4 + fpsSeg.currentIndex * (width + 4)
                                    y: 4
                                    width: (fpsSeg.width - 8 - 4 * (fpsSeg.segCount - 1)) / fpsSeg.segCount
                                    height: fpsSeg.height - 8
                                    radius: 6
                                    color: accent
                                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                                }
                                Row { anchors.fill: parent; anchors.margins: 4; spacing: 4
                                    Repeater {
                                        model: ["30 fps", "60 fps"]
                                        Button {
                                            id: fbtn
                                            height: fpsSeg.height - 8
                                            width: (parent.width - 4) / 2
                                            text: modelData
                                            opacity: pressed ? 0.8 : 1
                                            Behavior on opacity { NumberAnimation { duration: 80 } }
                                            background: Rectangle {
                                                radius: 6; color: "transparent"
                                                border.width: fbtn.focus ? 1.5 : 0
                                                border.color: "#ffffff"
                                            }
                                            contentItem: Text {
                                                text: parent.text; font.pixelSize: 13
                                                color: fpsSeg.currentIndex === index ? "#ffffff"
                                                      : (fbtn.hovered ? textPrimary : textSecondary)
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            Accessible.role: Accessible.RadioButton
                                            Accessible.name: modelData
                                            Accessible.checked: fpsSeg.currentIndex === index
                                            onClicked: fpsSeg.currentIndex = index
                                        }
                                    }
                                }
                                onCurrentIndexChanged: picked(currentIndex)
                                onPicked: panel.write("spectrum.fps", idx === 0 ? 30 : 60)
                            }
                            NumSpin {
                                id: fpsSpin
                                Layout.fillWidth: true
                                from: 30; to: 60; stepSize: 30
                                Accessible.name: qsTr("律动刷新率数值输入")
                                Accessible.description: qsTr("直接输入刷新率，自动取 30 或 60")
                                textFromValue: function(v, locale) { return Number(v).toFixed(0) + " fps" }
                                valueFromText: function(text, locale) {
                                    var n = parseFloat(text)
                                    if (isNaN(n)) return fpsSpin.value
                                    return n <= 45 ? 30 : 60
                                }
                                onValueChanged: {
                                    var idx = value === 30 ? 0 : 1
                                    if (fpsSeg.currentIndex !== idx) fpsSeg.currentIndex = idx
                                    panel.write("spectrum.fps", value)
                                }
                            }
                            Label { text: qsTr("高刷新率更流畅，但会占用更多 CPU"); color: textMuted; font.pixelSize: 11 }
                        }
                    }
                }

                // ===== Card: sensitivity =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: sensCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    ColumnLayout {
                        id: sensCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("灵敏度"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: 56; height: 26; radius: 6
                                color: Qt.rgba(accent.r, accent.g, accent.b, 0.16)
                                border.width: 1
                                border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.4)
                                Label {
                                    anchors.centerIn: parent
                                    text: sensSlider.value.toFixed(1) + "x"
                                    color: accent; font.pixelSize: 13; font.bold: true
                                }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        // Quick presets: one click instead of fine dragging.
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Repeater {
                                model: [ { t: qsTr("舒缓"), v: 0.7 },
                                         { t: qsTr("标准"), v: 1.0 },
                                         { t: qsTr("增强"), v: 1.8 } ]
                                Button {
                                    text: modelData.t
                                    implicitWidth: 64; implicitHeight: 28
                                    property bool active: Math.abs(sensSlider.value - modelData.v) < 0.051
                                    background: Rectangle {
                                        radius: 14
                                        color: parent.active
                                               ? Qt.rgba(accent.r, accent.g, accent.b, 0.18)
                                               : (parent.hovered ? Qt.rgba(255,255,255,0.08) : bgDeep)
                                        border.width: 1
                                        border.color: parent.active ? accent : borderSoft
                                    }
                                    contentItem: Text {
                                        text: parent.text; font.pixelSize: 12
                                        color: parent.active ? accent : textSecondary
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Accessible.role: Accessible.RadioButton
                                    Accessible.name: modelData.t
                                    Accessible.checked: active
                                    onClicked: sensSlider.value = modelData.v
                                }
                            }
                            Item { Layout.fillWidth: true }
                        }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Slider {
                                id: sensSlider
                                Layout.fillWidth: true
                                Accessible.name: qsTr("律动灵敏度")
                                Accessible.description: qsTr("0.3 到 3.0 倍，可用左右方向键微调")
                                from: 0.3; to: 3; stepSize: 0.1
                                onValueChanged: {
                                    sensSpin.value = Math.round(value * 10)
                                    panel.write("spectrum.sensitivity", value)
                                }
                                background: Rectangle {
                                    x: sensSlider.leftPadding
                                    y: sensSlider.height / 2 - 3
                                    width: sensSlider.availableWidth
                                    height: 6; radius: 3
                                    color: bgDeep
                                    Rectangle {
                                        width: sensSlider.visualPosition * parent.width
                                        height: parent.height; radius: 3
                                        color: accent
                                    }
                                }
                                handle: Rectangle {
                                    x: sensSlider.leftPadding + sensSlider.visualPosition
                                       * (sensSlider.width - sensSlider.leftPadding - sensSlider.rightPadding) - width / 2
                                    y: sensSlider.height / 2 - height / 2
                                    width: 18; height: 18; radius: 9
                                    color: "#ffffff"
                                    scale: sensSlider.pressed ? 1.2 : (sensSlider.hovered ? 1.1 : 1.0)
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                                    border.width: sensSlider.focus ? 2 : 3
                                    border.color: sensSlider.focus ? "#ffffff" : accent
                                }
                            }
                            NumSpin {
                                id: sensSpin
                                Layout.preferredWidth: 108
                                // Qt 5.14 SpinBox is integer-only: sensitivity
                                // (0.3-3.0) is stored internally as value*10 (3..30).
                                from: 3; to: 30; stepSize: 1
                                Accessible.name: qsTr("律动灵敏度数值输入")
                                Accessible.description: qsTr("直接输入倍率，范围 0.3 到 3.0，无效输入会自动回退")
                                textFromValue: function(v, locale) { return (v / 10).toFixed(1) + "x" }
                                valueFromText: function(text, locale) {
                                    var n = parseFloat(text)
                                    if (isNaN(n)) return sensSpin.value
                                    return Math.round(n * 10)
                                }
                                onValueChanged: {
                                    var s = value / 10
                                    if (Math.abs(sensSlider.value - s) > 0.001)
                                        sensSlider.value = s
                                    panel.write("spectrum.sensitivity", s)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("舒缓 0.3x"); color: textMuted; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("强烈 3.0x"); color: textMuted; font.pixelSize: 11 }
                        }
                    }
                }

                // ===== Card: whole-block vertical offset =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: offsetCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    ColumnLayout {
                        id: offsetCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("整体偏移"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("上下移动"); color: textMuted; font.pixelSize: 11 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Slider {
                                id: offsetYSlider
                                Layout.fillWidth: true
                                from: -panel.offsetRange; to: panel.offsetRange; stepSize: 1
                                Accessible.name: qsTr("整体垂直偏移")
                                Accessible.description: qsTr("律动整体在垂直方向上的位置，范围覆盖整个屏幕高度")
                                onValueChanged: {
                                    offsetYSpin.value = value
                                    panel.write("spectrum.offsetY", value)
                                }
                                background: Rectangle {
                                    x: offsetYSlider.leftPadding
                                    y: offsetYSlider.height / 2 - 3
                                    width: offsetYSlider.availableWidth
                                    height: 6; radius: 3; color: bgDeep
                                    Rectangle {
                                        width: offsetYSlider.visualPosition * parent.width
                                        height: parent.height; radius: 3; color: accent
                                    }
                                }
                                handle: Rectangle {
                                    x: offsetYSlider.leftPadding + offsetYSlider.visualPosition
                                       * (offsetYSlider.width - offsetYSlider.leftPadding - offsetYSlider.rightPadding) - width / 2
                                    y: offsetYSlider.height / 2 - height / 2
                                    width: 18; height: 18; radius: 9; color: "#ffffff"
                                    scale: offsetYSlider.pressed ? 1.2 : (offsetYSlider.hovered ? 1.1 : 1.0)
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                                    border.width: 3; border.color: accent
                                }
                            }
                            NumSpin {
                                id: offsetYSpin
                                Layout.preferredWidth: 108
                                from: -panel.offsetRange; to: panel.offsetRange; stepSize: 1
                                Accessible.name: qsTr("整体垂直偏移数值输入")
                                Accessible.description: qsTr("直接输入像素值，范围覆盖整个屏幕高度，无效输入会自动回退")
                                textFromValue: function(v, locale) { return Number(v).toFixed(0) + " px" }
                                valueFromText: function(text, locale) {
                                    var n = parseFloat(text)
                                    if (isNaN(n)) return offsetYSpin.value
                                    return Math.round(n)
                                }
                                onValueChanged: {
                                    if (offsetYSlider.value !== value) offsetYSlider.value = value
                                    panel.write("spectrum.offsetY", value)
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("上移 -%1px").arg(panel.offsetRange); color: textMuted; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("下移 %1px").arg(panel.offsetRange); color: textMuted; font.pixelSize: 11 }
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("设置律动整体在垂直方向上的位置（0 为默认位置，负值上移、正值下移），范围可覆盖整个屏幕高度，取代原来的自动浮动动画。")
                            color: textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap
                        }
                    }
                }

                // ===== Card: mirror offset (only visible in mirror mode) =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: mirrorCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft
                    visible: modeSeg.currentIndex === 1

                    ColumnLayout {
                        id: mirrorCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("镜像偏移"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label { text: mirrorOffsetSlider.value + " px"; color: accent; font.pixelSize: 12; font.bold: true }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Slider {
                                id: mirrorOffsetSlider
                                Layout.fillWidth: true
                                from: 0; to: 50; stepSize: 1
                                Accessible.name: qsTr("镜像偏移")
                                Accessible.description: qsTr("上下两组各自向中线两侧退让的距离，0 到 50 像素")
                                onValueChanged: {
                                    mirrorOffsetSpin.value = value
                                    panel.write("spectrum.mirrorOffset", value)
                                }
                                background: Rectangle {
                                    x: mirrorOffsetSlider.leftPadding
                                    y: mirrorOffsetSlider.height / 2 - 3
                                    width: mirrorOffsetSlider.availableWidth
                                    height: 6; radius: 3; color: bgDeep
                                    Rectangle {
                                        width: mirrorOffsetSlider.visualPosition * parent.width
                                        height: parent.height; radius: 3; color: accent
                                    }
                                }
                                handle: Rectangle {
                                    x: mirrorOffsetSlider.leftPadding + mirrorOffsetSlider.visualPosition
                                       * (mirrorOffsetSlider.width - mirrorOffsetSlider.leftPadding - mirrorOffsetSlider.rightPadding) - width / 2
                                    y: mirrorOffsetSlider.height / 2 - height / 2
                                    width: 18; height: 18; radius: 9; color: "#ffffff"
                                    scale: mirrorOffsetSlider.pressed ? 1.2 : (mirrorOffsetSlider.hovered ? 1.1 : 1.0)
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                                    border.width: 3; border.color: accent
                                }
                            }
                            NumSpin {
                                id: mirrorOffsetSpin
                                Layout.preferredWidth: 108
                                from: 0; to: 50; stepSize: 1
                                Accessible.name: qsTr("镜像偏移数值输入")
                                Accessible.description: qsTr("直接输入像素值，范围 0 到 50，无效输入会自动回退")
                                textFromValue: function(v, locale) { return Number(v).toFixed(0) + " px" }
                                valueFromText: function(text, locale) {
                                    var n = parseFloat(text)
                                    if (isNaN(n)) return mirrorOffsetSpin.value
                                    return Math.round(n)
                                }
                                onValueChanged: {
                                    if (mirrorOffsetSlider.value !== value) mirrorOffsetSlider.value = value
                                    panel.write("spectrum.mirrorOffset", value)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: qsTr("贴合 0px"); color: textMuted; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("最大 50px"); color: textMuted; font.pixelSize: 11 }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("镜像模式下，上下两组律动各自向中线两侧退让的距离（0–50 px）。中心空隙约为该值的 2 倍；数值越大，两组分开得越远。")
                            color: textMuted; font.pixelSize: 11; wrapMode: Text.WordWrap
                        }
                    }
                }

                // ===== Card: color =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: colorCard.implicitHeight + 32
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    ColumnLayout {
                        id: colorCard
                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Rectangle { width: 3; height: 15; radius: 1.5; color: accent }
                            Label { text: qsTr("颜色"); color: textPrimary; font.pixelSize: 14; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label { text: qsTr("律动条专用"); color: textMuted; font.pixelSize: 11 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06) }

                        // Follow theme switch — whole right side is clickable.
                        Switch {
                            id: followSwitch
                            Layout.fillWidth: true
                            implicitHeight: 30
                            Accessible.name: qsTr("跟随主题色")
                            Accessible.description: qsTr("律动条颜色跟随封面自动提取的主题色")
                            onCheckedChanged: panel.write("spectrum.followTheme", checked)
                            contentItem: Item {}
                            indicator: Rectangle {
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 44; height: 26; radius: 13
                                color: followSwitch.checked
                                       ? accent
                                       : (followSwitch.hovered ? "#35394a" : bgRaised)
                                border.width: followSwitch.focus ? 2 : 0
                                border.color: "#ffffff"
                                Behavior on color { ColorAnimation { duration: 120 } }
                                Rectangle {
                                    width: 20; height: 20; radius: 10; color: "#ffffff"
                                    y: 3
                                    x: followSwitch.checked ? parent.width - 23 : 3
                                    Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Label { text: qsTr("律动条颜色跟随封面自动提取的主题色"); color: textMuted; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Label { text: followSwitch.checked ? qsTr("当前：已开启") : qsTr("当前：自定义")
                                     color: followSwitch.checked ? accent : textMuted; font.pixelSize: 11 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1
                                   color: Qt.rgba(255, 255, 255, 0.06)
                                   opacity: followSwitch.checked ? 0.3 : 1 }

                        // Custom color group (wraps on narrow panels)
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 10
                            opacity: followSwitch.checked ? 0.35 : 1.0
                            enabled: !followSwitch.checked
                            Behavior on opacity { NumberAnimation { duration: 150 } }

                            Label { text: qsTr("自定义颜色"); color: textSecondary; font.pixelSize: 12 }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 10
                                // Live color preview
                                Rectangle {
                                    width: 40; height: 40; radius: 8
                                    color: {
                                        var s = String(colorField.text)
                                        return s.length === 7 ? s : "#6aa6ff"
                                    }
                                    border.width: 1; border.color: borderSoft
                                }
                                TextField {
                                    id: colorField
                                    width: 150; height: 40
                                    placeholderText: "#6aa6ff"
                                    color: textPrimary
                                    font.pixelSize: 13
                                    selectByMouse: true
                                    verticalAlignment: TextInput.AlignVCenter
                                    Accessible.name: qsTr("律动颜色十六进制值")
                                    background: Rectangle {
                                        radius: 8; color: bgDeep
                                        border.width: colorField.activeFocus ? 2 : 1
                                        border.color: colorField.activeFocus ? accent : borderSoft
                                    }
                                    onEditingFinished: {
                                        var raw = text.trim()
                                        if (!raw.startsWith("#")) raw = "#" + raw
                                        if (/^#[0-9a-fA-F]{6}$/.test(raw)) {
                                            text = raw.toLowerCase()
                                            panel.write("spectrum.color", raw.toLowerCase())
                                        }
                                    }
                                }
                            }

                            // Swatch palette with hover/press feedback
                            Flow {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: ["#4f8cff","#ff6b9d","#00d4aa","#ff9f43",
                                            "#a55eea","#1dd1a1","#ff5a5a","#54a0ff",
                                            "#feca57","#5f27cd","#01a3a4","#ee5253"]
                                    Button {
                                        id: swatch
                                        text: ""; implicitWidth: 30; implicitHeight: 30
                                        property bool hov: false
                                        scale: pressed ? 0.88 : (hov ? 1.15 : 1.0)
                                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                                        background: Rectangle {
                                            color: modelData; radius: 7
                                            border.width: String(colorField.text) === modelData ? 2.5 : 0
                                            border.color: "#ffffff"
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.NoButton
                                            hoverEnabled: true
                                            onContainsMouseChanged: swatch.hov = containsMouse
                                        }
                                        Accessible.name: qsTr("选择颜色") + " " + modelData
                                        Accessible.role: Accessible.Button
                                        onClicked: {
                                            colorField.text = modelData
                                            panel.write("spectrum.color", modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ===== Card: linkage =====
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 76
                    color: bgPanel; radius: 10
                    border.width: 1; border.color: borderSoft

                    Switch {
                        id: onlyCloudSwitch
                        anchors.fill: parent
                        anchors.margins: 16
                        implicitHeight: 44
                        Accessible.name: qsTr("仅联动网易云音乐")
                        Accessible.description: qsTr("只有网易云播放时才显示律动，其他播放器不响应")
                        onCheckedChanged: panel.write("media.onlyCloudMusic", checked)
                        contentItem: ColumnLayout {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Label { text: qsTr("仅联动网易云音乐"); color: textPrimary; font.pixelSize: 13 }
                            Label { text: qsTr("只有网易云播放时才显示律动，其他播放器不响应"); color: textMuted; font.pixelSize: 11 }
                        }
                        indicator: Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 44; height: 26; radius: 13
                            color: onlyCloudSwitch.checked
                                   ? accent
                                   : (onlyCloudSwitch.hovered ? "#35394a" : bgRaised)
                            border.width: onlyCloudSwitch.focus ? 2 : 0
                            border.color: "#ffffff"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            Rectangle {
                                width: 20; height: 20; radius: 10; color: "#ffffff"
                                y: 3
                                x: onlyCloudSwitch.checked ? parent.width - 23 : 3
                                Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                            }
                        }
                    }
                }

                // ===== Quick action: reset spectrum defaults =====
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("所有变更实时生效并自动保存"); color: textMuted; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("恢复律动默认设置")
                        implicitHeight: 30
                        Accessible.name: qsTr("恢复律动默认设置")
                        Accessible.description: qsTr("将律动样式、模式、条带数、刷新率、灵敏度和颜色恢复为默认值")
                        background: Rectangle {
                            radius: 7; color: parent.hovered ? "#35394a" : "transparent"
                            border.width: 1; border.color: borderSoft
                        }
                        contentItem: Text {
                            text: parent.text; font.pixelSize: 12
                            color: parent.hovered ? textPrimary : textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            panel.write("spectrum.style", "bars")
                            panel.write("spectrum.mode", "normal")
                            panel.write("spectrum.bars", 48)
                            panel.write("spectrum.fps", 60)
                            panel.write("spectrum.sensitivity", 1.0)
                            panel.write("spectrum.followTheme", true)
                            panel.write("spectrum.color", "#6aa6ff")
                            panel.write("spectrum.offsetY", 0)
                            panel.write("spectrum.mirrorOffset", 0)
                            panel.showToast(qsTr("已恢复律动默认设置"))
                        }
                    }
                }
            }

            // ---- Performance Tab ----
            GridLayout {
                Layout.fillWidth: true
                columns: 2; columnSpacing: 20; rowSpacing: 16

                Label { text: qsTr("FFT 点数"); color: textPrimary }
                ComboBox { id: fftCombo; implicitWidth: 140; background: null
                    model: ["512", "1024", "2048"]
                    onActivated: panel.write("perf.fftSize", parseInt(displayText)) }

                Label { text: qsTr("减弱动画"); color: textPrimary }
                Switch { id: motionSwitch; onCheckedChanged: panel.write("perf.reduceMotion", checked) }
            }
        }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 12

            Button {
                text: qsTr("恢复默认设置")
                onClicked: cfg.resetToDefaults()
                background: Rectangle {
                    color: parent.pressed ? "#3a3d4d" : "#2c2f3c"
                    radius: 6; border.width: 1; border.color: borderSoft
                }
                contentItem: Text { text: parent.text; color: textSecondary; anchors.centerIn: parent }
            }

            Item { Layout.fillWidth: true }

            Label {
                color: textMuted; wrapMode: Text.WordWrap
                text: qsTr("双击托盘图标快速打开本面板。")
            }
        }
    }

    // ---- Lightweight "applied" toast ----
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: toastText.implicitWidth + 40
        height: 34
        radius: 17
        opacity: 0
        visible: opacity > 0
        color: Qt.rgba(38, 42, 56, 0.96)
        border.width: 1
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.45)
        z: 99

        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        function show(msg) {
            toastText.text = msg
            opacity = 1
            toastTimer.restart()
        }

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#eef1f8"
            font.pixelSize: 12
            text: qsTr("律动设置已实时应用")
        }

        Timer {
            id: toastTimer
            interval: 1100
            onTriggered: toast.opacity = 0
        }
    }

    // ---- Color picker popup (Window for Qt 5.14 compatibility) ----
    Window {
        id: colorPickerPopup
        width: 260; height: 340
        flags: Qt.ToolTip | Qt.FramelessWindowHint
        color: bgPanel
        visible: false
        Rectangle {
            anchors.fill: parent; radius: 10
            color: bgPanel
            border.width: 1; border.color: borderSoft
        }
        Column { spacing: 8; anchors.fill: parent; anchors.margins: 12
            Text { text: qsTr("选择颜色"); color: textPrimary; font.bold: true }
            Flow { spacing: 6
                Repeater {
                    model: ["#4f8cff","#ff6b9d","#00d4aa","#ff9f43","#a55eea","#1dd1a1",
                            "#ff5a5a","#54a0ff","#feca57","#5f27cd","#01a3a4","#ee5253",
                            "#2ecc71","#e67e22","#3498db","#e74c3c"]
                    Button { text: ""; implicitWidth: 32; implicitHeight: 32
                        background: Rectangle {
                            color: modelData; radius: 6
                            border.width: 2; border.color: "#ffffff10"
                        }
                        onClicked: { accentField.text = modelData; panel.write("theme.accent", modelData); colorPickerPopup.close() }
                    }
                }
            }
        }
    }
}
