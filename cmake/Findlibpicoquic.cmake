if(NOT TARGET libpicoquic::libpicoquic)
    include(FindPackageHandleStandardArgs)

    set( _libpicoquic_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../picoquic"
        "${CMAKE_SOURCE_DIR}/../../picoquic"
	)
        #"${PROJECT_DEPS_DIR}/libpicotls/include"

    set( _libpicoquic_LIB_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../picoquic"
        "${CMAKE_CURRENT_LIST_DIR}/../../picoquic"
	)
        #"${PROJECT_DEPS_DIR}/libpicotls/lib/arm64-v8a"

    set( _libpicoquic_HEADER_SEARCH_DIRS_hide
            "${PROJECT_DEPS_DIR}/libpicoquic/include"
            "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquic"
            "${PROJECT_DEPS_DIR}/libpicoquic/include/picohttp"
            "${CMAKE_SOURCE_DIR}/libpicoquic/include/loglib"
            "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquicfirst"
            "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquictest"
            )

    set( _libpicoquic_LIB_SEARCH_DIRS_hide
        "${PROJECT_DEPS_DIR}/libpicoquic/lib/arm64-v8a")
        
	find_path(LIBPICOQUIC_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES picoquic/picoquic.h REQUIRED
                PATHS ${_libpicoquic_HEADER_SEARCH_DIRS})

    add_library(libpicoquic::libpicoquic STATIC IMPORTED)
    add_library(libpicoquic::libpicoquichttp STATIC IMPORTED)
    add_library(libpicoquic::libpicoquiclog STATIC IMPORTED)

    find_library(LIBPICOQUIC_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picoquic-core REQUIRED
        PATHS ${_libpicoquic_LIB_SEARCH_DIRS})

    find_library(LIBPICOQUICHTTP_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picohttp-core REQUIRED
        PATHS ${_libpicoquic_LIB_SEARCH_DIRS})

    find_library(LIBPICOQUICLOG_LIB
        NO_CMAKE_SYSTEM_PATH
        NAMES picoquic-log REQUIRED
        PATHS ${_libpicoquic_LIB_SEARCH_DIRS})

    set_target_properties(
        libpicoquic::libpicoquic PROPERTIES
        IMPORTED_LOCATION ${LIBPICOQUIC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOQUIC_HEADER_DIRS})

    mark_as_advanced(libpicoquic::libpicoquic)

    set_target_properties(
        libpicoquic::libpicoquichttp PROPERTIES
        IMPORTED_LOCATION ${LIBPICOQUICHTTP_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOQUIC_HEADER_DIRS})

    mark_as_advanced(libpicoquic::libpicoquichttp)

    set_target_properties(
        libpicoquic::libpicoquiclog PROPERTIES
        IMPORTED_LOCATION ${LIBPICOQUICLOG_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOQUIC_HEADER_DIRS})

    mark_as_advanced(libpicoquic::libpicoquiclog)

endif()

