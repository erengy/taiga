cmake_minimum_required(VERSION 3.21)

project(Taiga
	VERSION 2.0.0
	DESCRIPTION "The desktop tiger"
	HOMEPAGE_URL "https://taiga.moe/"
	LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(TAIGA_PORTABLE "Portable mode" ON)

include(TaigaConfig)

add_subdirectory(deps)
add_subdirectory(src)
