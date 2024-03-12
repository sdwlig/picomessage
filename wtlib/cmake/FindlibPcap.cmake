if(NOT TARGET pcap::pcap)
    include(FindPackageHandleStandardArgs)

    set( _libpcap_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/pcap/include"
        )

    set( _libpcap_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/pcap")
        
	if(LIBPCAP_ROOT_DIR)
		list( INSERT _libpcap_HEADER_SEARCH_DIRS 0 "${LIBPCAP_ROOT_DIR}/include")
        list( INSERT _libpcap_LIB_SEARCH_DIRS 0 "${LIBPCAP_ROOT_DIR}/lib")
    endif()

	find_path(LIBPCAP_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES pcap.h REQUIRED
                PATHS ${_libpcap_HEADER_SEARCH_DIRS})

    add_library(pcap::pcap STATIC IMPORTED)

	find_library(LIBPCAP_LIB
                NO_CMAKE_SYSTEM_PATH
                NAMES pcap REQUIRED
                PATHS ${_libpcap_LIB_SEARCH_DIRS})
    
    set_target_properties(
            pcap::pcap PROPERTIES
			IMPORTED_LOCATION ${LIBPCAP_LIB}
			INTERFACE_INCLUDE_DIRECTORIES ${LIBPCAP_HEADER_DIRS})

    mark_as_advanced(pcap::pcap)
    
endif()

