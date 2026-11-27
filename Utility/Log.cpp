/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 17:16:35
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-27 11:39:31
 * @FilePath: /DOD-D_Serial/Utility/Log.cpp
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */
#include "Log.h"

// ============================================================================
// 日志门面的唯一实现文件：这里才允许出现具体日志库（spdlog）的符号。
// 换库 = 重写本文件 + 改 CMakeLists 的依赖，业务代码不动。
// ============================================================================

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <windows.h> // SetConsoleOutputCP：让 Windows 控制台按 UTF-8 输出中文
#endif

namespace Log
{
    namespace
    {
        std::mutex                      g_mutex;
        std::shared_ptr<spdlog::logger> g_logger;
        Level                           g_activeLevel = Level::Debug; // 两个 sink 里更“宽松”的那个，用于快速判断

        spdlog::level::level_enum toSpdlogLevel(Level level)
        {
            switch (level) {
                case Level::Trace: return spdlog::level::trace;
                case Level::Debug: return spdlog::level::debug;
                case Level::Info: return spdlog::level::info;
                case Level::Warn: return spdlog::level::warn;
                case Level::Error: return spdlog::level::err;
                case Level::Critical: return spdlog::level::critical;
            }
            return spdlog::level::info;
        }

        Level minLevel(Level a, Level b)
        {
            return static_cast<int>(a) <= static_cast<int>(b) ? a : b;
        }

        // Qt 侧消息 -> 本日志系统（QML 运行时告警会走这里）
        void forwardQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
        {
            Level lvl = Level::Info;
            switch (type) {
                case QtDebugMsg: lvl = Level::Debug; break;
                case QtInfoMsg: lvl = Level::Info; break;
                case QtWarningMsg: lvl = Level::Warn; break;
                case QtCriticalMsg: lvl = Level::Error; break;
                case QtFatalMsg: lvl = Level::Critical; break;
            }
            write(lvl, context.file != nullptr ? context.file : "Qt", context.line, message.toStdString());
        }

        // 日志目录：优先 exe 同级 logs/（方便用户直接找到），不可写时退到系统应用数据目录
        QString resolveLogDir(const Log::Options& options)
        {
            QStringList candidates;
            if (!options.logDir.empty()) {
                candidates << QString::fromStdString(options.logDir);
            }
            if (QCoreApplication::instance() != nullptr) {
                candidates << QCoreApplication::applicationDirPath() + QStringLiteral("/logs");
            }
            const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (!appData.isEmpty()) {
                candidates << appData + QStringLiteral("/logs");
            }

            for (const QString& dir : candidates) {
                if (QDir().mkpath(dir)) {
                    return dir;
                }
            }
            return {};
        }
    } // namespace

    bool init(const Options& options)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_logger) {
            return false; // 幂等
        }

#ifdef _WIN32
        if (options.consoleSink) {
            ::SetConsoleOutputCP(CP_UTF8);
        }
#endif

        std::vector<spdlog::sink_ptr> sinks;

        if (options.consoleSink) {
            auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console->set_level(toSpdlogLevel(options.consoleLevel));
            sinks.push_back(console);
        }

        if (options.fileSink) {
            const QString dir = resolveLogDir(options);
            if (!dir.isEmpty()) {
                const QString file = dir + QLatin1Char('/') + QString::fromStdString(options.appName) + QStringLiteral(".log");
                try {
                    auto fileSink =
                        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file.toStdString(), static_cast<size_t>(options.maxFileSizeMb) * 1024U * 1024U, static_cast<size_t>(options.maxFiles));
                    fileSink->set_level(toSpdlogLevel(options.fileLevel));
                    sinks.push_back(fileSink);
                } catch (const std::exception& e) {
                    // 文件 sink 建不起来不应该拖死程序，降级为只在控制台输出
                    std::fprintf(stderr, "[Log] 无法创建日志文件 %s: %s\n", file.toStdString().c_str(), e.what());
                }
            }
        }

        if (sinks.empty()) {
            return false;
        }

        auto logger = std::make_shared<spdlog::logger>(options.appName, sinks.begin(), sinks.end());
        // 时间 | 级别 | 线程 | 源位置 | 内容；跨平台一致，便于日志解析
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] [t:%t] %s:%# %v");
        logger->flush_on(spdlog::level::warn); // 警告以上立即落盘，避免崩溃时丢日志

        g_logger = logger;
        // 注册到 spdlog 全局表：flush_every() 只刷「已注册」的 logger，也方便外部按名字取用
        spdlog::register_logger(logger);
        g_activeLevel = options.consoleSink && options.fileSink ? minLevel(options.consoleLevel, options.fileLevel) : (options.consoleSink ? options.consoleLevel : options.fileLevel);

        // 定时刷盘：程序被强杀（不带 shutdown）时也能保住最近几秒的日志
        spdlog::flush_every(std::chrono::seconds(2));
        return true;
    }

    void shutdown()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_logger) {
            return;
        }
        g_logger->flush();
        g_logger.reset();
        // 清空 spdlog 全局状态，方便重复 init（例如单元测试里多次起停）
        spdlog::shutdown();
    }

    void setLevel(Level level)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_activeLevel = level;
        if (g_logger) {
            g_logger->set_level(toSpdlogLevel(level));
        }
    }

    Level level()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_activeLevel;
    }

    bool shouldLog(Level lvl)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_logger != nullptr && static_cast<int>(lvl) >= static_cast<int>(g_activeLevel);
    }

    void write(Level lvl, const char* file, int line, std::string message)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_logger) {
            // 初始化之前就打的日志也别丢：直接落到 stderr
            std::fprintf(stderr, "[Log:未初始化] %s (%s:%d)\n", message.c_str(), file != nullptr ? file : "?", line);
            return;
        }
        const spdlog::source_loc loc{file, line, ""};
        g_logger->log(loc, toSpdlogLevel(lvl), "{}", message);
    }

    void installQtMessageHandler()
    {
        // 只装一次；装完后 qDebug/qWarning 与 QML 告警都进同一个日志文件
        static bool installed = false;
        if (installed) {
            return;
        }
        installed = true;
        qInstallMessageHandler(&forwardQtMessage);
    }
} // namespace Log
