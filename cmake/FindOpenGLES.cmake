# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# CMake package find script for GLES. This creates a single target:
# 
#  1. OpenGL::GLES: This is an interface target for the GLES libraries.
# 
# Copyright 2022 - V-Nova Ltd.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

include (FindPackageHandleStandardArgs)

if(ANDROID)

	# libGLESv3 only available from r18 onwards
	message("Using SDK ${ANDROID_NATIVE_API_LEVEL}")
	if (${ANDROID_NATIVE_API_LEVEL} GREATER 17)
		find_library(OpenGLES_LIBRARIES 
			NAMES	GLESv3
			DOC		"The GLESv3 library")
	else()
		find_library(OpenGLES_LIBRARIES 
			NAMES	GLESv2
			DOC		"The GLESv2 library")
	endif()

	find_library(egl_LIBRARIES 
		NAMES EGL
		DOC 	"The EGL library")

	if(egl_LIBRARIES)
		set(OpenGL_EGL_FOUND TRUE)
	else()
		set(OpenGL_EGL_FOUND FALSE)
	endif()

	find_package_handle_standard_args(OpenGLES REQUIRE_VARS OpenGLES_LIBRARIES)
	mark_as_advanced(OpenGLES_LIBRARIES)
else()
	set(OpenGLES_FOUND 1)
	set(OpenGL_EGL_FOUND 1)
endif()

if(OpenGLES_FOUND AND NOT TARGET OpenGL::GLES)
	message(STATUS "  OpenGLES")

	add_library(OpenGL::GLES INTERFACE IMPORTED)
	set_target_properties(OpenGL::GLES PROPERTIES 
							INTERFACE_LINK_LIBRARIES "${OpenGLES_LIBRARIES}"
							INTERFACE_COMPILE_DEFINITIONS "VNEnableGLES=1;VNGLESVersion=32")
endif()

if(OpenGL_EGL_FOUND)
	message(STATUS "  egl")

	add_library(OpenGL::EGL INTERFACE IMPORTED)
	set_target_properties(OpenGL::EGL PROPERTIES 
							INTERFACE_LINK_LIBRARIES "${egl_LIBRARIES}")
endif()
