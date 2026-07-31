/**
 * @file DX12Log.h
 * @brief DX12 日志系统 / DX12 Logging System
 *
 * 提供分级日志输出功能，支持控制台彩色输出、调试输出和文件日志。
 * 日志文件定期刷新到 Data 目录，包含完整日志和错误日志两个文件。
 * 在 Release 构建下日志函数变为空操作以消除运行时开销。
 *
 * Provides leveled logging output with console color output, debug output,
 * and file logging. Log files are periodically flushed to the Data directory,
 * containing both full log and error log files. In Release builds, log
 * functions become no-ops to eliminate runtime overhead.
 */

#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <mutex>
#include <cstdio>

namespace YingLong
{
    /**
     * @brief 日志消息的严重程度分类 / Severity classification for log messages
     *
     * 定义不同级别的日志消息，用于控制输出颜色和过滤。
     * Defines different levels of log messages for controlling output color and filtering.
     */
    enum class LogSeverity : unsigned char
    {
        Info,      ///< 信息级 / Information level
        Success,   ///< 成功级 / Success level
        Warning,   ///< 警告级 / Warning level
        Error,     ///< 错误级 / Error level
        Header     ///< 标题级 / Header level
    };

    /**
     * @brief 获取指定流的控制台句柄 / Get console handle for the specified stream
     *
     * 返回标准输出或标准错误流的控制台句柄，用于设置控制台文本颜色。
     * 使用静态局部变量缓存句柄，避免重复调用系统 API。
     *
     * Returns the console handle for stdout or stderr stream, used for setting
     * console text color. Uses static local variables to cache handles and avoid
     * repeated system API calls.
     *
     * @param isStderr 是否为标准错误流 / Whether it's the stderr stream
     * @return 控制台句柄 / Console handle
     */
    inline HANDLE GetConsoleHandleForStream(bool isStderr)
    {
        static HANDLE hStdOut = []() {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            return (h == INVALID_HANDLE_VALUE) ? nullptr : h;
        }();
        static HANDLE hStdErr = []() {
            HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
            return (h == INVALID_HANDLE_VALUE) ? nullptr : h;
        }();
        return isStderr ? hStdErr : hStdOut;
    }

    /**
     * @brief 将日志严重级别转换为控制台颜色属性 / Convert log severity to console color attribute
     *
     * 根据日志严重级别返回对应的 Windows 控制台颜色属性值。
     * Returns the corresponding Windows console color attribute value based on
     * the log severity level.
     *
     * @param sev 日志严重级别 / Log severity level
     * @return 控制台颜色属性值 / Console color attribute value
     */
    inline WORD SeverityToColor(LogSeverity sev) noexcept
    {
        switch (sev)
        {
        case LogSeverity::Success: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case LogSeverity::Warning: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case LogSeverity::Error:   return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case LogSeverity::Header:  return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case LogSeverity::Info:
        default:                   return FOREGROUND_INTENSITY;
        }
    }

    // ---- 文件日志状态 ----
    // 两个滚动缓冲区每 3 秒刷新到 <exe_dir>/Data/ 目录：
    //   debug_all.txt    - 所有日志消息
    //   debug_errors.txt - 仅警告和错误
    // ---- File logging state ----
    // Two rolling buffers flushed to <exe_dir>/Data/ every 3 seconds:
    //   debug_all.txt    - every log message
    //   debug_errors.txt - only warnings and errors

    /**
     * @brief 文件日志状态结构体 / File logging state structure
     *
     * 管理文件日志的内部状态，包括缓冲区、互斥锁和刷新计时器。
     * 使用双缓冲机制减少磁盘 I/O 开销。
     *
     * Manages the internal state of file logging, including buffers, mutex,
     * and flush timer. Uses a double-buffering mechanism to reduce disk I/O overhead.
     */
    struct DX12LogFileState
    {
        std::mutex mu;                                                              ///< 缓冲区互斥锁 / Buffer mutex lock
        std::string all_buffer;                                                     ///< 所有日志的缓冲区 / Buffer for all logs
        std::string error_buffer;                                                   ///< 错误日志的缓冲区 / Buffer for error logs
        std::chrono::steady_clock::time_point last_flush = std::chrono::steady_clock::now();   ///< 上次刷新时间 / Last flush time
        bool enabled = true;                                                        ///< 文件日志是否启用 / Whether file logging is enabled
    };

    /**
     * @brief 获取全局日志文件状态实例 / Get global log file state instance
     *
     * 返回 DX12LogFileState 的单例实例，使用 Meyer's Singleton 模式。
     * Returns the singleton instance of DX12LogFileState using Meyer's Singleton pattern.
     *
     * @return 日志文件状态引用 / Log file state reference
     */
    inline DX12LogFileState& GetLogFileState()
    {
        static DX12LogFileState s;
        return s;
    }

    /**
     * @brief 获取可执行文件所在目录 / Get the executable directory
     *
     * 返回当前可执行文件所在的目录路径。
     * Returns the directory path where the current executable is located.
     *
     * @return 可执行文件目录路径 / Executable directory path
     */
    inline std::string GetExeDir()
    {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string p(path);
        size_t pos = p.find_last_of("\\/");
        return (pos != std::string::npos) ? p.substr(0, pos) : p;
    }

    /**
     * @brief 获取日志时间戳字符串 / Get log timestamp string
     *
     * 返回当前时间的格式化时间戳字符串（时:分:秒.毫秒）。
     * Returns a formatted timestamp string of the current time (hour:minute:second.millisecond).
     *
     * @return 时间戳字符串 / Timestamp string
     */
    inline std::string GetLogTimestamp()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        struct tm tmv;
        localtime_s(&tmv, &t);
        char buf[32];
        sprintf_s(buf, "%02d:%02d:%02d.%03d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (int)ms.count());
        return std::string(buf);
    }

