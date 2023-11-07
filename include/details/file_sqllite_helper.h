// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef file_sqllite_helper_h
#define file_sqllite_helper_h

#include "common.h"

#include <spdlog/common.h>
#include <sqlite3.h>
#include <tuple>

namespace vtpl
{
struct sqllite_event_handlers {
  sqllite_event_handlers() : before_open(nullptr), after_open(nullptr), before_close(nullptr), after_close(nullptr) {}

  std::function<void(const spdlog::filename_t& filename)> before_open;
  std::function<void(const spdlog::filename_t& filename, sqlite3* file_stream)> after_open;
  std::function<void(const spdlog::filename_t& filename, sqlite3* file_stream)> before_close;
  std::function<void(const spdlog::filename_t& filename)> after_close;
};
namespace details
{

// Helper class for sqlite file sinks.
// When failing to open a file, retry several times(5) with a delay interval(10 ms).
// Throw spdlog_ex exception on errors.
class VTPL_API file_sqllite_helper
{
public:
  file_sqllite_helper() = default;
  explicit file_sqllite_helper(const vtpl::sqllite_event_handlers& event_handlers);

  file_sqllite_helper(const file_sqllite_helper&) = delete;
  file_sqllite_helper& operator=(const file_sqllite_helper&) = delete;
  ~file_sqllite_helper();

  void open(const spdlog::filename_t& fname, bool truncate = false);
  void reopen(bool truncate);
  void flush();
  void sync();
  void close();
  void write(const spdlog::memory_buf_t& buf);
  size_t size() const;
  const spdlog::filename_t& filename() const;

private:
  const int open_tries_ = 5;
  const unsigned int open_interval_ = 10;
  // sqlite3 *db_{nullptr};
  sqlite3* fd_{nullptr};
  // std::FILE *fd_{nullptr};
  spdlog::filename_t filename_;
  vtpl::sqllite_event_handlers event_handlers_;
};
} // namespace details
} // namespace vtpl

#ifdef VTPL_HEADER_ONLY
    #include "file_sqllite_helper-inl.h"
#endif

#endif // file_sqllite_helper_h
