if(NOT TARGET libav::libav)
    include(FindPackageHandleStandardArgs)

    set( _libav_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/libav/include"
        )

    set( _libav_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/libav/lib")
        
    if(LIBAV_ROOT_DIR)
        list( INSERT _libav_HEADER_SEARCH_DIRS 0 "${LIBAV_ROOT_DIR}/include")
        list( INSERT _libav_LIB_SEARCH_DIRS 0 "${LIBAV_ROOT_DIR}/lib")
    endif()

    find_path(LIBAV_FORMAT_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES libavformat/avformat.h REQUIRED
                PATHS ${_libav_HEADER_SEARCH_DIRS})

    add_library(libav::format SHARED IMPORTED)

    find_library(LIBAV_FORMAT_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES avformat REQUIRED
                PATHS ${_libav_LIB_SEARCH_DIRS})
    
    set_target_properties(
            libav::format PROPERTIES
            IMPORTED_LOCATION ${LIBAV_FORMAT_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${LIBAV_FORMAT_HEADER_DIRS})

    add_library(libav::util SHARED IMPORTED)

    find_library(LIBAV_UTIL_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES avutil REQUIRED
                PATHS ${_libav_LIB_SEARCH_DIRS})

    set_target_properties(
            libav::util PROPERTIES
            IMPORTED_LOCATION ${LIBAV_UTIL_LIB})

    add_library(libav::codec SHARED IMPORTED)

    find_library(LIBAV_CODEC_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES avcodec REQUIRED
                PATHS ${_libav_LIB_SEARCH_DIRS})
                
    set_target_properties(
            libav::codec PROPERTIES
            IMPORTED_LOCATION ${LIBAV_CODEC_LIB})

    add_library(libav::avresample SHARED IMPORTED)
                
    find_library(LIBAV_AVRESAMPLE_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES avresample REQUIRED
                PATHS ${_libav_LIB_SEARCH_DIRS})
                
    set_target_properties(
            libav::avresample PROPERTIES
            IMPORTED_LOCATION ${LIBAV_AVRESAMPLE_LIB})

    add_library(libav::libav INTERFACE IMPORTED)
    target_link_libraries(libav::libav INTERFACE libav::format libav::util libav::codec libav::avresample)
    
    mark_as_advanced(libav::libav)
    
endif()
