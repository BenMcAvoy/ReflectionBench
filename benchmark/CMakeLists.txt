cmake_minimum_required(VERSION 3.21)

project(meta_bench VERSION 0.1.0 LANGUAGES CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(META_BENCH_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(META_BENCH_LTO "Enable link-time optimization for Release" OFF)

# Warnings and UTF-8 handling
if(MSVC)
	add_compile_options(/W4 /permissive- /utf-8)
	if(META_BENCH_WARNINGS_AS_ERRORS)
		add_compile_options(/WX)
	endif()
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
	if(META_BENCH_WARNINGS_AS_ERRORS)
		add_compile_options(-Werror)
	endif()
endif()

# This is from VCPKG
find_package(rttr CONFIG REQUIRED)

# Download WIP branch of EnTT for new optimisations
include(FetchContent)
FetchContent_Declare(
	entt
	GIT_REPOSITORY https://github.com/skypjack/entt.git
	GIT_TAG wip
	GIT_SHALLOW TRUE # dont download loads of extra stuff
)
FetchContent_MakeAvailable(entt)

add_executable(meta_bench
	src/main.cpp
)

target_link_libraries(meta_bench PRIVATE
	RTTR::Core
	EnTT::EnTT
)

target_compile_definitions(meta_bench PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

# Enable LTO for Release if requested
if(META_BENCH_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
	if(ipo_supported)
		set_property(TARGET meta_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
	else()
		message(WARNING "IPO/LTO not supported: ${ipo_output}")
	endif()
endif()

# Copy dependent runtime DLLs (e.g., rttr_core.dll) next to the executable on Windows
if(WIN32)
	add_custom_command(TARGET meta_bench POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_RUNTIME_DLLS:meta_bench>
			$<TARGET_FILE_DIR:meta_bench>
		COMMAND_EXPAND_LISTS)
endif()

# Helpful default for MSVC multi-config generators (Visual Studio or Ninja Multi-Config)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Install rule (local staging if desired, optional)
install(TARGETS meta_bench RUNTIME DESTINATION .)

# Enable compile_commands.json where supported (useful for IDEs, LSP specifically)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

