if(NOT TARGET picoquic::picoquic)
    include(FindPackageHandleStandardArgs)

    set( _picoquic_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../picoquic"
        "${CMAKE_SOURCE_DIR}/../../picoquic"
	)
        #"${PROJECT_DEPS_DIR}/picotls/include"

    set( _picoquic_LIB_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../picoquic"
        "${CMAKE_CURRENT_LIST_DIR}/../../picoquic"
	)
        #"${PROJECT_DEPS_DIR}/picotls/lib/arm64-v8a"

    set( _picoquic_HEADER_SEARCH_DIRS_hide
            "${PROJECT_DEPS_DIR}/picoquic/include"
            "${PROJECT_DEPS_DIR}/picoquic/include/picoquic"
            "${PROJECT_DEPS_DIR}/picoquic/include/picohttp"
            "${CMAKE_SOURCE_DIR}/picoquic/include/loglib"
            "${PROJECT_DEPS_DIR}/picoquic/include/picoquicfirst"
            "${PROJECT_DEPS_DIR}/picoquic/include/picoquictest"
            )

    set( _picoquic_LIB_SEARCH_DIRS_hide
        "${PROJECT_DEPS_DIR}/picoquic/lib/arm64-v8a")
        
	find_path(PICOQUIC_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES picoquic/picoquic.h REQUIRED
                PATHS ${_picoquic_HEADER_SEARCH_DIRS})

    add_library(picoquic::picoquic STATIC IMPORTED)
    add_library(picoquic::picoquichttp STATIC IMPORTED)
    add_library(picoquic::picoquiclog STATIC IMPORTED)

    find_library(PICOQUIC_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picoquic-core REQUIRED
        PATHS ${_picoquic_LIB_SEARCH_DIRS})

    find_library(PICOQUICHTTP_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picohttp-core REQUIRED
        PATHS ${_picoquic_LIB_SEARCH_DIRS})

    find_library(PICOQUICLOG_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picoquic-log REQUIRED
        PATHS ${_picoquic_LIB_SEARCH_DIRS})

    set_target_properties(
        picoquic::picoquic PROPERTIES
        IMPORTED_LOCATION ${PICOQUIC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${PICOQUIC_HEADER_DIRS})

    mark_as_advanced(picoquic::picoquic)

    set_target_properties(
        picoquic::picoquichttp PROPERTIES
        IMPORTED_LOCATION ${PICOQUICHTTP_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${PICOQUIC_HEADER_DIRS})

    mark_as_advanced(picoquic::picoquichttp)

    set_target_properties(
        picoquic::picoquiclog PROPERTIES
        IMPORTED_LOCATION ${PICOQUICLOG_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${PICOQUIC_HEADER_DIRS})

    mark_as_advanced(picoquic::picoquiclog)

endif()

