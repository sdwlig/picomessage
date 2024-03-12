if(NOT TARGET wtlib)
 
    include(FindPackageHandleStandardArgs)

    add_library(wtlib INTERFACE IMPORTED)

    set( _wtlib_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../wt/include"
        )
        
    if(WTLIB_ROOT_DIR)
        list( INSERT _wtlib_HEADER_SEARCH_DIRS 0 "${WTLIB_ROOT_DIR}/include")
    endif()

    find_path(WTLIB_FORMAT_HEADER_DIRS 
                NAMES "WebTransport.hpp" REQUIRED
                PATHS ${_wtlib_HEADER_SEARCH_DIRS})

    target_include_directories(wtlib INTERFACE ${WTLIB_FORMAT_HEADER_DIRS})
endif()
