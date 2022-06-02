# *****************************************************
#    Copyright 2022 Videonetics Technology Pvt Ltd
# *****************************************************
cmake_minimum_required(VERSION 3.15)

set(VERSION 1.0.8)

project(logutil
	VERSION ${VERSION}
	DESCRIPTION "A simple wrapper around spdlog"
	HOMEPAGE_URL "https://github.com/vtpl1/logutil"
	LANGUAGES CXX)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Poco REQUIRED Foundation)

set(BUILD_SHARED_LIBS TRUE)
add_library(${PROJECT_NAME}
    src/Chameleon.cpp
    src/ConfigFile.cpp
    src/logging.cpp

    inc/Chameleon.h
    inc/ConfigFile.h
    include/logging.h)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/logging.h)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

if(BUILD_SHARED_LIBS)
target_compile_definitions(${PROJECT_NAME}
	PRIVATE LOGUTIL_EXPORTS
	PRIVATE BUILD_SHARED_LIBS)
endif()

include(GNUInstallDirs)

# target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)
target_link_libraries(${PROJECT_NAME}
	PUBLIC spdlog::spdlog
	PUBLIC fmt::fmt
	PRIVATE Poco::Foundation)

find_package(Git)
if(Git_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=40
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_COMMIT_HASH)
    message(STATUS gch: ${GIT_COMMIT_HASH})
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GIT_COMMIT_BRANCH)
    message(STATUS gbr: ${GIT_COMMIT_BRANCH})
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE GIT_COMMIT_BRANCH="${GIT_COMMIT_BRANCH}")

    # the date of the commit
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad --date=local
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET OUTPUT_VARIABLE GIT_COMMIT_DATE)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE GIT_COMMIT_DATE="${GIT_COMMIT_DATE}")
endif() #git



## Install library + targets ##################################################
include(CMakePackageConfigHelpers)

add_library(${PROJECT_NAME}::core INTERFACE IMPORTED)
target_link_libraries(${PROJECT_NAME}::core INTERFACE ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    	NAMELINK_SKIP
	# on Windows put the dlls into bin
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	# ... and the import lib into the devel package
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(EXPORT  ${PROJECT_NAME}-targets
	DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
	NAMESPACE  ${PROJECT_NAME}::)

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		NAMELINK_ONLY
	# on Windows put the dlls into bin
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	# ... and the import lib into the devel package
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}

	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/config.cmake.in
	${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
	NO_SET_AND_CHECK_MACRO)

write_basic_package_version_file(
    ${PROJECT_NAME}-config-version.cmake
    VERSION ${VERSION}
    COMPATIBILITY SameMajorVersion)

install(
    FILES
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME})