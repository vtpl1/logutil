// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef file_sqllite_helper_inl_h
#define file_sqllite_helper_inl_h

#ifndef SPDLOG_HEADER_ONLY
#include "details/file_sqllite_helper.h"
#endif

#include <spdlog/common.h>
#include <spdlog/details/os.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <tuple>

namespace vtpl
{
namespace details
{
SPDLOG_INLINE file_sqllite_helper::file_sqllite_helper(const vtpl::sqllite_event_handlers& event_handlers)
    : event_handlers_(event_handlers)
{
}
SPDLOG_INLINE file_sqllite_helper::~file_sqllite_helper() { close(); }

SPDLOG_INLINE void file_sqllite_helper::open(const spdlog::filename_t& fname, bool truncate)
{
  close();
  filename_ = fname;

  auto* mode = SPDLOG_FILENAME_T("ab");
  auto* trunc_mode = SPDLOG_FILENAME_T("wb");

  if (event_handlers_.before_open) {
    event_handlers_.before_open(filename_);
  }

  for (int tries = 0; tries < open_tries_; ++tries) {
    // create containing folder if not exists already.
    spdlog::details::os::create_dir(spdlog::details::os::dir_name(fname));
    if (truncate) {
      // Truncate by opening-and-closing a tmp file in "wb" mode, always
      // opening the actual log-we-write-to in "ab" mode, since that
      // interacts more politely with eternal processes that might
      // rotate/truncate the file underneath us.
      std::FILE* tmp;
      if (spdlog::details::os::fopen_s(&tmp, fname, trunc_mode)) {
        continue;
      }
      std::fclose(tmp);
    }
    if (!sqlite3_open(fname.c_str(), &fd_)) {
      if (event_handlers_.after_open) {
        event_handlers_.after_open(filename_, fd_);
      }
      return;
    }
    spdlog::details::os::sleep_for_millis(open_interval_);
  }
  spdlog::throw_spdlog_ex("Failed opening file " + spdlog::details::os::filename_to_str(filename_) + " for writing",
                          errno);
}

SPDLOG_INLINE void file_sqllite_helper::reopen(bool truncate)
{
  if (filename_.empty()) {
    spdlog::throw_spdlog_ex("Failed re opening file - was not opened before");
  }
  this->open(filename_, truncate);
}

SPDLOG_INLINE void file_sqllite_helper::flush()
{
  // if (std::fflush(fd_) != 0) {
  //   spdlog::throw_spdlog_ex("Failed flush to file " + spdlog::details::os::filename_to_str(filename_), errno);
  // }
}

SPDLOG_INLINE void file_sqllite_helper::sync()
{
  // if (!spdlog::details::os::fsync(fd_)) {
  //   spdlog::throw_spdlog_ex("Failed to fsync file " + spdlog::details::os::filename_to_str(filename_), errno);
  // }
}

SPDLOG_INLINE void file_sqllite_helper::close()
{
  if (fd_ != nullptr) {
    if (event_handlers_.before_close) {
      event_handlers_.before_close(filename_, fd_);
    }

    sqlite3_close(fd_);
    fd_ = nullptr;

    if (event_handlers_.after_close) {
      event_handlers_.after_close(filename_);
    }
  }
}

SPDLOG_INLINE void file_sqllite_helper::write(const spdlog::memory_buf_t& buf)
{
  size_t msg_size = buf.size();
  auto data = buf.data();
  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> here\n");

  sqlite3_exec(fd_, "insert into Test (id, name) VALUES (1, 'Soumyadip santra');", NULL, NULL, NULL);

  // if (std::fwrite(data, 1, msg_size, fd_) != msg_size) {
  //   spdlog::throw_spdlog_ex("Failed writing to file " + spdlog::details::os::filename_to_str(filename_), errno);
  // }
}

int size_callback(void* size, int count, char** data, char** columns)
{
  int idx;

  printf("There are %d column(s)\n", count);

  for (idx = 0; idx < count; idx++) {
    printf("The data in column \"%s\" is: %s\n", columns[idx], data[idx]);
  }

  size = *data;

  printf("\n");

  return 0;
}

SPDLOG_INLINE size_t file_sqllite_helper::size() const
{
  if (fd_ == nullptr) {
    spdlog::throw_spdlog_ex("Cannot use size() on closed file " + spdlog::details::os::filename_to_str(filename_));
  }

  char* size;
  sqlite3_exec(fd_, "select * from pragma_page_size();", size_callback, size, NULL);

  return atoi(size);
}

SPDLOG_INLINE const spdlog::filename_t& file_sqllite_helper::filename() const { return filename_; }

} // namespace details
} // namespace vtpl
#endif // file_sqllite_helper_inl_h
