if(NOT TARGET datachannel::datachannel)
    include(FindPackageHandleStandardArgs)

    set( _libdatachannel_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/datachannel/include"
        )

    set( _libdatachannel_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/datachannel/lib")
        
	if(LIBDATACHANNEL_ROOT_DIR)
		list( INSERT _libdatachannel_HEADER_SEARCH_DIRS 0 "${LIBDATACHANNEL_ROOT_DIR}/include")
        list( INSERT _libdatachannel_LIB_SEARCH_DIRS 0 "${LIBDATACHANNEL_ROOT_DIR}/lib")
    endif()

	find_path(LIBDATACHANNEL_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES rtc REQUIRED
                PATHS ${_libdatachannel_HEADER_SEARCH_DIRS})

    add_library(libdatachannel::libdatachannel STATIC IMPORTED)

	find_library(LIBDATACHANNEL_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES datachannel REQUIRED
                PATHS ${_libdatachannel_LIB_SEARCH_DIRS})
    
    set_target_properties(
            libdatachannel::libdatachannel PROPERTIES
			IMPORTED_LOCATION ${LIBDATACHANNEL_LIB}
			INTERFACE_INCLUDE_DIRECTORIES ${LIBDATACHANNEL_HEADER_DIRS})

    mark_as_advanced(libdatachannel::libdatachannel)
    
endif()

