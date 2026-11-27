/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 16:07:42
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 16:56:59
 * @FilePath: /DOD-D_Serial/main.cpp
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */
#include "App/AmsApplication.h"
#include "ApplicationContext.h"
#include "Config.h" // 由 CMake 每次编译生成：版本 / git / 构建时间
#include "Log.h"
#include "Model/NicModel/NicModel.h"
#include "Service/NicSearch/NicSearch.h"
#include "Service/UdpReceiver/UdpReceiver.h"
#include "Ulxd/UlxdClient.h"
#include "Ulxd/UlxdCommands.h"

#include <QApplication>
#include <QSysInfo>
#include <QSurfaceFormat>
#include <QtQml/qqml.h>

namespace
{
    void registerQmlTypes()
    {
        qmlRegisterType<NicModel>("DodModel", 1, 0, "NicModel");
    }

    void setupApplicationContext()
    {
        ApplicationContext::instance().setContext("nicSearch", &NicSearch::instance());
    }
} // namespace

int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    Log::Options logOptions;
    logOptions.appName      = "DOD-D_Serial";
    logOptions.consoleLevel = Log::Level::Debug;
    logOptions.fileLevel    = Log::Level::Debug;
    Log::init(logOptions); // Qt 自身的输出（含 QML 运行时告警）也并入同一份日志
    Log::installQtMessageHandler();
    LOG_INFO("应用启动: Qt {} | {} | {} {} x {}", QString::fromLatin1(qVersion()), QSysInfo::prettyProductName(), QSysInfo::kernelType(), QSysInfo::currentCpuArchitecture(),
             QSysInfo::buildCpuArchitecture());
    LOG_INFO("版本 {} | Git {}@{} | 构建于 {}", VERSION_STRING, GIT_BRANCH, GIT_COMMIT_HASH, BUILD_TIME);

    registerQmlTypes();
    setupApplicationContext();

    // UdpReceiver udpReceiver;
    // udpReceiver.startListening();
    // ApplicationContext::instance().setContext("udpReceiver", &udpReceiver);

    // ============================================================================
    // 【临时联调】设置环境变量 DOD_ULXD_HOST 后，启动即连一次 ULX-D 接收机
    //   DOD_ULXD_HOST=192.168.3.123   （默认就是它；设为 off 可关闭联调）
    //   DOD_ULXD_FREQ=550000          （可选，单位 kHz，默认 630000 = 630.000MHz）
    //   DOD_ULXD_CHANNEL=1            （可选，默认通道 1）
    // 验证完成后删掉本段即可
    // ============================================================================
    Ulxd::Client  ulxdClient; // 必须活得比事件循环久
    const QString ulxdHost = qEnvironmentVariable("DOD_ULXD_HOST", "192.168.3.123");
    if (!ulxdHost.isEmpty() && ulxdHost.compare(QStringLiteral("off"), Qt::CaseInsensitive) != 0) {
        LOG_INFO("[联调] 启用，目标 {}（设 DOD_ULXD_HOST=off 可关闭）", ulxdHost);
        bool      okChannel   = false;
        const int envChannel  = qEnvironmentVariableIntValue("DOD_ULXD_CHANNEL", &okChannel);
        const int ulxdChannel = okChannel ? envChannel : 1;

        bool      okFreq  = false;
        const int envFreq = qEnvironmentVariableIntValue("DOD_ULXD_FREQ", &okFreq);
        const int ulxdKhz = okFreq ? envFreq : 630000; // 630000 = 630.000MHz

        QObject::connect(&ulxdClient, &Ulxd::Client::connected, [&ulxdClient, ulxdChannel, ulxdKhz]() {
            const Ulxd::Message command = Ulxd::Commands::setFrequency(ulxdChannel, ulxdKhz);
            LOG_INFO("[联调] 发命令: {}", command.toWire());
            ulxdClient.send(command);
        });
        QObject::connect(&ulxdClient, &Ulxd::Client::replyReceived, [](const Ulxd::Message& message) {
            LOG_INFO("[联调] 收到回执: {}  (param={} channel={} args={})", message.toWire(), message.parameter, message.channel, message.args.join(QStringLiteral(",")));
        });
        QObject::connect(&ulxdClient, &Ulxd::Client::messageReceived, [](const Ulxd::Message& message) {
            LOG_DEBUG("[联调] 帧: {}", message.toWire());
        });
        QObject::connect(&ulxdClient, &Ulxd::Client::requestTimeout, [](const Ulxd::Message& message) {
            LOG_WARN("[联调] 回执超时: {}", message.toWire());
        });
        QObject::connect(&ulxdClient, &Ulxd::Client::requestRejected, [](const Ulxd::Message& request) {
            LOG_WARN("[联调] 设备拒绝: {}（频段不匹配或格式不对）", request.toWire());
        });
        QObject::connect(&ulxdClient, &Ulxd::Client::errorOccurred, [](const QString& error) {
            LOG_WARN("[联调] 连接错误: {}", error);
        });

        ulxdClient.connectToDevice(ulxdHost, Ulxd::kDefaultTcpPort);
    }

    AmsApplication window;
    QString        appTitle = QStringLiteral("WIRELESS TERMINAL CONSOLE") + QStringLiteral(" ") + QStringLiteral("WTC-3") + QStringLiteral(" ") + QStringLiteral("无线终端控制中心");
    window.setWindowTitle(appTitle);
    window.resize(1280, 800);
    window.show();

    const int exitCode = app.exec();
    LOG_INFO("应用退出, code={}", exitCode);
    // udpReceiver.stop();
    Log::shutdown();
    return exitCode;
}
