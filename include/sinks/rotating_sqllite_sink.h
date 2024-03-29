// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************
#pragma once
#ifndef rotating_sqllite_sink_h
#define rotating_sqllite_sink_h
#include "common.h"
#include "details/file_sqllite_helper.h"
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>

namespace vtpl
{
namespace sinks
{
template <typename Mutex>
class rotating_sqllite_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
  rotating_sqllite_sink(spdlog::filename_t base_filename, std::size_t max_size, std::size_t max_files,
                        bool rotate_on_open = false, const vtpl::sqllite_event_handlers& event_handlers = {});
  static spdlog::filename_t calc_filename(const spdlog::filename_t& filename, std::size_t index);
  spdlog::filename_t filename();

  ~rotating_sqllite_sink() { flush_(); }

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override;
  void flush_() override;

private:
  // Rotate files:
  // log.txt -> log.1.txt
  // log.1.txt -> log.2.txt
  // log.2.txt -> log.3.txt
  // log.3.txt -> delete
  void rotate_();

  // delete the target if exists, and rename the src file  to target
  // return true on success, false otherwise.
  bool rename_file_(const spdlog::filename_t& src_filename, const spdlog::filename_t& target_filename);

  spdlog::filename_t base_filename_;
  std::size_t max_size_;
  std::size_t max_files_;
  std::size_t current_size_;
  vtpl::details::file_sqllite_helper file_sqllite_helper_;
};

using rotating_sqllite_sink_mt = rotating_sqllite_sink<std::mutex>;
using rotating_sqllite_sink_st = rotating_sqllite_sink<spdlog::details::null_mutex>;
} // namespace sinks
//
// factory functions
//
template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger>
rotating_sqllite_logger_mt(const std::string& logger_name, const spdlog::filename_t& filename, size_t max_file_size,
                           size_t max_files, bool rotate_on_open = false,
                           const vtpl::sqllite_event_handlers& event_handlers = {})
{
  return Factory::template create<vtpl::sinks::rotating_sqllite_sink_mt>(logger_name, filename, max_file_size, max_files,
                                                                   rotate_on_open, event_handlers);
}
template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger>
rotating_sqllite_logger_st(const std::string& logger_name, const spdlog::filename_t& filename, size_t max_file_size,
                           size_t max_files, bool rotate_on_open = false,
                           const vtpl::sqllite_event_handlers& event_handlers = {})
{
  return Factory::template create<vtpl::sinks::rotating_sqllite_sink_st>(logger_name, filename, max_file_size, max_files,
                                                                   rotate_on_open, event_handlers);
}
} // namespace vtpl
#ifdef VTPL_HEADER_ONLY
    #include "rotating_sqllite_sink-inl.h"
#endif

#endif // rotating_sqllite_sink_h
