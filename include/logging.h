#pragma once

// #include <gtest/gtest_prod.h>
#include <atomic>
#include <chrono> // chrono::system_clock
#include <ctime>  // localtime
#include <fmt/core.h>
#include <functional>
#include <iomanip> // put_time
#include <iostream>
#include <memory>
#include <spdlog/logger.h>
#include <sstream> // stringstream
#include <string>  // string
#include <vector>

#include <core_export.h>
#include <version.h>

#if defined(_WIN32)
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN // Sorry for the inconvenience. Please include any related
                            // headers you need manually.
                            // (https://stackoverflow.com/a/8294669)
#define WIN32_LEAN_AND_MEAN // Prevent inclusion of WinSock2.h
#endif
#include <Windows.h> // Force inclusion of WinGDI here to resolve name conflict
#endif
#ifdef ERROR // Should be true unless someone else undef'd it already
#undef ERROR // Windows GDI defines this macro; make it a global enum so it doesn't
             // conflict with our code
enum { ERROR = 0 };
#endif
#endif

#if defined(DEBUG) && DEBUG == 1
// Bazel defines the DEBUG macro for historical reasons:
// https://github.com/bazelbuild/bazel/issues/3513#issuecomment-323829248
// Undefine the DEBUG macro to prevent conflicts with our usage below
#undef DEBUG
// Redefine DEBUG as itself to allow any '#ifdef DEBUG' to keep working regardless
#define DEBUG DEBUG
#endif

namespace ray {
/// This function returns the current call stack information.
std::string GetCallTrace();

enum class RayLogLevel { TRACE = -2, DEBUG = -1, INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3 };

#define RAY_LOG_INTERNAL(level) ::ray::RayLog(__FILE__, __LINE__, level)

#define RAY_LOG_ENABLED(level) ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level)

#define RAY_LOG(level)                                                                                                 \
  if (ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level))                                                            \
  RAY_LOG_INTERNAL(ray::RayLogLevel::level)

#define RAY_LOG_TRC RAY_LOG(TRACE)
#define RAY_LOG_DBG RAY_LOG(DEBUG)
#define RAY_LOG_INF RAY_LOG(INFO)
#define RAY_LOG_WAR RAY_LOG(WARNING)
#define RAY_LOG_ERR RAY_LOG(ERROR)
#define RAY_LOG_FAT RAY_LOG(FATAL)

#define RAY_IGNORE_EXPR(expr) ((void)(expr))

#define RAY_CHECK(condition)                                                                                           \
  (condition) ? RAY_IGNORE_EXPR(0)                                                                                     \
              : ::ray::Voidify() & ::ray::RayLog(__FILE__, __LINE__, ray::RayLogLevel::FATAL)                          \
                                       << " Check failed: " #condition " "

#ifdef NDEBUG

#define RAY_DCHECK(condition)                                                                                          \
  (condition) ? RAY_IGNORE_EXPR(0)                                                                                     \
              : ::ray::Voidify() & ::ray::RayLog(__FILE__, __LINE__, ray::RayLogLevel::ERROR)                          \
                                       << " Debug check failed: " #condition " "
#else

#define RAY_DCHECK(condition) RAY_CHECK(condition)

#endif // NDEBUG

// RAY_LOG_EVERY_N/RAY_LOG_EVERY_MS, adaped from
// https://github.com/google/glog/blob/master/src/glog/logging.h.in
#define RAY_LOG_EVERY_N_VARNAME(base, line)        RAY_LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define RAY_LOG_EVERY_N_VARNAME_CONCAT(base, line) base##line

#define RAY_LOG_OCCURRENCES RAY_LOG_EVERY_N_VARNAME(occurrences_, __LINE__)

// Occasional logging, log every n'th occurrence of an event.
#define RAY_LOG_EVERY_N(level, n)                                                                                      \
  static std::atomic<uint64_t> RAY_LOG_OCCURRENCES(0);                                                                 \
  if (ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level) && RAY_LOG_OCCURRENCES.fetch_add(1) % n == 0)               \
  RAY_LOG_INTERNAL(ray::RayLogLevel::level) << "[" << RAY_LOG_OCCURRENCES << "] "

