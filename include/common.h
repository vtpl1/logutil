// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef common_h
#define common_h
#ifdef VTPL_COMPILED_LIB
#    undef VTPL_HEADER_ONLY
#    if defined(VTPL_SHARED_LIB)
#        if defined(_WIN32)
#            ifdef VTPL_EXPORTS
#                define VTPL_API __declspec(dllexport)
#            else // !VTPL_EXPORTS
#                define VTPL_API __declspec(dllimport)
#            endif
#        else // !defined(_WIN32)
#            define VTPL_API __attribute__((visibility("default")))
#        endif
#    else // !defined(VTPL_SHARED_LIB)
#        define VTPL_API
#    endif
#    define VTPL_INLINE
#else // !defined(VTPL_COMPILED_LIB)
#    define VTPL_API
#    define VTPL_HEADER_ONLY
#    define VTPL_INLINE inline
#endif // #ifdef VTPL_COMPILED_LIB
#endif	// common_h
