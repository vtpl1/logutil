// *****************************************************
//  Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************
#include "common.h"

#ifndef VTPL_COMPILED_LIB
#error Please define SPDLOG_COMPILED_LIB to compile this file.
#endif

#include "sinks/rotating_sqllite_sink.h"
#include <mutex>
#include <spdlog/details/null_mutex.h>

#include "details/file_sqllite_helper-inl.h"
#include "sinks/rotating_sqllite_sink-inl.h"
template class VTPL_API vtpl::sinks::rotating_sqllite_sink<std::mutex>;
template class VTPL_API vtpl::sinks::rotating_sqllite_sink<spdlog::details::null_mutex>;