// Occasional logging with DEBUG fallback:
// If DEBUG is not enabled, log every n'th occurrence of an event.
// Otherwise, if DEBUG is enabled, always log as DEBUG events.
#define RAY_LOG_EVERY_N_OR_DEBUG(level, n)                                                                             \
  static std::atomic<uint64_t> RAY_LOG_OCCURRENCES(0);                                                                 \
  if (ray::RayLog::IsLevelEnabled(ray::RayLogLevel::DEBUG) ||                                                          \
      (ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level) && RAY_LOG_OCCURRENCES.fetch_add(1) % n == 0))             \
  RAY_LOG_INTERNAL(ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level) ? ray::RayLogLevel::level                      \
                                                                        : ray::RayLogLevel::DEBUG)                     \
      << "[" << RAY_LOG_OCCURRENCES << "] "

/// Macros for RAY_LOG_EVERY_MS
#define RAY_LOG_TIME_PERIOD       RAY_LOG_EVERY_N_VARNAME(timePeriod_, __LINE__)
#define RAY_LOG_PREVIOUS_TIME_RAW RAY_LOG_EVERY_N_VARNAME(previousTimeRaw_, __LINE__)
#define RAY_LOG_TIME_DELTA        RAY_LOG_EVERY_N_VARNAME(deltaTime_, __LINE__)
#define RAY_LOG_CURRENT_TIME      RAY_LOG_EVERY_N_VARNAME(currentTime_, __LINE__)
#define RAY_LOG_PREVIOUS_TIME     RAY_LOG_EVERY_N_VARNAME(previousTime_, __LINE__)

#define RAY_LOG_EVERY_MS(level, ms)                                                                                    \
  constexpr std::chrono::milliseconds  RAY_LOG_TIME_PERIOD(ms);                                                        \
  static std::atomic<int64_t>          RAY_LOG_PREVIOUS_TIME_RAW;                                                      \
  const auto                           RAY_LOG_CURRENT_TIME = std::chrono::steady_clock::now().time_since_epoch();     \
  const decltype(RAY_LOG_CURRENT_TIME) RAY_LOG_PREVIOUS_TIME(                                                          \
      RAY_LOG_PREVIOUS_TIME_RAW.load(std::memory_order_relaxed));                                                      \
  const auto RAY_LOG_TIME_DELTA = RAY_LOG_CURRENT_TIME - RAY_LOG_PREVIOUS_TIME;                                        \
  if (RAY_LOG_TIME_DELTA > RAY_LOG_TIME_PERIOD)                                                                        \
    RAY_LOG_PREVIOUS_TIME_RAW.store(RAY_LOG_CURRENT_TIME.count(), std::memory_order_relaxed);                          \
  if (ray::RayLog::IsLevelEnabled(ray::RayLogLevel::level) && RAY_LOG_TIME_DELTA > RAY_LOG_TIME_PERIOD)                \
  RAY_LOG_INTERNAL(ray::RayLogLevel::level)

// To make the logging lib plugable with other logging libs and make
// the implementation unawared by the user, RayLog is only a declaration
// which hide the implementation into logging.cc file.
// In logging.cc, we can choose different log libs using different macros.

// This is also a null log which does not output anything.
class RayLogBase {
public:
  virtual ~RayLogBase(){};

  // By default, this class is a null log because it return false here.
  virtual bool IsEnabled() const { return false; };

  // This function to judge whether current log is fatal or not.
  virtual bool IsFatal() const { return false; };

  template <typename T> RayLogBase& operator<<(const T& t) {
    if (IsEnabled()) {
      Stream() << t;
    }
    if (IsFatal()) {
      ExposeStream() << t;
    }
    return *this;
  }

protected:
  virtual std::ostream& Stream() { return std::cerr; };
  virtual std::ostream& ExposeStream() { return std::cerr; };
};

/// Callback function which will be triggered to expose fatal log.
/// The first argument: a string representing log type or label.
/// The second argument: log content.
using FatalLogCallback = std::function<void(const std::string&, const std::string&)>;

class CORE_EXPORT RayLog : public RayLogBase {
public:
  RayLog(const char* file_name, int line_number, RayLogLevel severity);

