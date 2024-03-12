if(NOT TARGET nlohmann::nlohmann)
 
    include(FindPackageHandleStandardArgs)

    add_library(nlohmann::nlohmann INTERFACE IMPORTED)

    set( _NLOHMANN_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/nlohmann"
        "${CMAKE_SOURCE_DIR}/../../external/nlohmann"
        "${CMAKE_SOURCE_DIR}/../../../external/nlohmann"
        )

    if(NLOHMANN_ROOT_DIR)
        list( INSERT _NLOHMANN_HEADER_SEARCH_DIRS 0 "${NLOHMANN_ROOT_DIR}")
    endif()

    find_path(NLOHMANN_FORMAT_HEADER_DIRS
                NAMES "json.hpp" REQUIRED
                PATHS ${_NLOHMANN_HEADER_SEARCH_DIRS})

    target_include_directories(nlohmann::nlohmann INTERFACE ${NLOHMANN_FORMAT_HEADER_DIRS})
endif()