    /**
     * @brief 确保 Data 目录存在 / Ensure the Data directory exists
     *
     * 如果 Data 目录不存在则创建它。
     * Creates the Data directory if it doesn't exist.
     */
    inline void EnsureDataDir()
    {
        CreateDirectoryA((GetExeDir() + "\\Data").c_str(), nullptr);
    }

    /**
     * @brief 刷新日志文件 / Flush log files
     *
     * 将缓冲区中的所有日志内容写入磁盘文件，覆盖现有文件。
     * Writes all log content from buffers to disk files, overwriting existing files.
     */
    inline void FlushLogFiles()
    {
        DX12LogFileState& s = GetLogFileState();
        if (!s.enabled) return;
        s.last_flush = std::chrono::steady_clock::now();
        EnsureDataDir();
        std::ofstream fAll(GetExeDir() + "\\Data\\debug_all.txt", std::ios::out | std::ios::trunc);
        if (fAll.is_open()) fAll << s.all_buffer;
        std::ofstream fErr(GetExeDir() + "\\Data\\debug_errors.txt", std::ios::out | std::ios::trunc);
        if (fErr.is_open()) fErr << s.error_buffer;
    }

    /**
     * @brief 尝试刷新日志文件（定时刷新）/ Try to flush log files (timed flush)
     *
     * 检查距离上次刷新是否超过 3 秒，如果是则执行刷新。
     * Checks if more than 3 seconds have passed since the last flush,
     * and performs a flush if so.
     */
    inline void TryFlushLogFiles()
    {
        DX12LogFileState& s = GetLogFileState();
        if (!s.enabled) return;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s.last_flush).count() < 3000) return;
        FlushLogFiles();
    }

    /**
     * @brief 追加日志消息到文件 / Append log message to file
     *
     * 将带时间戳的日志消息追加到内存缓冲区，错误级别的消息同时追加到错误缓冲区。
     * 追加后会尝试定时刷新到磁盘。
     *
     * Appends timestamped log messages to the in-memory buffer. Error-level messages
     * are also appended to the error buffer. After appending, a timed flush to disk is attempted.
     *
     * @param msg 日志消息内容 / Log message content
     * @param isError 是否为错误级别消息 / Whether it's an error-level message
     */
    inline void AppendToLogFile(const char* msg, bool isError)
    {
        DX12LogFileState& s = GetLogFileState();
        std::lock_guard<std::mutex> lock(s.mu);
        std::string ts = "[" + GetLogTimestamp() + "] ";
        s.all_buffer += ts;
        s.all_buffer += msg;
        if (isError)
        {
            s.error_buffer += ts;
            s.error_buffer += msg;
        }
        FlushLogFiles();
    }

    /**
     * @brief 所有公共日志函数共享的核心实现 / Core implementation shared by all public logging functions
     *
     * 将日志输出到三个目标：OutputDebugStringA、带颜色的控制台、以及文件日志。
     * Outputs logs to three destinations: OutputDebugStringA, colored console, and file log.
     *
     * @param sev 日志严重级别 / Log severity level
     * @param msg 日志消息内容 / Log message content
     */
    inline void DX12LogImpl(LogSeverity sev, const char* msg)
    {
        OutputDebugStringA(msg);

        bool isError = (sev == LogSeverity::Error || sev == LogSeverity::Warning);
        HANDLE h = GetConsoleHandleForStream(isError);
        if (h) SetConsoleTextAttribute(h, SeverityToColor(sev));
        (isError ? std::cerr : std::cout) << msg << std::flush;
        if (h) SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        AppendToLogFile(msg, isError);
    }

    // ---- 公共日志 API ----
    // 在 Release 构建下（未定义 _DEBUG），所有日志变为空操作
    // 以消除热路径上的控制台 I/O、互斥锁和文件写入开销。
    // ---- Public logging API ----
    // In Release builds (_DEBUG undefined) all logging becomes a no-op
    // to eliminate the overhead of console I/O, mutex locks, and file
    // writes on the hot path.

#if defined(_DEBUG)
    /**
     * @brief 输出信息级日志 / Output info level log
     * @param msg 日志消息 / Log message
     */
    inline void DX12Log(const char* msg)                 { DX12LogImpl(LogSeverity::Info,    msg); }

    /**
     * @brief 输出成功级日志 / Output success level log
     * @param msg 日志消息 / Log message
     */
    inline void DX12LogSuccess(const char* msg)          { DX12LogImpl(LogSeverity::Success, msg); }

    /**
     * @brief 输出错误级日志 / Output error level log
     * @param msg 日志消息 / Log message
     */
    inline void DX12LogError(const char* msg)             { DX12LogImpl(LogSeverity::Error,   msg); }

    /**
     * @brief 输出警告级日志 / Output warning level log
     * @param msg 日志消息 / Log message
     */
    inline void DX12LogWarning(const char* msg)           { DX12LogImpl(LogSeverity::Warning, msg); }

    /**
     * @brief 输出标题级日志 / Output header level log
     * @param msg 日志消息 / Log message
     */
    inline void DX12LogHeader(const char* msg)            { DX12LogImpl(LogSeverity::Header,  msg); }
#else
    inline void DX12Log(const char*) {}
    inline void DX12LogSuccess(const char*) {}
    inline void DX12LogError(const char*) {}
    inline void DX12LogWarning(const char*) {}
    inline void DX12LogHeader(const char*) {}
#endif
}
