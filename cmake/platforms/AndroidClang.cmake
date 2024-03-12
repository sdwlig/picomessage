# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Configuration for compiling the source code for Android with the GCC compiler.
#
# Authored by Robert Johnson.
# Copyright 2022 - V-Nova Ltd.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

message(STATUS "Configuring for platform Android/Clang")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Platform compile flags & defs
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(PLATFORM_COMPILE_DEFS
	_SCL_SECURE_NO_WARNINGS			# Allow calling any one of the potentially unsafe methods in the Standard C++ Library
	_CRT_SECURE_NO_WARNINGS			# Disable CRT deprecation warnings
	RAPIDJSON_HAS_STDSTRING			# Globally enabled across all projects.
	_VARIADIC_MAX=10
#	_FILE_OFFSET_BITS=64
#	__USE_FILE_OFFSET64=1
	LINUX=1
	__ANDROID__=1
	VNPlatformSupportsStaticAssert
	VNInline=inline
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
	-Wno-tautological-constant-compare
	-ffunction-sections
	-funwind-tables
	-fstack-protector
	-no-canonical-prefixes
	-fno-strict-aliasing
	$<$<CONFIG:Debug>:-O0>
	$<$<CONFIG:Release>:-O2>
	$<$<CONFIG:Release>:-fvisibility=hidden>
)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL armv7-a)
	if (ANDROID_ARM_NEON)
		set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableNEON)
	else()
		set(PLATFORM_PROCESSOR_SPECIFIC_DEFS SIMD_NONE)
	endif()
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mfloat-abi=softfp)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableSSE)
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -msse4.2)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i686)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableSSSE3)
	set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mssse3)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64)
	set(PLATFORM_PROCESSOR_SPECIFIC_DEFS VNEnableNEON)
	#set(PLATFORM_PROCESSOR_SPECIFIC_FLAGS -mfloat-abi=softfp) # no soft floats with aarch64
