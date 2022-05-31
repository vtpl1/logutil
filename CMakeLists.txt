# *****************************************************
#    Copyright 2022 Videonetics Technology Pvt Ltd
# *****************************************************
cmake_minimum_required(VERSION 3.15)

set(logutil_VERSION 1.0.0)

project(logutil
	VERSION ${logutil_VERSION}
	DESCRIPTION "A simple wrapper around spdlog"
	HOMEPAGE_URL "https://github.com/vtpl1/logutil"
	LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(spdlog REQUIRED)

add_library(logutil
    src/Chameleon.cpp
    src/ConfigFile.cpp
    src/logging.cpp

    inc/Chameleon.h
    inc/ConfigFile.h
    inc/logging.h)
add_library(logutil::logutil ALIAS logutil)

if(BUILD_SHARED_LIBS AND WIN32)
	target_compile_definitions(logutil
		PRIVATE "LOGUTIL_EXPORT=__declspec(dllexport)"
		INTERFACE "LOGUTIL_EXPORT=__declspec(dllimport)")
endif()

target_compile_features(logutil PUBLIC cxx_std_17)
target_include_directories(logutil
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(logutil
	PUBLIC spdlog::spdlog)

configure_package_config_file(cmake/config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/logutil-config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/logutil
	NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/logutil-config-version.cmake
	VERSION ${logutil_VERSION}
	COMPATIBILITY SameMajorVersion)
install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/logutil-config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/logutil-config-version.cmake
	DESTINATION
		${CMAKE_INSTALL_DATADIR}/logutil)

install(TARGETS logutil EXPORT logutil-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT logutil-targets
	NAMESPACE logutil::
	DESTINATION ${CMAKE_INSTALL_DATADIR}/logutil)