// *****************************************************
//  Copyright 2024 Videonetics Technology Pvt Ltd
// *****************************************************

#include "logging.h"

#ifdef _WIN32
#include <process.h>
#else
// #include <execinfo.h>
#include <ctime>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

#include "ConfigFile.h"
#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
// #include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

constexpr int max_size      = 1048576 * 5;
constexpr int max_files     = 3;
constexpr int banner_spaces = 80;

namespace ray {

#define EL_RAY_FATAL_CHECK_FAILED "RAY_FATAL_CHECK_FAILED"

RayLogLevel RayLog::severity_threshold_ = RayLogLevel::INFO;
std::string RayLog::app_name_;
std::string RayLog::log_dir_;
// Format pattern is 2020-08-21 17:00:00,000 I 100 1001 msg.
// %L is loglevel, %P is process id, %t for thread id.
std::string RayLog::log_format_pattern_                  = "[%Y-%m-%d %H:%M:%S,%e %L %P %t] %v";
std::string RayLog::logger_name_                         = "ray_log_sink";
uint64_t    RayLog::log_rotation_max_size_               = (1 << 23);
int64_t     RayLog::log_rotation_file_num_               = 3;
bool        RayLog::is_failure_signal_handler_installed_ = false;

inline const char* ConstBasename(const char* filepath) {
  const char* base = strrchr(filepath, '/');
#ifdef OS_WINDOWS // Look for either path separator in Windows
  if (base != nullptr)
    base = strrchr(filepath, '\\');
#endif
  return (base != nullptr) ? (base + 1) : filepath;
}

/// A logger that prints logs to stderr.
/// This is the default logger if logging is not initialized.
/// NOTE(lingxuan.zlx): Default stderr logger must be singleton and global
/// variable so core worker process can invoke `RAY_LOG` in its whole lifecyle.
class DefaultStdErrLogger final {
public:
  std::shared_ptr<spdlog::logger> GetDefaultLogger() { return default_stderr_logger_; }

  static DefaultStdErrLogger& Instance() {
    static DefaultStdErrLogger instance;
    return instance;
  }
  ~DefaultStdErrLogger()                                      = default;
  DefaultStdErrLogger(DefaultStdErrLogger const&)             = delete;
  DefaultStdErrLogger(DefaultStdErrLogger&&)                  = delete;
  DefaultStdErrLogger& operator=(DefaultStdErrLogger&& other) = delete;
  DefaultStdErrLogger& operator=(DefaultStdErrLogger other)   = delete;

private:
  DefaultStdErrLogger() {
    default_stderr_logger_ = spdlog::stderr_color_mt("stderr");
    default_stderr_logger_->set_pattern(RayLog::GetLogFormatPattern());
  }
  std::shared_ptr<spdlog::logger> default_stderr_logger_;
};

class SpdLogMessage final {
public:
  explicit SpdLogMessage(const char* file, int line, int loglevel, std::shared_ptr<std::ostringstream> expose_osstream)
      : loglevel_(loglevel), expose_osstream_(std::move(expose_osstream)) {
    stream() << ConstBasename(file) << ":" << line << ": ";
  }

  inline void Flush() {
    auto logger = spdlog::get(RayLog::GetLoggerName());
    if (!logger) {
      logger = DefaultStdErrLogger::Instance().GetDefaultLogger();
    }

    if (loglevel_ == static_cast<int>(spdlog::level::critical)) {
      stream() << "\n*** StackTrace Information ***\n";
    }
    if (expose_osstream_) {
      *expose_osstream_ << "\n*** StackTrace Information ***\n";
    }
    // NOTE(lingxuan.zlx): See more fmt by visiting https://github.com/fmtlib/fmt.
    logger->log(static_cast<spdlog::level::level_enum>(loglevel_), /*fmt*/ "{}", str_.str());
    logger->flush();
  }

