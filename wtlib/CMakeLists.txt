# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Configuration for compiling the libwt library.
#
#  1. ${LIBWT_TARGET}: 	This builds a static library.
#  2. dec_libwt: 
#
# Copyright 2024 - Stephen D. Williams sdw@lig.net
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cmake_minimum_required(VERSION 3.21)
project(wt)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(PROJECT_DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/../AndroidExternals")

SET(CMAKE_CXX_FLAGS_DEBUG "-O0")
SET(CMAKE_C_FLAGS_DEBUG "-O0")

# find_package(libpcap REQUIRED)
# find_package(nlohmann REQUIRED)
find_package(libpicoquic REQUIRED)
find_package(libpicotls REQUIRED)

# ${PP_DEC_DEMUX_LIBWT_TARGET}

# set(ANDROID_NATIVE_APP_GLUE ${ANDROID_NDK}/sources/android/native_app_glue)

add_library(wtlib STATIC src/WebTransport.cpp)

target_include_directories(wtlib
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../wt/include"
    "${CMAKE_SOURCE_DIR}/../picoquic/include"
    "${CMAKE_SOURCE_DIR}/../picoquic"
    "${CMAKE_SOURCE_DIR}/../picoquic/picoquic"
    "${CMAKE_SOURCE_DIR}/../picotls/include"
    "${CMAKE_SOURCE_DIR}/../picotls"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../external/oculus-openxr/include/"
    PUBLIC	"${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE	"${CMAKE_CURRENT_LIST_DIR}/src"
    ${ANDROID_NATIVE_APP_GLUE}
    PRIVATE ${ANDROID_NDK}/sources/android)

# set(LOCAL_STATIC_LIBRARIES cpufeatures android_native_app_glue ndk_helper)
# set(ANDROID_NATIVE_APP_GLUE ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(wtlib
  libpicoquic::libpicoquic
  libpicotls::libpicotls
  # pcap::pcap
)

if(ANDROID)
  target_link_libraries(DEC_DEMUX_LIBWT
    PRIVATE 	-landroid -llog)

  if(OpenGL_EGL_FOUND)
    target_compile_definitions(DEC_DEMUX_LIBWT
      PRIVATE		VNEnableEGL=1)
  endif()

endif()
