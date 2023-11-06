// *****************************************************
//  Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#ifndef SPDLOG_COMPILED_LIB
#error Please define SPDLOG_COMPILED_LIB to compile this file.
#endif

#include "sinks/rotating_sqllite_sink.h"
#include "details/file_sqllite_helper-inl.h"
#include <spdlog/details/null_mutex.h>

#include <mutex>

#include "sinks/rotating_sqllite_sink-inl.h"
template class SPDLOG_API vtpl::sinks::rotating_sqllite_sink<std::mutex>;
template class SPDLOG_API vtpl::sinks::rotating_sqllite_sink<spdlog::details::null_mutex>;
