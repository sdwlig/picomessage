if(NOT TARGET openxr::openxr)
    include(FindPackageHandleStandardArgs)

    set( _openxr_HEADER_SEARCH_DIRS
            "${CMAKE_SOURCE_DIR}/../external/oculus-openxr/include"
            "${CMAKE_SOURCE_DIR}/../../external/oculus-openxr/include"
            )

    set( _openxr_LIB_SEARCH_DIRS
            "${CMAKE_SOURCE_DIR}/../external/oculus-openxr/lib"
            "${CMAKE_SOURCE_DIR}/../../external/oculus-openxr/lib")

    if(OPENXR_ROOT_DIR)
        list( INSERT _openxr_HEADER_SEARCH_DIRS 0 "${OPENXR_ROOT_DIR}/OpenXR/Include")
        list( INSERT _openxr_HEADER_SEARCH_DIRS 0 "${OPENXR_ROOT_DIR}/3rdParty/khronos/openxr/OpenXR-SDK/include")
        list( INSERT _openxr_HEADER_SEARCH_DIRS 0 "${OPENXR_ROOT_DIR}/include")
        list( INSERT _openxr_LIB_SEARCH_DIRS 0 "${OPENXR_ROOT_DIR}/OpenXR/Libs/Android/${ANDROID_ARCHITECTURE}/${CMAKE_BUILD_TYPE}")
        list( INSERT _openxr_LIB_SEARCH_DIRS 0 "${OPENXR_ROOT_DIR}/lib")
    endif()

    find_path(OPENXR_LOADER_HEADER_DIRS
            NAMES openxr/openxr_oculus.h REQUIRED
            PATHS ${_openxr_HEADER_SEARCH_DIRS})
    
    find_path(OPENXR_SDK_HEADER_DIRS
            NAMES openxr/openxr.h REQUIRED
            PATHS ${_openxr_HEADER_SEARCH_DIRS})

    find_library(OPENXR_LOADER_LIB
            NO_CMAKE_SYSTEM_PATH
            NAMES openxr_loader REQUIRED
            PATHS ${_openxr_LIB_SEARCH_DIRS})

    add_library(openxr::openxr SHARED IMPORTED)
    
    set_property(TARGET openxr::openxr PROPERTY IMPORTED_LOCATION ${OPENXR_LOADER_LIB})
    set_property(TARGET openxr::openxr PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENXR_LOADER_HEADER_DIRS} ${OPENXR_SDK_HEADER_DIRS})

endif()
