if(NOT TARGET libopus::libopus)
    include(FindPackageHandleStandardArgs)

    set( _libopus_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/libopus/include")

    set( _libopus_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/libopus/lib/arm64-v8a")
        
	find_path(LIBOPUS_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES opus.h REQUIRED
                PATHS ${_libopus_HEADER_SEARCH_DIRS})

    add_library(libopus::libopus SHARED IMPORTED)

	find_library(LIBOPUS_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES opus REQUIRED
                PATHS ${_libopus_LIB_SEARCH_DIRS})
    
    set_target_properties(
            libopus::libopus PROPERTIES
			IMPORTED_LOCATION ${LIBOPUS_LIB}
			INTERFACE_INCLUDE_DIRECTORIES ${LIBOPUS_HEADER_DIRS})

    mark_as_advanced(libopus::libopus)
    
endif()

