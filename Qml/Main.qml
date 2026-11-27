import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import DodModel 1.0

import "Common"

Item {
    id: root
    color: "#333333"

    NicModel {
        id: nicModel
    }

    // 单元格通用样式
    readonly property int labelWidth: 44
    readonly property int labelSize: 12
    readonly property int controlHeight: 24
    readonly property int itemSpacing: 6

    // 第一行占父高度的 10%
    readonly property real firstRowHeight: 40
    readonly property real firstColWidth: firstRowHeight * 5

    // 2 行 2 列网格：每格内容横向排布，一行即一条
    GridLayout {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        columns: 2
        columnSpacing: 8
        rowSpacing: 6

        // ---------- 第 1 行 第 1 列：网卡 ----------
        RowLayout {
            Layout.maximumHeight: root.firstRowHeight
            Layout.maximumWidth: root.firstColWidth
            Layout.preferredHeight: root.firstRowHeight
            Layout.preferredWidth: root.firstColWidth

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Text {
            //     text: qsTr("选择网卡")
            //     Layout.preferredWidth: root.labelWidth
            //     horizontalAlignment: Text.AlignRight
            //     color: "#aaaaaa"
            //     font.pixelSize: root.labelSize
            // }

            // GeneralCombox {
            //     id: combo
            //     Layout.fillWidth: true
            //     Layout.preferredHeight: root.controlHeight
            //     model: nicModel
            //     pixelSize: 13
            //     textColor: "#ffffff"
            //     bgColorNormal: "#444444"
            //     bgColorHovered: "#4f4f4f"
            //     bgColorPressed: "#3a3a3a"

            //     textRole: "address"
            //     valueRole: "address"
            // }
        }

        // ---------- 第 1 行 第 2 列 ----------
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: root.firstRowHeight
            Layout.maximumHeight: root.firstRowHeight
            

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // ---------- 第 2 行 第 1 列 ----------
        RowLayout {
            Layout.fillHeight: true
            Layout.maximumWidth: root.firstColWidth
            Layout.preferredWidth: root.firstColWidth

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // ---------- 第 2 行 第 2 列 ----------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}