else()
	message(FATAL_ERROR "Unsupported Android system processor defined: " ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(PLATFORM_COMPILE_DEFS ${PLATFORM_COMPILE_DEFS} ${PLATFORM_PROCESSOR_SPECIFIC_DEFS})
set(PLATFORM_COMPILE_FLAGS ${PLATFORM_COMPILE_FLAGS} ${PLATFORM_PROCESSOR_SPECIFIC_FLAGS})

# Special case C++11 support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Platform libs & linker
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

set(PLATFORM_LIBRARIES)
set(PLATFORM_LIBRARIES_MEDIA)

find_library(PLATFORM_LOG_LIBRARY
	NAMES 		log
	DOC 		"log library used for Android logging")

list(APPEND PLATFORM_LIBRARIES ${PLATFORM_LOG_LIBRARY})

if(ANDROID_VENDOR)
	find_library(PLATFORM_NATIVE_WINDOW_LIBRARY
				 NAMES nativewindow)

	list(APPEND PLATFORM_LIBRARIES_MEDIA ${PLATFORM_NATIVE_WINDOW_LIBRARY})
else()
	find_library(PLATFORM_ANDROID_LIBRARY
		NAMES		android
		DOC			"android library")

	if(${ANDROID_NATIVE_API_LEVEL} LESS_EQUAL 21)
		list(APPEND PLATFORM_LIBRARIES_MEDIA ${PLATFORM_ANDROID_LIBRARY})
	endif()

	find_library(PLATFORM_CXX_LIBRARY
		NAMES 		libc++_shared.so
		DOC 		"C++ standard library")

	# nativewindow only available from r26 onwards
	if(${ANDROID_NATIVE_API_LEVEL} GREATER 25)
		find_library(PLATFORM_NATIVE_WINDOW_LIBRARY
			NAMES		nativewindow
			DOC			"nativewindow library")

		list(APPEND PLATFORM_LIBRARIES_MEDIA ${PLATFORM_NATIVE_WINDOW_LIBRARY})
	endif()

	# mediandk only available from r21 onwards
	if(${ANDROID_NATIVE_API_LEVEL} GREATER 20)
		find_library(PLATFORM_MEDIANDK_LIBRARY
			NAMES 		mediandk
			DOC 		"mediandk library used for Android media codec")

		list(APPEND PLATFORM_LIBRARIES_MEDIA ${PLATFORM_MEDIANDK_LIBRARY})
	endif()
endif()

message("Platform Libraries >>>>  ${PLATFORM_LIBRARIES}")
message(STATUS "Platform Media Libraries >>>>  ${PLATFORM_LIBRARIES_MEDIA}")

# Force gold linker because of EPI linking issues with ldd in clang-6 VNDK package.
# The tool chain can fail with 'unresolvable R_ARM_THM_CALL relocation' otherwise,
# This is temporary and is anticipated to be removed once the vndk toolchain file
# in the VNDK package is updated accordingly (similar to how android.toolchain.cmake)
# deals with a similar issue.
if (ANDROID_VENDOR AND ANDROID_NATIVE_API_LEVEL AND ANDROID_VNDK_VERSION VERSION_LESS_EQUAL 28)
	message("Forcing gold linker for VNDK build as VNDK version is <= 28")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
endif()

# Recommendation is to not mess with these, quite sensitive is the linker on Linux.
set(PLATFORM_LINKER_FLAGS_DEBUG
	"-Wl,--build-id -Wl,--exclude-libs,ALL"
)

set(PLATFORM_LINKER_FLAGS_RELEASE
	"-Wl,--build-id -Wl,--exclude-libs,ALL,--strip-debug"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Setup global platform vars
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

if(ANDROID_VENDOR)
	set(CONAN_PROFILE "android-${ANDROID_ARCHITECTURE}-vndk-${ANDROID_VNDK_VERSION}-${CONAN_BUILD_TYPE}")
	set(PLATFORM_BINARIES_PATH
		"${PROJECT_SOURCE_DIR}/build/${PLATFORM_ARCHITECTURE}/Android/VNDK${ANDROID_VNDK_VERSION}/${CMAKE_BUILD_TYPE}"
	)
else()
	set(CONAN_PROFILE_ABI_VERSION ${ANDROID_NATIVE_API_LEVEL})

	if (DEFINED PP_CONAN_FORCE_ANDROID_VERSION)
		message(STATUS "Forcing Android conan profile ABI version: ${PP_CONAN_FORCE_ANDROID_VERSION}")
		set(CONAN_PROFILE_ABI_VERSION ${PP_CONAN_FORCE_ANDROID_VERSION})
	endif()

	set(CONAN_PROFILE "android-${ANDROID_ARCHITECTURE}-api-${CONAN_PROFILE_ABI_VERSION}-${CONAN_BUILD_TYPE}")
	set(PLATFORM_BINARIES_PATH
		"${PROJECT_SOURCE_DIR}/build/${PLATFORM_ARCHITECTURE}/Android/API${ANDROID_NATIVE_API_LEVEL}/${CMAKE_BUILD_TYPE}"
	)
endif()

message(STATUS "Using conan profile: ${CONAN_PROFILE}")

if (PP_ENABLE_ASAN OR
	PP_CLANG_ENABLE_MSAN OR
	PP_CLANG_ENABLE_UBSAN OR
	PP_CLANG_ENABLE_TSAN)

	# Common suppressions file, all sanitizer suppression to be added to it
	set(SANITIZER_COMPILER_FLAGS -fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/sanitizer-blacklist.txt -fno-omit-frame-pointer)
	if (PP_ENABLE_ASAN)
		set(SANITIZER_COMPILER_FLAGS ${SANITIZER_COMPILER_FLAGS} -fsanitize=address -fsanitize-recover=address)
	endif (PP_ENABLE_ASAN)
	if (PP_CLANG_ENABLE_MSAN)
		set(SANITIZER_COMPILER_FLAGS ${SANITIZER_COMPILER_FLAGS} -fsanitize=memory -fsanitize-recover=memory)
	endif (PP_CLANG_ENABLE_MSAN)
	if (PP_CLANG_ENABLE_TSAN)
		set(SANITIZER_COMPILER_FLAGS ${SANITIZER_COMPILER_FLAGS} -fsanitize=thread -fsanitize-recover=thread)
	endif (PP_CLANG_ENABLE_TSAN)
	if (PP_CLANG_ENABLE_UBSAN)
		set(SANITIZER_COMPILER_FLAGS ${SANITIZER_COMPILER_FLAGS}
			-fsanitize=undefined
			-fsanitize=implicit-integer-truncation
			-fsanitize=implicit-integer-arithmetic-value-change
			-fsanitize=implicit-conversion
			-fsanitize=integer
			-fsanitize=nullability)
	endif (PP_CLANG_ENABLE_UBSAN)

	# Linking shared sanitizer runtime, has to be found in library path
	# string(REPLACE ";" " " SANITIZER_LINKER_FLAGS "${SANITIZER_COMPILER_FLAGS};-shared-libsan")
	string(REPLACE ";" " " SANITIZER_LINKER_FLAGS "${SANITIZER_COMPILER_FLAGS}")

	set(PLATFORM_COMPILE_FLAGS ${PLATFORM_COMPILE_FLAGS} ${SANITIZER_COMPILER_FLAGS})
	set(PLATFORM_LINKER_FLAGS_DEBUG "${PLATFORM_LINKER_FLAGS_DEBUG} ${SANITIZER_LINKER_FLAGS}")
	set(PLATFORM_LINKER_FLAGS_RELEASE "${PLATFORM_LINKER_FLAGS_RELEASE} ${SANITIZER_LINKER_FLAGS}")

else()
	set(PLATFORM_COMPILE_FLAGS ${PLATFORM_COMPILE_FLAGS} -fomit-frame-pointer)
endif()