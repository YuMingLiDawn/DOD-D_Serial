import QtQuick 2.12
import QtGraphicalEffects 1.12

Item {
    id: root

    property bool selected: false
    property bool pressed: false
    property bool hovered: false
    property bool enabled: true
    property int radius: 3
    readonly property bool isActive: root.pressed || root.selected

    // 外投影：让按钮有“抬起”的空间层次
    DropShadow {
        anchors.fill: base
        source: base
        horizontalOffset: 0
        verticalOffset: root.isActive ? 1 : 2
        radius: root.isActive ? 6 : 10
        samples: 25
        color: root.isActive ? "#33000000" : "#55000000"
        transparentBorder: true
        visible: root.enabled
        z: -1
    }

    Rectangle {
        id: base
        anchors.fill: parent
        radius: root.radius
        antialiasing: true
        opacity: root.enabled ? 1.0 : 0.6
        gradient: Gradient {
            GradientStop {
                position: -0.1
                color: root.isActive ? "#666666" : "#4c4c4c"
            }
            GradientStop {
                position: 1.12
                color: root.isActive ? "#161616" : "#2a2929"
            }
        }
        // border.width: 1
        // border.color: "#7a7a7a"
    }

    // 顶部高光：模拟受光面，增加立体质感
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#66ffffff" }
            GradientStop { position: 0.35; color: "#22ffffff" }
            GradientStop { position: 1.0; color: "#00ffffff" }
        }
        opacity: root.isActive ? 0.22 : 0.42
    }

    // 底部内阴影：模拟厚度边缘
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.55; color: "#00000000" }
            GradientStop { position: 1.0; color: root.isActive ? "#66000000" : "#44000000" }
        }
        opacity: 0.9
    }

    // 悬浮时中间圆形微发光（不改变文字颜色）
    Item {
        id: hoverGlowSource
        anchors.centerIn: parent
        width: parent.width * 0.5
        height: parent.height * 0.5
        visible: root.hovered && root.enabled && !root.isActive

        // Rectangle {
        //     anchors.fill: parent
        //     radius: width / 2
        //     color: root.selected ? "#3ef3a8" : "#D9D9D9"
        //     opacity: 0.02
        //     antialiasing: true
        // }
    }

    RectangularGlow {
        anchors.fill: hoverGlowSource
        glowRadius: Math.min(Math.max(6, hoverGlowSource.width * 0.55), hoverGlowSource.height)

        // glowRadius: Math.max(6, hoverGlowSource.width * 0.3)
        spread: 0.25
        color: root.selected ? "#3ef3a8" : "#D9D9D9"
        cornerRadius: hoverGlowSource.width / 2 + glowRadius
        opacity: {
            if (root.selected) {
                return 0.2
            }

            return root.hovered && root.enabled && !root.isActive ? 0.2 : 0.0
        }
        
        visible: opacity > 0
    }
}