  SpdLogMessage(SpdLogMessage&&)                  = delete;
  SpdLogMessage(const SpdLogMessage&)             = delete;
  SpdLogMessage& operator=(const SpdLogMessage&)  = delete;
  SpdLogMessage& operator=(SpdLogMessage&& other) = delete;
  SpdLogMessage& operator=(SpdLogMessage other)   = delete;
  ~SpdLogMessage() { Flush(); }
  inline std::ostream& stream() { return str_; }

private:
  std::ostringstream                  str_;
  int                                 loglevel_;
  std::shared_ptr<std::ostringstream> expose_osstream_;
};

using LoggingProvider = ray::SpdLogMessage;

// Spdlog's severity map.
static int GetMappedSeverity(RayLogLevel severity) {
  switch (severity) {
  case RayLogLevel::TRACE:
    return spdlog::level::trace;
  case RayLogLevel::DEBUG:
    return spdlog::level::debug;
  case RayLogLevel::INFO:
    return spdlog::level::info;
  case RayLogLevel::WARNING:
    return spdlog::level::warn;
  case RayLogLevel::ERROR:
    return spdlog::level::err;
  case RayLogLevel::FATAL:
    return spdlog::level::critical;
  default:
    RAY_LOG(FATAL) << "Unsupported logging level: " << static_cast<int>(severity);
    // This return won't be hit but compiler needs it.
    return spdlog::level::off;
  }
}

std::vector<FatalLogCallback> RayLog::fatal_log_callbacks_;

void RayLog::StartRayLog(const std::string& app_name, RayLogLevel severity_threshold, const std::string& log_dir,
                         bool use_pid) {
  const char* var_value = getenv("RAY_BACKEND_LOG_LEVEL");
  if (var_value != nullptr) {
    std::string data = var_value;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    if (data == "trace") {
      severity_threshold = RayLogLevel::TRACE;
    } else if (data == "debug") {
      severity_threshold = RayLogLevel::DEBUG;
    } else if (data == "info") {
      severity_threshold = RayLogLevel::INFO;
    } else if (data == "warning") {
      severity_threshold = RayLogLevel::WARNING;
    } else if (data == "error") {
      severity_threshold = RayLogLevel::ERROR;
    } else if (data == "fatal") {
      severity_threshold = RayLogLevel::FATAL;
    } else {
      RAY_LOG(WARNING) << "Unrecognized setting of RAY_BACKEND_LOG_LEVEL=" << var_value;
    }
    RAY_LOG(INFO) << "Set ray log level from environment variable RAY_BACKEND_LOG_LEVEL"
                  << " to " << static_cast<int>(severity_threshold);
  }
  severity_threshold_ = severity_threshold;
  app_name_           = app_name;
  log_dir_            = log_dir;

  if (!log_dir_.empty()) {
    // Enable log file if log_dir_ is not empty.
    const std::string  dir_ends_with_slash   = log_dir_;
    const std::string& app_name_without_path = app_name;
#ifdef _WIN32
    int pid = use_pid ? _getpid() : 100;
#else
    const pid_t pid = use_pid ? getpid() : 100;
#endif
    // Reset log pattern and level and we assume a log file can be rotated with
    // 10 files in max size 512M by default.
    char* ray_rotation_max_bytes = getenv("RAY_ROTATION_MAX_BYTES");
    if (ray_rotation_max_bytes != nullptr) {
      auto max_size = std::atol(ray_rotation_max_bytes);
      // 0 means no log rotation in python, but not in spdlog. We just use the default
      // value here.
      if (max_size != 0) {
        log_rotation_max_size_ = max_size;
      }
    }
    char* ray_rotation_backup_count = getenv("RAY_ROTATION_BACKUP_COUNT");
    if (ray_rotation_backup_count != nullptr) {
      auto file_num = std::atol(ray_rotation_backup_count);
      if (file_num != 0) {
        log_rotation_file_num_ = file_num;
      }
    }
    spdlog::set_pattern(log_format_pattern_);
    spdlog::set_level(static_cast<spdlog::level::level_enum>(severity_threshold_));
    // Sink all log stuff to default file logger we defined here. We may need
    // multiple sinks for different files or loglevel.
    auto file_logger = spdlog::get(RayLog::GetLoggerName());
    if (file_logger) {
      // Drop this old logger first if we need reset filename or reconfig
      // logger.
      spdlog::drop(RayLog::GetLoggerName());
    }
    const std::string log_file = dir_ends_with_slash + app_name_without_path + "_" + std::to_string(pid) + ".log";
    std::cout << "\n\nLog at: " << log_file << '\n';
    file_logger =
        spdlog::rotating_logger_mt(RayLog::GetLoggerName(), log_file, log_rotation_max_size_, log_rotation_file_num_);
    spdlog::set_default_logger(file_logger);
  } else {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern(log_format_pattern_);
    auto level = static_cast<spdlog::level::level_enum>(severity_threshold_);
    console_sink->set_level(level);

    auto err_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    err_sink->set_pattern(log_format_pattern_);
    err_sink->set_level(spdlog::level::err);

    auto logger =
        std::make_shared<spdlog::logger>(RayLog::GetLoggerName(), spdlog::sinks_init_list({console_sink, err_sink}));

    logger->set_level(level);
    spdlog::set_default_logger(logger);
  }
}

void RayLog::UninstallSignalAction() {
  if (!is_failure_signal_handler_installed_) {
    return;
  }
  RAY_LOG(DEBUG) << "Uninstall signal handlers.";
  const std::vector<int> installed_signals({SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGTERM});
#ifdef _WIN32 // Do NOT use WIN32 (without the underscore); we want _WIN32 here
  for (int signal_num : installed_signals) {
    RAY_CHECK(signal(signal_num, SIG_DFL) != SIG_ERR);
  }
#else
  struct sigaction sig_action {};
  memset(&sig_action, 0, sizeof(sig_action));
  sigemptyset(&sig_action.sa_mask);
  sig_action.sa_handler = SIG_DFL;
  for (const int signal_num : installed_signals) {
    RAY_CHECK(sigaction(signal_num, &sig_action, nullptr) == 0);
  }
#endif
  is_failure_signal_handler_installed_ = false;
}

void RayLog::ShutDownRayLog() {
  UninstallSignalAction();
  if (spdlog::default_logger()) {
    spdlog::default_logger()->flush();
  }
  // NOTE(lingxuan.zlx) All loggers will be closed in shutdown but we don't need drop
  // console logger out because of some console logging might be used after shutdown ray
  // log. spdlog::shutdown();
}

void WriteFailureMessage(const char* data) {
  // The data & size represent one line failure message.
  // The second parameter `size-1` means we should strip last char `\n`
  // for pretty printing.
  if (nullptr != data) {
    RAY_LOG(ERROR) << std::string(data, strlen(data) - 1);
  }

  // If logger writes logs to files, logs are fully-buffered, which is different from
  // stdout (line-buffered) and stderr (unbuffered). So always flush here in case logs are
  // lost when logger writes logs to files.
  if (spdlog::default_logger()) {
    spdlog::default_logger()->flush();
  }
}

bool RayLog::IsFailureSignalHandlerEnabled() { return is_failure_signal_handler_installed_; }

bool RayLog::IsLevelEnabled(RayLogLevel log_level) { return log_level >= severity_threshold_; }

std::string RayLog::GetLogFormatPattern() { return log_format_pattern_; }

std::string RayLog::GetLoggerName() { return logger_name_; }

void RayLog::AddFatalLogCallbacks(const std::vector<FatalLogCallback>& expose_log_callbacks) {
  fatal_log_callbacks_.insert(fatal_log_callbacks_.end(), expose_log_callbacks.begin(), expose_log_callbacks.end());
}

RayLog::RayLog(const char* file_name, int line_number, RayLogLevel severity)
    : logging_provider_(nullptr), is_enabled_(severity >= severity_threshold_), severity_(severity),
      is_fatal_(severity == RayLogLevel::FATAL) {
  if (is_fatal_) {
    expose_osstream_ = std::make_shared<std::ostringstream>();
    *expose_osstream_ << file_name << ":" << line_number << ":";
  }
  if (is_enabled_) {
    logging_provider_ = new LoggingProvider(file_name, line_number, GetMappedSeverity(severity), expose_osstream_);
  }
}

std::ostream& RayLog::Stream() {
  auto* logging_provider = reinterpret_cast<LoggingProvider*>(logging_provider_);
  // Before calling this function, user should check IsEnabled.
  // When IsEnabled == false, logging_provider_ will be empty.
  return logging_provider->stream();
}

bool RayLog::IsEnabled() const { return is_enabled_; }

bool RayLog::IsFatal() const { return is_fatal_; }

std::ostream& RayLog::ExposeStream() { return *expose_osstream_; }

RayLog::~RayLog() {
  if (logging_provider_ != nullptr) {
    delete reinterpret_cast<LoggingProvider*>(logging_provider_);
    logging_provider_ = nullptr;
  }
  if (expose_osstream_ != nullptr) {
    for (const auto& callback : fatal_log_callbacks_) {
      callback(EL_RAY_FATAL_CHECK_FAILED, expose_osstream_->str());
    }
  }
  if (severity_ == RayLogLevel::FATAL) {
    std::_Exit(EXIT_FAILURE);
  }
}

} // namespace ray

