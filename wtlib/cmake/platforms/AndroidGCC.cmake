# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Configuration for compiling the source code for Android with the GCC compiler.
# 
# Copyright 2022 - V-Nova Ltd.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

message(STATUS "Configuring for platform Android/GCC")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Platform compile flags & defs
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(PLATFORM_COMPILE_DEFS
	_SCL_SECURE_NO_WARNINGS			# Allow calling any one of the potentially unsafe methods in the Standard C++ Library
	_CRT_SECURE_NO_WARNINGS			# Disable CRT deprecation warnings
	RAPIDJSON_HAS_STDSTRING			# Globally enabled across all projects.
	_VARIADIC_MAX=10
	LINUX=1
	__ANDROID__=1
	VNPlatformSupportsStaticAssert
	VNInline=inline
	VNEnableGCC=1
)

set(PLATFORM_COMPILE_FLAGS
	-g3
	-Wall
	-Werror
	-pthread
	-fPIC
	-flax-vector-conversions
	-Wno-unused-function
	-Wno-unused-variable
	#-Wunused
	$<$<CONFIG:Release>:-fvisibility=hidden>
)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL armv7-a)
    if (${NEON})
        set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableNEON)
    else()
        set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableNEON)
    endif()
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mfloat-abi=softfp)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableSSE _FILE_OFFSET_BITS=64)
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -msse4.2)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableSSSE3)
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mssse3)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableNEON _FILE_OFFSET_BITS=64)

	#set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mfloat-abi=softfp) # no soft floats with aarch64
else()
	message(FATAL_ERROR "Unsupported Android system processor defined: " ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(PLATFORM_COMPILE_DEFS ${PLATFORM_COMPILE_DEFS} ${PLATFORM_PROCESSOR_SPECIFIC_DEFS})
set(PLATFORM_COMPILE_FLAGS ${PLATFORM_COMPILE_FLAGS} ${PLATFORM_PROCESSOR_SPECIFIC_FLAGS})

# Special case C++11 support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

# Strip
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Platform libs & linker
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

find_library(PLATFORM_DL_LIBRARY
	NAMES 		dl
	DOC 		"DL library used by x264")

find_library(PLATFORM_M_LIBRARY
	NAMES 		m
	DOC 		"m library")

find_library(PLATFORM_LOG_LIBRARY
	NAMES 		log
	DOC 		"log library used for Android logging")

set(PLATFORM_LIBRARIES ${PLATFORM_DL_LIBRARY} ${PLATFORM_RT_LIBRARY} ${PLATFORM_PTHREAD_LIBRARY} ${PLATFORM_M_LIBRARY} ${PLATFORM_LOG_LIBRARY})
message(STATUS "Platform Libraries: ${PLATFORM_LIBRARIES}")

# Recommendation is to not mess with these, quite sensitive is the linker on Linux.
set(PLATFORM_LINKER_FLAGS_DEBUG
	"-Wl,--no-as-needed -pthread -Wl,--build-id=none -Wl,--exclude-libs,ALL"
)

set(PLATFORM_LINKER_FLAGS_RELEASE
	"-Wl,--no-as-needed -pthread -Wl,--build-id=none -Wl,--exclude-libs,ALL"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Setup global platform vars
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(PLATFORM_BINARIES_PATH
	"${PROJECT_SOURCE_DIR}/build/${PLATFORM_ARCHITECTURE}/Android/API${ANDROID_NATIVE_API_LEVEL}/${CMAKE_BUILD_TYPE}"
)

set(PLATFORM_STATIC_SUFFIX
	".a"
)

set(PLATFORM_NAME
	${ANDROID}
)

set(PLATFORM_BUILD
	"Android"
)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}")
if (NOT CONAN_BUILD_TYPE)
	message(STATUS "CONAN_BUILD_TYPE not set, fallback to Release")
	set(CONAN_BUILD_TYPE Release)
endif ()
set(CONAN_CONFIGURATION_TYPES
	${CONAN_BUILD_TYPE}
)
set(CONAN_CMAKE_FIND_PACKAGE_GENERATOR cmake_find_package)

set(CONAN_PROFILE "android-${ANDROID_ARCHITECTURE}-api-${ANDROID_NATIVE_API_LEVEL}-${CONAN_BUILD_TYPE}")
message(STATUS "Using conan profile: ${CONAN_PROFILE}")
