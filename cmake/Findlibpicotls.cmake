if(NOT TARGET libpicotls::libpicotls)
    include(FindPackageHandleStandardArgs)

    set( _libpicotls_HEADER_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../picotls/include"
	)
        #"${PROJECT_DEPS_DIR}/libpicotls/include"

    set( _libpicotls_LIB_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../picotls/build"
	)
        #"${PROJECT_DEPS_DIR}/libpicotls/lib/arm64-v8a"
        
	find_path(LIBPICOTLS_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES picotls.h REQUIRED
                PATHS ${_libpicotls_HEADER_SEARCH_DIRS})

    add_library(libpicotls::libpicotls STATIC IMPORTED)
    add_library(libpicotls::minicrypto STATIC IMPORTED)
    add_library(libpicotls::openssl STATIC IMPORTED)

endif()

find_library(LIBPICOTLS_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-core REQUIRED
    PATHS ${_libpicotls_LIB_SEARCH_DIRS})

set_target_properties(
    libpicotls::libpicotls PROPERTIES
    IMPORTED_LOCATION ${LIBPICOTLS_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOTLS_HEADER_DIRS})

mark_as_advanced(libpicotls::libpicotls)

find_library(LIBPICOTLS_MINICRYPTO_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-minicrypto REQUIRED
    PATHS ${_libpicotls_LIB_SEARCH_DIRS})

set_target_properties(
    libpicotls::minicrypto PROPERTIES
    IMPORTED_LOCATION ${LIBPICOTLS_MINICRYPTO_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOTLS_HEADER_DIRS})

mark_as_advanced(libpicotls::openssl)

find_library(LIBPICOTLS_OPENSSL_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-openssl REQUIRED
    PATHS ${_libpicotls_LIB_SEARCH_DIRS})

set_target_properties(
    libpicotls::openssl PROPERTIES
    IMPORTED_LOCATION ${LIBPICOTLS_OPENSSL_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${LIBPICOTLS_HEADER_DIRS})

mark_as_advanced(libpicotls::openssl)