std::string printable_git_info_safe(const std::string& git_details) {
  std::stringstream ss;
  ss << "\n-------------------------------------------------------------------------------\n";
  ss << git_details;
  ss << "\n";
  ss << get_current_time_str();
  ss << "\n-------------------------------------------------------------------------------\n";
  return ss.str();
}

std::string printable_git_info(const std::string& git_details) {
  // std::string git_details = fmt::format("{}_{}_[{}]", GIT_COMMIT_BRANCH, GIT_COMMIT_HASH, GIT_COMMIT_DATE);
  return fmt::format("\n┌{0:─^{2}}┐\n"
                     "│{1: ^{2}}│\n"
                     "│{3: ^{2}}│\n"
                     "└{0:─^{2}}┘\n",
                     "", get_current_time_str(), banner_spaces, git_details);
}

std::string printable_current_time() {
  return fmt::format("\n┌{0:─^{2}}┐\n"
                     "│{1: ^{2}}│\n"
                     "└{0:─^{2}}┘\n",
                     "", get_current_time_str(), banner_spaces);
}

std::shared_ptr<spdlog::logger> get_logger_st_internal(const std::string& logger_name, const std::string& logger_path) {
  std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
  if (logger == nullptr) {
    logger = spdlog::rotating_logger_st(logger_name, logger_path, max_size, max_files);
    logger->set_pattern("%v");
  }
  return logger;
}
std::shared_ptr<spdlog::logger> get_logger_st(const std::string& session_folder, const std::string& base_name,
                                              int16_t channel_id, int16_t app_id) {
  bool enable_logging = false;

  std::string logger_name = fmt::format("{}", base_name);
  {
    std::stringstream base_path_cnf;
    base_path_cnf << session_folder;
#if defined(_WIN32)
    base_path_cnf << "\\";
#else
    base_path_cnf << "/";
#endif
    base_path_cnf << logger_name;
    base_path_cnf << ".cnf";
    // Poco::Path base_path_cnf(session_folder);
    // base_path_cnf.append(fmt::format("{}.cnf", logger_name));
    ConfigFile f(base_path_cnf.str());
    auto       d = static_cast<double>(f.Value(base_name, logger_name, 1.0));
    if (d > 0) {
      enable_logging = true;
    }
    if ((channel_id != 0) || (app_id != 0)) {
      logger_name = fmt::format("{}_{}_{}", logger_name, channel_id, app_id);
    }
    d = static_cast<double>(f.Value(base_name, logger_name, 1.0));
    if (d > 0) {
      enable_logging = true;
    }
  }
  if (enable_logging) {
    std::stringstream base_path_log;
    base_path_log << session_folder;
#if defined(_WIN32)
    base_path_log << "\\";
#else
    base_path_log << "/";
#endif
    base_path_log << logger_name;
    base_path_log << ".log";
    // Poco::Path base_path_log(session_folder);
    // base_path_log.append(fmt::format("{}.log", logger_name));
    return get_logger_st_internal(logger_name, base_path_log.str());
  }
  return nullptr;
}
void write_header(std::shared_ptr<spdlog::logger> logger, const std::string& header_msg) {
  if (logger) {
    logger->info(printable_current_time());
    logger->info(header_msg);
  }
}
void write_log(std::shared_ptr<spdlog::logger> logger, const std::string& log_msg) {
  if (logger) {
    logger->info(log_msg);
    logger->flush();
  }
}
std::string get_current_time_str() {
  const std::time_t t = std::time(nullptr);
  std::stringstream ss;
  ss << fmt::format("UTC: {:%Y-%m-%d %H:%M:%S}", fmt::gmtime(t));
  return ss.str();
}