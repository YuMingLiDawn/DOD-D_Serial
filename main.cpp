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
#include "AmsApplication.h"
#include "ApplicationContext.h"
#include "Log.h"
#include "SerialService.h"
#include "Model/NicModel/NicModel.h"
#include "Service/NicSearch/NicSearch.h"

#include <QApplication>
#include <QSysInfo>
#include <QSurfaceFormat>
#include <QtQml/qqml.h>

namespace
{
    // 1) 注册 QML 类型：Model / 可实例化组件，QML 里用 SerialPortModel { }
    void registerQmlTypes()
    {
        // 命名空间 DodModel，版本 1.0；QML 侧 import DodModel 1.0
         qmlRegisterType<NicModel>("DodModel", 1, 0, "NicModel");

        // 只暴露枚举/常量、不允许 QML 创建时用这两个：
        //   qmlRegisterUncreatableType<T>("DodModel", 1, 0, "T", "仅用于枚举");
        //   qmlRegisterUncreatableMetaObject(NS::staticMetaObject, "DodModel", 1, 0, "NS", "仅用于枚举");
    }

    // 2) 登记全局服务（单例）：QML 里直接 serialService.xxx
    void setupApplicationContext()
    {
        ApplicationContext::instance().setContext("serialService", &SerialService::instance());
        ApplicationContext::instance().setContext("nicSearch", &NicSearch::instance());

        // 单例也可以改成「类型」方式注册（QML 里不需注入上下文属性）：
        //   qmlRegisterSingletonInstance<SerialService>("DodModel", 1, 0, "SerialService",
        //                                              &SerialService::instance());
    }
} // namespace

int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    // 日志初始化必须在其他模块打日志之前；文件落在 <exe目录>/logs/DOD-D_Serial.log
    Log::Options logOptions;
    logOptions.appName      = "DOD-D_Serial";
    logOptions.consoleLevel = Log::Level::Debug;
    logOptions.fileLevel    = Log::Level::Debug;
    Log::init(logOptions); // Qt 自身的输出（含 QML 运行时告警）也并入同一份日志
    Log::installQtMessageHandler();
    LOG_INFO("应用启动: Qt {} | {} | {} {} x {}", QString::fromLatin1(qVersion()), QSysInfo::prettyProductName(), QSysInfo::kernelType(), QSysInfo::currentCpuArchitecture(),
             QSysInfo::buildCpuArchitecture());

    // 必须在创建 QML 引擎之前完成类型注册，否则 QML 里找不到 DodModel
    registerQmlTypes();
    setupApplicationContext();

    AmsApplication window;
    window.setWindowTitle(QStringLiteral("AMS"));
    window.resize(1280, 800);
    window.show();

    const int exitCode = app.exec();
    LOG_INFO("应用退出, code={}", exitCode);
    Log::shutdown();
    return exitCode;
}