  virtual ~RayLog();

  /// Return whether or not current logging instance is enabled.
  ///
  /// \return True if logging is enabled and false otherwise.
  virtual bool IsEnabled() const;

  virtual bool IsFatal() const;

  /// The init function of ray log for a program which should be called only once.
  ///
  /// \parem appName The app name which starts the log.
  /// \param severity_threshold Logging threshold for the program.
  /// \param logDir Logging output file name. If empty, the log won't output to file.
  static void StartRayLog(const std::string& app_name, RayLogLevel severity_threshold = RayLogLevel::INFO,
                          const std::string& log_dir = "", bool use_pid = true);

  /// The shutdown function of ray log which should be used with StartRayLog as a pair.
  static void ShutDownRayLog();

  /// Uninstall the signal actions installed by InstallFailureSignalHandler.
  static void UninstallSignalAction();

  /// Return whether or not the log level is enabled in current setting.
  ///
  /// \param log_level The input log level to test.
  /// \return True if input log level is not lower than the threshold.
  static bool IsLevelEnabled(RayLogLevel log_level);

  /// Install the failure signal handler to output call stack when crash.
  static void InstallFailureSignalHandler();

  /// To check failure signal handler enabled or not.
  static bool IsFailureSignalHandlerEnabled();

  /// Get the log level from environment variable.
  static RayLogLevel GetLogLevelFromEnv();

  static std::string GetLogFormatPattern();

  static std::string GetLoggerName();

  /// Add callback functions that will be triggered to expose fatal log.
  static void AddFatalLogCallbacks(const std::vector<FatalLogCallback>& expose_log_callbacks);

private:
  // FRIEND_TEST(PrintLogTest, TestRayLogEveryNOrDebug);
  // FRIEND_TEST(PrintLogTest, TestRayLogEveryN);
  // Hide the implementation of log provider by void *.
  // Otherwise, lib user may define the same macro to use the correct header file.
  void* logging_provider_;
  /// True if log messages should be logged and false if they should be ignored.
  bool is_enabled_;
  /// log level.
  RayLogLevel severity_;
  /// Whether current log is fatal or not.
  bool is_fatal_ = false;
  /// String stream of exposed log content.
  std::shared_ptr<std::ostringstream> expose_osstream_ = nullptr;
  /// Callback functions which will be triggered to expose fatal log.
  static std::vector<FatalLogCallback> fatal_log_callbacks_;
  static RayLogLevel                   severity_threshold_;
  // In InitGoogleLogging, it simply keeps the pointer.
  // We need to make sure the app name passed to InitGoogleLogging exist.
  static std::string app_name_;
  /// The directory where the log files are stored.
  /// If this is empty, logs are printed to stdout.
  static std::string log_dir_;
  /// This flag is used to avoid calling UninstallSignalAction in ShutDownRayLog if
  /// InstallFailureSignalHandler was not called.
  static bool is_failure_signal_handler_installed_;
  // Log format content.
  static std::string log_format_pattern_;
  // Log rotation file size limitation.
  static uint64_t log_rotation_max_size_;
  // Log rotation file number.
  static int64_t log_rotation_file_num_;
  // Ray default logger name.
  static std::string logger_name_;

protected:
  virtual std::ostream& Stream();
  virtual std::ostream& ExposeStream();
};

// This class make RAY_CHECK compilation pass to change the << operator to void.
class Voidify {
public:
  Voidify() {}
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(RayLogBase&) {}
};

} // namespace ray

std::string CORE_EXPORT printable_git_info_safe(const std::string& git_details);
std::string CORE_EXPORT printable_git_info(const std::string& git_details);

std::shared_ptr<spdlog::logger> CORE_EXPORT get_logger_st(const std::string& session_folder,
                                                          const std::string& base_name, int16_t channel_id = 0,
                                                          int16_t app_id = 0);
void CORE_EXPORT        write_header(std::shared_ptr<spdlog::logger> logger, const std::string& header_msg);
void CORE_EXPORT        write_log(std::shared_ptr<spdlog::logger> logger, const std::string& log_msg);
std::string CORE_EXPORT get_current_time_str();