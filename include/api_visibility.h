// *****************************************************
//    Copyright 2022 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef api_visibility_h
#define api_visibility_h
#if defined(_WIN32)
#if defined(BUILD_SHARED_LIBS)
#if defined(LOGUTIL_EXPORTS)
#define LOGUTIL_API __declspec(dllexport)
#else
#define LOGUTIL_API __declspec(dllimport)
#endif
#else
#define LOGUTIL_API
#endif
#elif !defined(LOGUTIL_NO_GCC_API_ATTRIBUTE) && defined(__GNUC__) && (__GNUC__ >= 4)
#define LOGUTIL_API __attribute__((visibility("default")))
#else
#define LOGUTIL_API
#endif

#endif // api_visibility_h
