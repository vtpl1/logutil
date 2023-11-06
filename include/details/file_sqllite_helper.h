// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef file_sqllite_helper_h
#define file_sqllite_helper_h
#include <spdlog/common.h>
#include <tuple>

namespace vtpl
{
namespace details
{

// Helper class for sqlite file sinks.
// When failing to open a file, retry several times(5) with a delay interval(10 ms).
// Throw spdlog_ex exception on errors.
class SPDLOG_API file_sqllite_helper
{
public:
  file_sqllite_helper() = default;
  explicit file_sqllite_helper(const spdlog::file_event_handlers& event_handlers);

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
    std::FILE *fd_{nullptr};
    spdlog::filename_t filename_;
    spdlog::file_event_handlers event_handlers_;
};
} // namespace details
} // namespace vtpl
#endif // file_sqllite_helper_h
