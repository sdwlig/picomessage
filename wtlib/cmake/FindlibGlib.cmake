if(NOT TARGET glib::glib)
    include(FindPackageHandleStandardArgs)

    set( _libglib_HEADER_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/gstreamer/arm64/include/glib-2.0"
        )

    set( _libglib_LIB_SEARCH_DIRS
        "${CMAKE_SOURCE_DIR}/../external/gstreamer/arm64/lib")
        
	if(LIBGLIB_ROOT_DIR)
		list( INSERT _libglib_HEADER_SEARCH_DIRS 0 "${LIBGLIB_ROOT_DIR}/include")
        list( INSERT _libglib_LIB_SEARCH_DIRS 0 "${LIBGLIB_ROOT_DIR}/lib")
    endif()

	find_path(LIBGLIB_HEADER_DIRS 
                NO_DEFAULT_PATH
                NO_SYSTEM_ENVIRONMENT_PATH
                NO_CMAKE_SYSTEM_PATH
                NAMES glib REQUIRED
                PATHS ${_libglib_HEADER_SEARCH_DIRS})

    add_library(glib::glib STATIC IMPORTED)
    add_library(glib::gobject STATIC IMPORTED)
    add_library(glib::gio STATIC IMPORTED)
    add_library(glib::gmodule STATIC IMPORTED)
    add_library(glib::gthread STATIC IMPORTED)
    add_library(glib::z STATIC IMPORTED)
    add_library(glib::intl STATIC IMPORTED)
    add_library(glib::iconv STATIC IMPORTED)
    add_library(glib::pcre STATIC IMPORTED)
    add_library(glib::ssl STATIC IMPORTED)
    add_library(glib::crypto STATIC IMPORTED)

    find_library(LIBGLIB_LIB NO_CMAKE_SYSTEM_PATH NAMES glib-2.0 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBGIO_LIB NO_CMAKE_SYSTEM_PATH NAMES gio-2.0 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBGOBJECT_LIB NO_CMAKE_SYSTEM_PATH NAMES gobject-2.0 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBGMODULE_LIB NO_CMAKE_SYSTEM_PATH NAMES gmodule-2.0 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBGTHREAD_LIB NO_CMAKE_SYSTEM_PATH NAMES gthread-2.0 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBZ_LIB NO_CMAKE_SYSTEM_PATH NAMES z REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBINTL_LIB NO_CMAKE_SYSTEM_PATH NAMES intl REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBICONV_LIB NO_CMAKE_SYSTEM_PATH NAMES iconv REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBPCRE_LIB NO_CMAKE_SYSTEM_PATH NAMES pcre2-8 REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBSSL_LIB NO_CMAKE_SYSTEM_PATH NAMES ssl REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    find_library(LIBCRYPTO_LIB NO_CMAKE_SYSTEM_PATH NAMES crypto REQUIRED PATHS ${_libglib_LIB_SEARCH_DIRS})
    
    set_target_properties(glib::glib PROPERTIES IMPORTED_LOCATION ${LIBGLIB_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::gobject PROPERTIES IMPORTED_LOCATION ${LIBGOBJECT_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::gio PROPERTIES IMPORTED_LOCATION ${LIBGIO_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::gmodule PROPERTIES IMPORTED_LOCATION ${LIBGMODULE_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::gthread PROPERTIES IMPORTED_LOCATION ${LIBGTHREAD_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::z PROPERTIES IMPORTED_LOCATION ${LIBZ_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::intl PROPERTIES IMPORTED_LOCATION ${LIBINTL_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::iconv PROPERTIES IMPORTED_LOCATION ${LIBICONV_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::pcre PROPERTIES IMPORTED_LOCATION ${LIBPCRE_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::ssl PROPERTIES IMPORTED_LOCATION ${LIBSSL_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})
    set_target_properties(glib::crypto PROPERTIES IMPORTED_LOCATION ${LIBCRYPTO_LIB} INTERFACE_INCLUDE_DIRECTORIES ${LIBGLIB_HEADER_DIRS})

    mark_as_advanced(glib::glib)
    mark_as_advanced(glib::gobject)
    mark_as_advanced(glib::gio)
    mark_as_advanced(glib::gmodule)
    mark_as_advanced(glib::gthread)
    mark_as_advanced(glib::z)
    mark_as_advanced(glib::intl)
    mark_as_advanced(glib::iconv)
    mark_as_advanced(glib::pcre)
    mark_as_advanced(glib::ssl)
    mark_as_advanced(glib::crypto)
    
endif()

