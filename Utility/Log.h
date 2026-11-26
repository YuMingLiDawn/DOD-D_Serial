#pragma once

// ============================================================================
// 日志门面（facade）
//
// 设计目标：
//   1. 业务代码只 include 本文件、只用 LOG_xxx 宏，不出现任何 spdlog 符号；
//   2. 以后换日志库，只改 Log.h / Log.cpp（以及 CMakeLists 里的依赖），
//      所有调用点零改动；
//   3. QML/CDebug 之类的零散输出统一收口到这里，便于按级别、按文件过滤。
//
// 用法：
//     LOG_INFO("port {} opened", portName);
//     LOG_ERROR("load {} failed: {}", path, reason);
// ============================================================================

#include <string>
#include <utility>

#if defined(QT_CORE_LIB)
#include <QString>
#endif

// 格式化能力：spdlog 自带 bundled fmt（third_party/spdlog/include/spdlog/fmt）。
// 这是「换库」时的第 1 个替换点：改成目标库的格式化头即可。
#include <spdlog/fmt/fmt.h>

namespace Log
{
    enum class Level { Trace = 0, Debug, Info, Warn, Error, Critical };

    struct Options {
        std::string appName = "app";
        std::string logDir; // 空 -> <exe目录>/logs；不可写时自动回退到系统应用数据目录
        bool        consoleSink   = true;
        bool        fileSink      = true;
        Level       consoleLevel  = Level::Debug;
        Level       fileLevel     = Level::Debug;
        int         maxFileSizeMb = 10; // 滚动文件单文件上限
        int         maxFiles      = 5;  // 保留的历史文件个数
    };

    // 幂等：只有第一次调用生效；返回 true 表示本次完成初始化
    bool init(const Options& options);
    void shutdown();

    // 把 Qt 自己的消息（qDebug/qWarning、QML 运行时告警）也转进本日志系统
    // 好处：Windows GUI 程序没有控制台时，QML 报错也能在日志文件里看到
    void installQtMessageHandler();

    void  setLevel(Level level);
    Level level();
    bool  shouldLog(Level level);

    // 底层入口（已格式化好的字符串）
    void write(Level level, const char* file, int line, std::string message);

    namespace detail
    {
        template <typename... Args>
        inline std::string format(const char* pattern, Args&&... args)
        {
            if constexpr (sizeof...(Args) == 0) {
                return std::string(pattern); // 无参数时不走 fmt，避免 {} 被当占位符
            }
            else {
                return fmt::format(pattern, std::forward<Args>(args)...);
            }
        }
    } // namespace detail

    // fmt 风格入口（宏会带上 __FILE__ / __LINE__）
    template <typename... Args>
    inline void trace(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Trace)) {
            return; // 级别没开就不格式化，省 CPU
        }
        write(Level::Trace, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void debug(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Debug)) {
            return;
        }
        write(Level::Debug, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void info(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Info)) {
            return;
        }
        write(Level::Info, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void warn(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Warn)) {
            return;
        }
        write(Level::Warn, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void error(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Error)) {
            return;
        }
        write(Level::Error, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void critical(const char* file, int line, const char* pattern, Args&&... args)
    {
        if (!shouldLog(Level::Critical)) {
            return;
        }
        write(Level::Critical, file, line, detail::format(pattern, std::forward<Args>(args)...));
    }
} // namespace Log

#if defined(QT_CORE_LIB)
// 让 QString 能直接放进 {} 占位符：LOG_INFO("open {}", qmlUrl.toString()) / LOG_INFO("{}", someQString)
// 纯 C++ 工程可整段删掉。
template <>
struct fmt::formatter<QString> : fmt::formatter<std::string_view> {
    auto format(const QString& value, fmt::format_context& ctx) const
    {
        const std::string utf8 = value.toStdString();
        return fmt::formatter<std::string_view>::format(utf8, ctx);
    }
};
#endif

// 业务代码统一用这些宏：文件名/行号自动带上，换库只需改上面的 Log::xxx 实现
#define LOG_TRACE(...) ::Log::trace(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) ::Log::debug(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) ::Log::info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) ::Log::warn(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) ::Log::error(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) ::Log::critical(__FILE__, __LINE__, __VA_ARGS__)
