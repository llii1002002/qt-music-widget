import QtQuick 2.14
import QtQuick.Controls 2.14

/**
 * Compact dark-themed editable SpinBox for precise numeric entry
 * (slider companion / standalone stepper).
 *
 * Exception handling is native to SpinBox:
 *  - Range: the value is always clamped to [from, to] by the control.
 *  - Invalid input: non-numeric text is rejected by the internal validator;
 *    on commit (Enter / focus loss) anything unparsable or out of range
 *    reverts to the last valid value — no NaN/overflow can reach the config.
 *
 * Units can be appended by overriding textFromValue at the usage site,
 * e.g. textFromValue: function(v, locale) { return Number(v).toFixed(0) + " px" }
 */
SpinBox {
    id: control

    implicitWidth: 104
    implicitHeight: 34
    font.pixelSize: 12
    editable: true

    property color accent: "#4f8cff"
    property color bgDeep: "#14161e"
    property color bgPanel: "#222531"
    property color borderSoft: "#3a3d4d"
    property color textPrimary: "#e4e8f1"
    property color textMuted: "#6b7285"

    background: Rectangle {
        implicitWidth: 104
        implicitHeight: 34
        radius: 8
        color: control.bgDeep
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? control.accent : control.borderSoft
    }

    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: control.textPrimary
        selectionColor: control.accent
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        leftPadding: 24   // leave room for the stepper buttons
        rightPadding: 24
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        onEditingFinished: control.commit()
    }

    up.indicator: Rectangle {
        x: parent.width - width - 2
        y: 2
        width: 24
        height: Math.round((parent.height - 4) / 2)
        radius: 5
        color: control.up.pressed
               ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.25)
               : (control.hovered ? control.bgPanel : control.bgDeep)
        border.width: 1
        border.color: control.borderSoft
        Text {
            anchors.centerIn: parent
            text: "\u25B2"
            font.pixelSize: 7
            color: control.up.pressed ? control.accent : control.textMuted
        }
    }

    down.indicator: Rectangle {
        x: parent.width - width - 2
        y: parent.height - height - 2
        width: 24
        height: Math.round((parent.height - 4) / 2)
        radius: 5
        color: control.down.pressed
               ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.25)
               : (control.hovered ? control.bgPanel : control.bgDeep)
        border.width: 1
        border.color: control.borderSoft
        Text {
            anchors.centerIn: parent
            text: "\u25BC"
            font.pixelSize: 7
            color: control.down.pressed ? control.accent : control.textMuted
        }
    }
}
