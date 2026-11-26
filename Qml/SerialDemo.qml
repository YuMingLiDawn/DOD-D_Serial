import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import DodModel 1.0   // ← qmlRegisterType 注册的命名空间

// 演示：Model（qmlRegisterType）+ 全局服务（ApplicationContext -> setContextProperty）
// 把 AmsApplication 里的加载路径改成 qrc:/Qml/SerialDemo.qml 就能看到这个页面
Rectangle {
    id: root
    color: "#f0f0f0"

    // QML 里直接 new 出 Model（对应 qmlRegisterType）
    SerialPortModel {
        id: portModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Text {
            text: "串口列表（Model 示例）"
            font.pixelSize: 18
            color: "#333333"
        }

        ListView {
            id: portList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: portModel

            delegate: Rectangle {
                width: portList.width
                height: 36
                color: index % 2 === 0 ? "#ffffff" : "#f7f7f7"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 12
                    font.pixelSize: 15
                    color: "#333333"
                    text: model.name + "   (" + model.description + ")"
                }
            }
        }

        RowLayout {
            spacing: 12

            Button {
                text: "刷新"
                onClicked: portModel.reload()
            }

            Button {
                text: serialService.connected ? "断开" : "连接 ttyUSB0"
                onClicked: {
                    if (serialService.connected)
                        serialService.disconnectPort();
                    else
                        serialService.connectPort("ttyUSB0");
                }
            }

            Button {
                text: "发一条"
                enabled: serialService.connected
                onClicked: serialService.sendText("hello from QML")
            }
        }

        // 全局服务属性的绑定：C++ 侧 emitted notify 后这里自动更新
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            color: "#666666"
            text: "服务状态：" + (serialService.connected ? "已连接" : "未连接")
                  + " | " + serialService.lastMessage
        }
    }
}
