cmake_minimum_required(VERSION 3.16)

project(virtual-input
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "Release")
	message(STATUS "Set build type to ${CMAKE_BUILD_TYPE}")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output LANGUAGES CXX)
if(result)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
	message(WARNING "IPO is not supported: ${output}")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(
		/W4 /utf-8 /Zc:inline,preprocessor
	)
	target_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else() # GCC, Clang, ...
	add_compile_options(
		-Wall -Wextra -Wpedantic
	)
endif()

aux_source_directory(. vinput_common_src)
list(FILTER vinput_common_src EXCLUDE REGEX "desktop_[a-z0-9]+\\.cc")

add_executable(vinput ${vinput_common_src})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(NOT DEFINED DISPLAY_SERVER)
		set(DISPLAY_SERVER "x11")
		message("`DISPLAY_SERVER` not defined, use x11 by default")
	else()
		string(TOLOWER ${DISPLAY_SERVER} DISPLAY_SERVER)
	endif()
	if(DISPLAY_SERVER STREQUAL "x11")
		target_compile_definitions(vinput PRIVATE "VINPUT_DESKTOP_X11=1")
		target_sources(vinput PRIVATE "desktop_x11.cc")
		target_link_libraries(vinput PRIVATE X11 Xtst)
	else()
		message(FATAL_ERROR "Unsupported display server protocol: ${DISPLAY_SERVER}")
	endif()
else()
	message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()

if(UNIX)
	set(vinput_install_dest "bin")
else()
	set(vinput_install_dest ".")
endif()
install(TARGETS vinput DESTINATION ${vinput_install_dest})

set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_NAME "vinput")
set(CPACK_PACKAGE_CHECKSUM MD5)
include(CPack)
