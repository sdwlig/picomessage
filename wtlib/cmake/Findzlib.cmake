if(NOT TARGET zlib::zlib)
 
    include(FindPackageHandleStandardArgs)

    add_library(zlib::zlib INTERFACE IMPORTED)

    set( _zlib_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/zlib/include"
        )
        
    if(ZLIB_ROOT_DIR)
        list( INSERT _zlib_HEADER_SEARCH_DIRS 0 "${ZLIB_ROOT_DIR}/include")
    endif()

    find_path(ZLIB_FORMAT_HEADER_DIRS 
                NAMES "zlib/zlib.h" REQUIRED
                PATHS ${_zlib_HEADER_SEARCH_DIRS})

    target_include_directories(zlib::zlib INTERFACE ${ZLIB_FORMAT_HEADER_DIRS})
endif()
