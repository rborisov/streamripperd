cmake_minimum_required(VERSION 2.6)

project (player)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/../cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "2")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(LibMPDClient REQUIRED)
#find_package(GTK2 REQUIRED glade gtk)
find_package(GTK2 REQUIRED gtk)
find_package(Sqlite3 REQUIRED)
find_package(Libnotify REQUIRED)
find_package(LibStreamripper REQUIRED)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
    ${GTK2_INCLUDE_DIRS} ${LIBMPDCLIENT_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIRS}
    ${LIBNOTIFY_INCLUDE_DIRS} ${LIBSTREAMRIPPER_INCLUDE_DIRS})
    #    ${GTK2_INCLUDE_DIRS} ${GTK2_GLADE_INCLUDE_DIR} ${LIBMPDCLIENT_INCLUDE_DIR})

    link_directories(${GTK2_LIBRARY_DIRS} ${LIBMPDCLIENT_LIBRARY_DIRS} ${SQLITE3_LIBRARY_DIRS}
        ${LIBNOTIFY_LIBRARY_DIRS} ${LIBSTREAMRIPPER_LIBRARY_DIRS})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "-std=gnu99 -Wall") 
set(CMAKE_C_FLAGS_DEBUG "-ggdb -pedantic")

get_target_property(MKDATA_EXE mkdata LOCATION)

set(SOURCES
    main.c
    mpd_utils.c
    gtk_utils.c
    sql.c
    db_utils.c
    utils.c
)

add_executable(player ${SOURCES})
target_link_libraries(player ${CMAKE_THREAD_LIBS_INIT} ${GTK2_LIBRARIES} 
    ${LIBMPDCLIENT_LIBRARIES} ${SQLITE3_LIBRARIES} ${LIBNOTIFY_LIBRARIES}
    )
    #    ${GTK2_GLADE_LIBRARY} ${LIBMPDCLIENT_LIBRARIES})

install(TARGETS player DESTINATION bin)
