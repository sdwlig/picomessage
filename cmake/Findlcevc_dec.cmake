if(NOT TARGET lcevc_dec::lcevc_dec)
    include(FindPackageHandleStandardArgs)

    set( _lcevc_dec_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/lcevc_dec/include"
        "${CMAKE_SOURCE_DIR}/../external/lcevc_dec/include/LCEVC/integration/utility"
        )

    set( _lcevc_dec_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/lcevc_dec/lib")
        
    if(LCEVC_DEC_ROOT_DIR)
        list( INSERT _lcevc_dec_HEADER_SEARCH_DIRS 0 "${LCEVC_DEC_ROOT_DIR}/include")
        list( INSERT _lcevc_dec_HEADER_SEARCH_DIRS 0 "${LCEVC_DEC_ROOT_DIR}/include/LCEVC/integration/utility")
        list( INSERT _lcevc_dec_LIB_SEARCH_DIRS 0 "${LCEVC_DEC_ROOT_DIR}/lib")
    endif()

    find_path(LCEVC_DEC_HEADER_DIRS
                NAMES "LCEVC/lcevc.h" REQUIRED
                PATHS ${_lcevc_dec_HEADER_SEARCH_DIRS})

    find_library(LCEVC_DEC_CORE_LIB
            NAMES lcevc_dec_core REQUIRED
            PATHS ${_lcevc_dec_LIB_SEARCH_DIRS})

    add_library(lcevc_dec::core SHARED IMPORTED)
    set_target_properties(
            lcevc_dec::core PROPERTIES
            IMPORTED_LOCATION ${LCEVC_DEC_CORE_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${LCEVC_DEC_HEADER_DIRS})

    add_library(lcevc_dec::integration SHARED IMPORTED)

    find_library(LCEVC_DEC_INTEGRATION_LIB
            NAMES lcevc_dec_integration REQUIRED
            PATHS ${_lcevc_dec_LIB_SEARCH_DIRS})

    set_target_properties(
            lcevc_dec::integration PROPERTIES
            IMPORTED_LOCATION ${LCEVC_DEC_INTEGRATION_LIB})
    
    find_path(LCEVC_DEC_UTIL_HEADER_DIRS
                NAMES "uLog.h" REQUIRED
                PATHS ${_lcevc_dec_HEADER_SEARCH_DIRS})

    find_library(LCEVC_DEC_UTIL_LIB
                NAMES lcevc_dec_integration_utility REQUIRED
                PATHS ${_lcevc_dec_LIB_SEARCH_DIRS})

    add_library(lcevc_dec::util SHARED IMPORTED)
    set_target_properties(
            lcevc_dec::util PROPERTIES
            IMPORTED_LOCATION ${LCEVC_DEC_UTIL_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${LCEVC_DEC_UTIL_HEADER_DIRS})

    add_library(lcevc_dec::lcevc_dec INTERFACE IMPORTED)
    target_link_libraries(lcevc_dec::lcevc_dec INTERFACE lcevc_dec::core lcevc_dec::integration lcevc_dec::util)

    mark_as_advanced(lcevc_dec::lcevc_dec)
endif()



