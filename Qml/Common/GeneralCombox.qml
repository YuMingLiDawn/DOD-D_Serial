/*
 * @Author: dawn 1964173012@qq.com
 * @Date: 2025-10-24 14:26:29
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 18:17:53
 * @FilePath: /DOD-D_Serial/Qml/Common/GeneralCombox.qml
 * @Description: 
 * 
 * Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved. 
 */
import QtQuick 2.9
import QtQuick.Controls 2.2
import Common 1.0

ComboBox {
    id: root
    property string selectedValue: ""

    property int  hoveredIndex: -1
    property int  pixelSize: 12
    property bool hoveredEnable: true
    property bool showIndicator: true

    /* Background colors */
    property color bgColorNormal: "black"
    property color bgColorHovered: "black"
    property color bgColorPressed: "black"
    property color bgColorDisabled: Qt.darker(bgColorNormal, 2)

    /* Border colors */
    property color borderColorNormal: "gray"
    property color borderColorActive: "#ffffff"

    /* Popup and item colors */
    property color popupBgColor: "white"
    property color itemBgColorNormal: "white"
    property color itemBgColorHovered: "#eff8fd"
    property color itemBgColorPressed: "#dddddd"

    /* Item border colors */
    property color itemBorderColorNormal: "white"
    property color itemBorderColorHovered: "#00BFFF"
    property color itemBorderColorPressed: "#c5c5c5"

    /* Dimensions */
    property int scrollBarSize: 10
    property int fixedWidth: 100
    property int fixedHeight: 20
    property int controlBorderRadius: 0
    property int borderWidth: 1
    property int indicatorSize: height * 0.6
    property int indicatorRightPadding: 8
    property string indicatorImagePath: "res" // 图标目录：res/indicator.svg
    property color textColor: "#ffffff"
    readonly property int contentSidePadding: root.showIndicator ? (root.indicatorSize + root.indicatorRightPadding + 4) : 4


    leftPadding: root.contentSidePadding
    rightPadding: root.contentSidePadding

    // 不要给 focus 绑 popup.visible，也不要 in onFocusChanged 里 close()：
    // 弹窗打开会接管 activeFocus，导致刚打开就被自己关掉（点击无法下拉）。
    // 「点外面关闭」由 popup.closePolicy 负责。
    currentIndex: 0

    signal close()

    background: Rectangle {
        color: enabled
            ? root.bgColorNormal
            : root.bgColorDisabled

        radius: root.controlBorderRadius
        border.width: root.borderWidth

        border.color: (root.focus ||
                       (root.hovered && hoveredEnable))
            ? root.borderColorActive
            : root.borderColorNormal
    }

    function getModelItem(index) {
        try {
            if (root.model && typeof root.model.get === 'function') {
                return root.model.get(index)
            }
            if (root.delegateModel && typeof root.delegateModel.get === 'function') {
                return root.delegateModel.get(index)
            }
            // fallback: try array-like access
            return root.model && root.model[index] !== undefined ? root.model[index] : null
        }
        catch (e) {
            return null
        }
    }

    function getItemText(index) {
        var it = getModelItem(index)
        if (!it)
            return ""
        if (root.textRole && it[root.textRole] !== undefined)
            return it[root.textRole]
        return it
    }

    contentItem: Text {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: root.displayText
        color: root.textColor
        font.pixelSize: root.pixelSize
        elide: Text.ElideNone
    }

    // 下拉箭头：用 ImageColorizer 现场渲染 res/indicator.svg 并着色
    // （颜色跟随 textColor，展开时旋转 180°）
    indicator: Item {
        visible: root.showIndicator
        width: root.indicatorSize
        height: root.indicatorSize
        x: root.width - width - root.indicatorRightPadding
        y: (root.height - height) / 2

        Image {
            // 颜色参数不能带 '#'（URL 里会被当锚点），ImageColorizer 支持 6 位十六进制
            source: "image://customImage/indicator.svg" +
                    "?color=" + root.colorParam(root.textColor) +
                    "&path=" + root.indicatorImagePath +
                    "&size=" + Math.max(1, Math.round(parent.width)) + "*" +
                               Math.max(1, Math.round(parent.height))
            width: parent.width
            height: Math.max(1, Math.round(parent.height * 0.8))
            sourceSize: Qt.size(Math.min(width, height), Math.min(width, height))
            smooth: true
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
            rotation: root.popup.visible ? 180 : 0

            Behavior on rotation {
                NumberAnimation {
                    duration: 150
                }
            }
        }
    }

    // 把 QML 颜色（"#ff0000"/"red"）转成 ImageColorizer 的 color 参数
    function colorParam(color) {
        var s = String(color)
        return s.charAt(0) === "#" ? s.substring(1) : s
    }

    popup: Popup {
        y: root.height
        // x: (root.width - root.width * 0.9) / 2
        width: root.width
        height: visible ? contentHeight : 0
        padding: 0

        contentItem: ListView {
            id: list
            clip: true
            implicitWidth: contentWidth * 0.9
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex

            ScrollBar.vertical: ScrollBar {
                visible: false
                opacity: root.popup.contentHeight >= root.popup.height ? 1.0 : 0.0
                active: visible
            }
        }

        background: Rectangle {
            color: root.popupBgColor
            radius: root.controlBorderRadius
            border.width: 1
            border.color: root.popupBgColor
        }

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onVisibleChanged: {
            if (visible) {
                // make sure the list can accept focus so that keyboard/press events go to it
                list.forceActiveFocus()
            } else {
                root.close()
            }
        }
    }

    delegate: ItemDelegate {
        property int hoveredIndex: -1

        width: root.width + root.indicator.width * 0.3
        height: root.height

        contentItem: Text {
            property var dData: {
                var md = (typeof modelData !== "undefined") ? modelData : undefined

                if (root.textRole) {
                    if (md && md[root.textRole] !== undefined)
                        return md[root.textRole]
                    if (model && model[root.textRole] !== undefined)
                        return model[root.textRole]
                    // fallback to querying root.model structure
                    if (root.model && root.model[index] && root.model[index][root.textRole] !== undefined)
                        return root.model[index][root.textRole]
                }
                if (typeof md !== "undefined")
                    return md
                if (typeof model !== "undefined")
                    return model
                return ""
            }

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            anchors {
                fill: parent
                // rightMargin: root.scrollBarSize
            }

            text: qsTr(String(dData))
            color: "black"
            font.pixelSize: root.pixelSize
            elide: Text.ElideNone
        }

        highlighted: hoveredIndex === index

        background: Item {
            width: root.width
            height: root.height

            Rectangle {
                radius: root.controlBorderRadius

                anchors {
                    fill: parent
                }

                border.color: {
                    if (root.hoveredIndex === index) {
                        return root.itemBorderColorHovered
                    }
                    else if (root.currentIndex === index) {
                        return root.itemBorderColorPressed
                    }
                    else {
                        return root.itemBorderColorNormal;
                    }
                }

                color: {
                    if (root.hoveredIndex === index) {
                        return root.itemBgColorHovered;
                    }
                    else if (root.currentIndex === index) {
                        return root.itemBorderColorPressed;
                    }
                    else {
                        return root.itemBgColorNormal;
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor

                onDoubleClicked: {
                    mouse.accepted = false
                }

                onPositionChanged: {
                    mouse.accepted = false
                }

                onPressed:  {
                    mouse.accepted = false
                }

                onPressAndHold: {
                    mouse.accepted = false
                }

                onClicked:  {
                    mouse.accepted = false
                }

                onReleased: {
                    mouse.accepted = false
                }

                onWheel: {
                    wheel.accepted = false
                }

                onContainsMouseChanged: {
                    root.hoveredIndex = containsMouse ? index : -1
                }
            }
        }
    }

    onCurrentIndexChanged: {
        var it = getModelItem(root.currentIndex)
        var newVal = ""
        if (it) {
            if (root.valueRole && it[root.valueRole] !== undefined)
                newVal = it[root.valueRole]
            else
                newVal = it
        }

        // Fallback for QAbstractItemModel-backed ComboBox where delegateModel.get()
        // may not expose role-object as expected.
        if ((newVal === "" || newVal === undefined || newVal === null) && root.valueRole === root.textRole) {
            newVal = root.displayText
        }

        if ((newVal === "" || newVal === undefined || newVal === null) && root.currentText !== undefined) {
            newVal = root.currentText
        }

        newVal = String(newVal)

        if (root.selectedValue !== newVal) {
            root.selectedValue = newVal
            root.selectedValueChanged()
        }
    }

    Component.onCompleted: {
        root.onCurrentIndexChanged()
    }
}
