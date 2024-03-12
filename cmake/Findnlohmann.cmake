if(NOT TARGET nlohmann::nlohmann)
 
    include(FindPackageHandleStandardArgs)

    add_library(nlohmann::nlohmann INTERFACE IMPORTED)

    set( _NLOHMANN_HEADER_SEARCH_DIRS
        "${PROJECT_DEPS_DIR}"
        "${PROJECT_DEPS_DIR}/nlohmann"
        )

    if(NLOHMANN_ROOT_DIR)
        list( INSERT _NLOHMANN_HEADER_SEARCH_DIRS 0 "${NLOHMANN_ROOT_DIR}")
    endif()

    find_path(NLOHMANN_FORMAT_HEADER_DIRS
                NAMES "nlohmann/json.hpp" REQUIRED
                PATHS ${_NLOHMANN_HEADER_SEARCH_DIRS})

    target_include_directories(nlohmann::nlohmann INTERFACE ${NLOHMANN_FORMAT_HEADER_DIRS})
endif()
