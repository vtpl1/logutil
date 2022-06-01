# *****************************************************
#    Copyright 2022 Videonetics Technology Pvt Ltd
# *****************************************************
cmake_minimum_required(VERSION 3.15)

set(logutil_VERSION 1.0.7)

project(logutil
	VERSION ${logutil_VERSION}
	DESCRIPTION "A simple wrapper around spdlog"
	HOMEPAGE_URL "https://github.com/vtpl1/logutil"
	LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Poco REQUIRED Foundation)

# set(BUILD_SHARED_LIBS TRUE)
add_library(logutil
    src/Chameleon.cpp
    src/ConfigFile.cpp
    src/logging.cpp

    inc/Chameleon.h
    inc/ConfigFile.h
    include/logging.h)
add_library(logutil::logutil ALIAS logutil)

if(BUILD_SHARED_LIBS)
target_compile_definitions(logutil
	PRIVATE LOGUTIL_EXPORTS
	PRIVATE BUILD_SHARED_LIBS)
endif()

target_compile_features(logutil PUBLIC cxx_std_17)
target_include_directories(logutil
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)
target_link_libraries(logutil
	PUBLIC spdlog::spdlog
	PUBLIC fmt::fmt
	PRIVATE Poco::Foundation)

find_package(Git)
if(Git_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=40
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_COMMIT_HASH)
	message(STATUS gch: ${GIT_COMMIT_HASH})
	target_compile_definitions(logutil
		PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_COMMIT_BRANCH)
	message(STATUS gbr: ${GIT_COMMIT_BRANCH})
	target_compile_definitions(logutil
		PRIVATE GIT_COMMIT_BRANCH="${GIT_COMMIT_BRANCH}")

	# the date of the commit
	execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad --date=local
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET OUTPUT_VARIABLE GIT_COMMIT_DATE)
	target_compile_definitions(logutil
		PRIVATE GIT_COMMIT_DATE="${GIT_COMMIT_DATE}")
endif() #git

## Install library + targets ##################################################

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

install(TARGETS logutil
	EXPORT logutil-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    	NAMELINK_SKIP
	# on Windows put the dlls into bin
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	# ... and the import lib into the devel package
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT logutil-targets
	NAMESPACE logutil::
	DESTINATION ${CMAKE_INSTALL_DATADIR}/logutil)
install(TARGETS logutil
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		NAMELINK_ONLY
	# on Windows put the dlls into bin
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	# ... and the import lib into the devel package
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
## Install headers ############################################################
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
