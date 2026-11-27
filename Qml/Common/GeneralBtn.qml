import QtQuick 2.12
import QtQuick.Controls 2.2

Button {
    id: root

    hoverEnabled: true

    property bool selected:     false
    property bool visibleText:  true
    property bool iconVisible:  false

    property url imageUrl:      ""
    readonly property color textColorNormal: "#ffffff"
    readonly property color textColorHovered: textColorNormal
    readonly property color textColorPressed: '#3ef3a8'
    readonly property color textColorDisabled: Qt.darker(textColorNormal, 1)
    property int pixelSize: root.height * 0.4

    property color textColor: {
        if (! root.enabled) {
            return textColorDisabled
        }
        else if (root.pressed || root.selected) {
            return textColorPressed
        }
        else if (root.hovered) {
            return textColorHovered
        }
        else {
            return textColorNormal
        }
    }

    contentItem: Item {
        Row {
            spacing: 5
            anchors.centerIn: parent

            Image {
                visible: root.iconVisible
                source: root.imageUrl
                height: root.height * 0.6
                width: height
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.family: font.name
                elide: Text.ElideNone
                text: root.text
                color: root.textColor
                font.pixelSize: root.pixelSize
                anchors.verticalCenter: parent.verticalCenter
           }

            FontLoader {
                id: font
                source: "qrc:/font/MicrosoftYaHei.ttf"
            }
        }
    }

    background: BtnStyle {
        selected: root.selected
        pressed: root.pressed
        hovered: root.hovered
        enabled: root.enabled
    }

    // Transparent MouseArea used only for changing the cursor on hover.
    // It does not accept mouse clicks (acceptedButtons: Qt.NoButton) so the
    // Button's own internal handlers continue to work.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        // Show pointing hand when enabled, arrow when disabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
