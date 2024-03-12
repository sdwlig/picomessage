if(NOT TARGET rapidjson::rapidjson)
 
    include(FindPackageHandleStandardArgs)

    add_library(rapidjson::rapidjson INTERFACE IMPORTED)

    set( _rapidjson_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/rapidjson/include"
        )
        
    if(RAPIDJSON_ROOT_DIR)
        list( INSERT _rapidjson_HEADER_SEARCH_DIRS 0 "${RAPIDJSON_ROOT_DIR}/include")
    endif()

    find_path(RAPIDJSON_FORMAT_HEADER_DIRS 
                NAMES "rapidjson/document.h" REQUIRED
                PATHS ${_rapidjson_HEADER_SEARCH_DIRS})

    target_include_directories(rapidjson::rapidjson INTERFACE ${RAPIDJSON_FORMAT_HEADER_DIRS})
endif()