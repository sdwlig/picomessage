if(NOT TARGET picotls)
    include(FindPackageHandleStandardArgs)

    set( _picotls_HEADER_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../picotls/include"
	)
        #"${PROJECT_DEPS_DIR}/picotls/include"

    set( _picotls_LIB_SEARCH_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../picotls/build"
	)
        #"${PROJECT_DEPS_DIR}/picotls/lib/arm64-v8a"
        
	find_path(PICOTLS_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES picotls.h REQUIRED
                PATHS ${_picotls_HEADER_SEARCH_DIRS})

    add_library(picotls::picotls STATIC IMPORTED)
    add_library(picotls::minicrypto STATIC IMPORTED)
    add_library(picotls::openssl STATIC IMPORTED)

endif()

find_library(PICOTLS_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-core REQUIRED
    PATHS ${_picotls_LIB_SEARCH_DIRS})

set_target_properties(
    picotls::picotls PROPERTIES
    IMPORTED_LOCATION ${PICOTLS_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${PICOTLS_HEADER_DIRS})

mark_as_advanced(picotls::picotls)

find_library(PICOTLS_MINICRYPTO_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-minicrypto REQUIRED
    PATHS ${_picotls_LIB_SEARCH_DIRS})

set_target_properties(
    picotls::minicrypto PROPERTIES
    IMPORTED_LOCATION ${PICOTLS_MINICRYPTO_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${PICOTLS_HEADER_DIRS})

mark_as_advanced(picotls::openssl)

find_library(PICOTLS_OPENSSL_LIB
    NO_CMAKE_SYSTEM_PATH
    NAMES picotls-openssl REQUIRED
    PATHS ${_picotls_LIB_SEARCH_DIRS})

set_target_properties(
    picotls::openssl PROPERTIES
    IMPORTED_LOCATION ${PICOTLS_OPENSSL_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${PICOTLS_HEADER_DIRS})

mark_as_advanced(picotls::openssl)


