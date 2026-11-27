import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import DodModel 1.0

import "Common"

Rectangle {
    id: root
    color: "#333333"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 5

        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.12

            ColumnLayout {
                anchors.fill: parent

                Item {
                    Layout.preferredHeight: parent.height * 0.04
                    Layout.fillWidth: true

                    NicModel {
                        id: nicModel
                    }

                    GeneralCombox {
                        id: combo
                        anchors.fill: parent
                        model: nicModel
                        pixelSize: 13
                        textColor: "#ffffff"
                        bgColorNormal: "#444444"
                        bgColorHovered: "#4f4f4f"
                        bgColorPressed: "#3a3a3a"

                        textRole: "address"
                        valueRole: "address"
                    }
                }




                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}