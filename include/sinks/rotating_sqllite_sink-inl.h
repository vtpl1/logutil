// *****************************************************
//  Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef rotating_sqllite_sink_inl_h
#define rotating_sqllite_sink_inl_h

#ifndef SPDLOG_HEADER_ONLY
#include "rotating_sqllite_sink.h"
#endif

#include <spdlog/common.h>

#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>

#include <spdlog/fmt/fmt.h>

#include <cerrno>
#include <chrono>
#include <ctime>
#include <mutex>
#include <string>
#include <tuple>
namespace vtpl
{
namespace sinks
{
template <typename Mutex>
SPDLOG_INLINE rotating_sqllite_sink<Mutex>::rotating_sqllite_sink(spdlog::filename_t base_filename,
                                                                  std::size_t max_size, std::size_t max_files,
                                                                  bool rotate_on_open,
                                                                  const vtpl::sqllite_event_handlers& event_handlers)
    : base_filename_(std::move(base_filename)), max_size_(max_size), max_files_(max_files),
      file_sqllite_helper_{event_handlers}
{
  if (max_size == 0) {
    spdlog::throw_spdlog_ex("rotating sink constructor: max_size arg cannot be zero");
  }

  if (max_files > 200000) {
    spdlog::throw_spdlog_ex("rotating sink constructor: max_files arg cannot exceed 200000");
  }
  file_sqllite_helper_.open(calc_filename(base_filename_, 0));
  current_size_ = file_sqllite_helper_.size(); // expensive. called only once
  if (rotate_on_open && current_size_ > 0) {
    rotate_();
    current_size_ = 0;
  }
}

// calc filename according to index and file extension if exists.
// e.g. calc_filename("logs/mylog.txt, 3) => "logs/mylog.3.txt".
template <typename Mutex>
SPDLOG_INLINE spdlog::filename_t rotating_sqllite_sink<Mutex>::calc_filename(const spdlog::filename_t& filename,
                                                                             std::size_t index)
{
  if (index == 0u) {
    return filename;
  }

  spdlog::filename_t basename, ext;
  std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(filename);
  return spdlog::fmt_lib::format(SPDLOG_FILENAME_T("{}.{}{}"), basename, index, ext);
}

template <typename Mutex>
SPDLOG_INLINE spdlog::filename_t rotating_sqllite_sink<Mutex>::filename()
{
  std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
  return file_sqllite_helper_.filename();
}

template <typename Mutex>
SPDLOG_INLINE void rotating_sqllite_sink<Mutex>::sink_it_(const spdlog::details::log_msg& msg)
{
  spdlog::memory_buf_t formatted;
  spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
  auto new_size = current_size_ + formatted.size();

  // rotate if the new estimated file size exceeds max size.
  // rotate only if the real size > 0 to better deal with full disk (see issue #2261).
  // we only check the real size when new_size > max_size_ because it is relatively expensive.
  if (new_size > max_size_) {
    file_sqllite_helper_.flush();
    if (file_sqllite_helper_.size() > 0) {
      rotate_();
      new_size = formatted.size();
    }
  }
  file_sqllite_helper_.write(formatted);
  current_size_ = new_size;
}

template <typename Mutex>
SPDLOG_INLINE void rotating_sqllite_sink<Mutex>::flush_()
{
  file_sqllite_helper_.flush();
}

// Rotate files:
// log.txt -> log.1.txt
// log.1.txt -> log.2.txt
// log.2.txt -> log.3.txt
// log.3.txt -> delete
template <typename Mutex>
SPDLOG_INLINE void rotating_sqllite_sink<Mutex>::rotate_()
{
  using spdlog::details::os::filename_to_str;
  using spdlog::details::os::path_exists;
  file_sqllite_helper_.close();
  for (auto i = max_files_; i > 0; --i) {
    spdlog::filename_t src = calc_filename(base_filename_, i - 1);
    if (!path_exists(src)) {
      continue;
    }
    spdlog::filename_t target = calc_filename(base_filename_, i);

    if (!rename_file_(src, target)) {
      // if failed try again after a small delay.
      // this is a workaround to a windows issue, where very high rotation
      // rates can cause the rename to fail with permission denied (because of antivirus?).
      spdlog::details::os::sleep_for_millis(100);
      if (!rename_file_(src, target)) {
        file_sqllite_helper_.reopen(true); // truncate the log file anyway to prevent it to grow beyond its limit!
        current_size_ = 0;
        spdlog::throw_spdlog_ex(
            "rotating_sqllite_sink: failed renaming " + filename_to_str(src) + " to " + filename_to_str(target), errno);
      }
    }
  }
  file_sqllite_helper_.reopen(true);
}

// delete the target if exists, and rename the src file  to target
// return true on success, false otherwise.
template <typename Mutex>
SPDLOG_INLINE bool rotating_sqllite_sink<Mutex>::rename_file_(const spdlog::filename_t& src_filename,
                                                              const spdlog::filename_t& target_filename)
{
  // try to delete the target file in case it already exists.
  (void)spdlog::details::os::remove(target_filename);
  return spdlog::details::os::rename(src_filename, target_filename) == 0;
}

} // namespace sinks
} // namespace vtpl
#endif // rotating_sqllite_sink_inl_h
