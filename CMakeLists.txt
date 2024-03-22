# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html
# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.
project("wttest")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
message(${CMAKE_CURRENT_LIST_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(PROJECT_DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/../AndroidExternals")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
SET(CMAKE_C_FLAGS_DEBUG "-g -O0")

add_subdirectory(wtlib)
#    find_library(wtlib
#        NO_CMAKE_SYSTEM_PATH
#        NAMES wtlib REQUIRED
#        PATHS wt/build)
#    find_package(wtlib REQUIRED MODULE)

# set(X64 OFF)
# set(PLATFORM_ARCHITECTURE "x86")

if(ANDROID)
        if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL armv7-a)
                set(PLATFORM_ARCHITECTURE "armv7a")
        elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL arm64-v8a)
                set(PLATFORM_ARCHITECTURE "aarch64")
        else()
                set(PLATFORM_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
        endif()

        if (${PLATFORM_ARCHITECTURE} STREQUAL "armv7a")
                set(ANDROID_ARCHITECTURE "armeabi-v7a")
        elseif (${PLATFORM_ARCHITECTURE} STREQUAL "aarch64")
                set(ANDROID_ARCHITECTURE "arm64-v8a")
        else()
                message(FATAL_ERROR "Failed to determine Android architecture, PLATFORM_ARCHITECTURE=${PLATFORM_ARCHITECTURE}")
        endif()
else()
#        message(FATAL_ERROR "Unsupported platform")
endif()

set(PLATFORM_COMPILER_IS_CLANG 0)
set(PLATFORM_COMPILER_IS_APPLECLANG 0)
set(PLATFORM_COMPILER_IS_GNU 0)
set(PLATFORM_COMPILER_IS_INTEL 0)
set(PLATFORM_COMPILER_IS_MSVC 0)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	set(PLATFORM_COMPILER_IS_CLANG 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
	set(PLATFORM_COMPILER_IS_GNU 1)
else()
	message(FATAL_ERROR "Unsupported compiler")
endif()

if(ANDROID)
	if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
		include(${CMAKE_CURRENT_LIST_DIR}/../cmake/platforms/AndroidClang.cmake)
	else()
		include(${CMAKE_CURRENT_LIST_DIR}/../cmake/platforms/AndroidGCC.cmake)
	endif()
        set(ANDROID_NATIVE_APP_GLUE ${ANDROID_NDK}/sources/android/native_app_glue)
else()
#	message(FATAL_ERROR "Unsupported platform")
endif()

add_compile_definitions(${PLATFORM_COMPILE_DEFS} 
						VNEnableLog=0
						VNEnableLibAV
						VNEnableMediaCodec
						VNEnableLibavformat
						VNEnableVnmuxTs
						VNEnableExternalDecoder
						VNEnableEGLExtensions
						)



add_compile_options("${PLATFORM_COMPILE_FLAGS}")

if(ANDROID)
  set(ANDROID_LIBS= native-lib.cpp AndroidHelper.cpp "${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c")
endif()

add_executable(wttest src/wttest.cpp)
# add_library(wttest SHARED src/wttest.cpp)


if(ANDROID)
  # ${ANDROID_NATIVE_APP_GLUE}
endif()

# "${PROJECT_DEPS_DIR}/libpicoquic/include/loglib"

target_include_directories(wttest PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/wt/include"
  "${PROJECT_DEPS_DIR}"
  "${PROJECT_DEPS_DIR}/oculus-openxr/include"
    "${CMAKE_SOURCE_DIR}/../picoquic/include"
    "${CMAKE_SOURCE_DIR}/../picoquic"
    "${CMAKE_SOURCE_DIR}/../picoquic/picoquic"
    "${CMAKE_SOURCE_DIR}/../picotls/include"
    "${CMAKE_SOURCE_DIR}/../picotls"
  ${CMAKE_CURRENT_LIST_DIR}
  PRIVATE ${ANDROID_NDK}/sources/android)

#  "${PROJECT_DEPS_DIR}/libpicoquic/include"
#  "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquic"
#  "${PROJECT_DEPS_DIR}/libpicoquic/include/picohttp"
#  "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquicfirst"
#  "${PROJECT_DEPS_DIR}/libpicoquic/include/picoquictest"
#    "${CMAKE_SOURCE_DIR}/../external/libpicoquic/include/picoquic"
#    "${CMAKE_SOURCE_DIR}/../external/libpicoquic/include/picohttp"
#    "${CMAKE_SOURCE_DIR}/../external/libpicoquic/include/loglib"
#    "${CMAKE_SOURCE_DIR}/../external/libpicoquic/include/picoquicfirst"
#    "${CMAKE_SOURCE_DIR}/../external/libpicoquic/include/picoquictest"


find_library(wttest # Sets the name of the path variable.
  wtlib
  log-lib
  log)

find_package(OpenSSL REQUIRED)

if(ANDROID)
find_package(openxr REQUIRED)
endif()
# find_package(rapidjson REQUIRED)
find_package(nlohmann REQUIRED)
#find_package(libdatachannel REQUIRED)
# find_package(libglib REQUIRED)
# find_package(libpcap REQUIRED)
find_package(picoquic REQUIRED)
find_package(picotls REQUIRED)

# add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/dec_api")
# add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/wtlib")

if(ANDROID)
  #android
  #GLESv3
  #EGL
endif()

target_link_libraries( # Specifies the target library.
  wttest PRIVATE
  # log
  # rapidjson::rapidjson
  nlohmann::nlohmann
  # openxr::openxr
  wtlib
  # DEC_API
  # DEC_DEMUX_LIBAV
  # DEC_DEMUX_LIBWT
  # DEC_MEDIACODEC
  # NativeAudioProxy
  # libdatachannel::libdatachannel
  #libpicoquic::libpicoquic
  #libpicoquic::libpicoquichttp
  #libpicoquic::libpicoquiclog
  #libpicotls::libpicotls
  #libpicotls::minicrypto
  #libpicotls::openssl
  picoquic::picoquic
  picoquic::picoquichttp
  picoquic::picoquiclog
  picotls::picotls
  picotls::minicrypto
  picotls::openssl
  ssl
  crypto
  pthread
  #picotls-minicrypto picotls-core picotls-openssl
    # pcap::pcap
    # glib::gobject 
    # glib::glib 
    # glib::gio 
    # glib::gmodule 
    # glib::gthread 
    # glib::z 
    # glib::intl 
    # glib::iconv 
    # glib::pcre 
    # glib::ssl 
    # glib::crypto 
)

#target_compile_options(redpillvr PUBLIC -fsanitize=hwaddress -fno-omit-frame-pointer)
#target_link_options(redpillvr PUBLIC -fsanitize=hwaddress)

#set(JNI_LIBS_DIR ${CMAKE_CURRENT_LIST_DIR}/../jniLibs/${ANDROID_ARCHITECTURE})
#message("JNI_LIBS_DIR=${JNI_LIBS_DIR}")
