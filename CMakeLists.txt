##-----------------------------------------------------------------------------
##  Welcome to Streamripper
##-----------------------------------------------------------------------------
project (STREAMRIPPER)
cmake_minimum_required (VERSION 2.8.12)

## Tell CMake to be quiet
cmake_policy (SET CMP0003 NEW)

## Default to release
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

##-----------------------------------------------------------------------------
##  Include files
##-----------------------------------------------------------------------------
include (CheckFunctionExists)
include (CheckLibraryExists)
include (FindPkgConfig)

# http://www.cmake.org/pipermail/cmake/2008-December/025886.html
include (InstallRequiredSystemLibraries)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -D__UNIX__ -fno-builtin")

##-----------------------------------------------------------------------------
##  Find libraries
##-----------------------------------------------------------------------------
set (MATH_LIB -lm)

pkg_check_modules (GLIB2 REQUIRED glib-2.0>=2.16)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package (IBERTY)
find_package (MAD)
find_package (OGG)
find_package (Threads)
find_package (VORBIS)

include (${CMAKE_SOURCE_DIR}/cmake/SR_FindSockets.cmake)

find_library (SEMAPHORE_LIBRARIES NAMES sem rt)

##-----------------------------------------------------------------------------
##  Include directories
##-----------------------------------------------------------------------------
include_directories (BEFORE ${GLIB2_INCLUDE_DIRS})
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

##-----------------------------------------------------------------------------
##  Configure include file
##-----------------------------------------------------------------------------
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sr_config.h.cmake 
    ${CMAKE_CURRENT_BINARY_DIR}/sr_config.h)

##-----------------------------------------------------------------------------
##  Build subdirectories
##-----------------------------------------------------------------------------
add_subdirectory (lib)

##-----------------------------------------------------------------------------
##  Source files
##-----------------------------------------------------------------------------
set (STREAMRIPPER_SRC
    test/streamripper.c
    )

set (SRD_SRC
    daemon/main.c
    daemon/config1.c
    )
##-----------------------------------------------------------------------------
##  Targets
##-----------------------------------------------------------------------------
message ("MAD_LIBRARIES = ${MAD_LIBRARIES}")
set (STREAMRIPPER_LIBS ${MAD_LIBRARIES} ${GLIB2_LIBRARIES})

if (SEMAPHORE_LIBRARIES)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SEMAPHORE_LIBRARIES})
endif ()

if (IBERTY_FOUND)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${IBERTY_LIBRARIES})
endif ()

if (OGG_FOUND AND VORBIS_FOUND)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} 
	${OGG_LIBRARIES} ${VORBIS_LIBRARIES})
endif ()

if (CMAKE_THREAD_LIBS_INIT)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif ()

set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SR_EXTRA_LIBS} ${MATH_LIB})

##-----------------------------------------------------------------------------
##  Linking
##-----------------------------------------------------------------------------
set (STREAMRIPPER_LDFLAGS "")

add_executable (streamripper ${STREAMRIPPER_SRC})
target_link_libraries (streamripper 
    streamripper1 ${STREAMRIPPER_LIBS})
if (NOT STREAMRIPPER_LDFLAGS STREQUAL "")
    set_target_properties (streamripper 
	PROPERTIES LINK_FLAGS ${STREAMRIPPER_LDFLAGS})
endif ()

add_executable (streamripperd ${SRD_SRC})
target_link_libraries (streamripperd 
    streamripper1 ${STREAMRIPPER_LIBS})
if (NOT STREAMRIPPER_LDFLAGS STREQUAL "")
    set_target_properties (streamripperd
        PROPERTIES LINK_FLAGS ${STREAMRIPPER_LDFLAGS})
endif ()

##-----------------------------------------------------------------------------
##  Install
##-----------------------------------------------------------------------------
#install (TARGETS streamripper DESTINATION bin)
