cmake_minimum_required(VERSION 3.21)

project(Taiga
	VERSION 2.0.0
	DESCRIPTION "The desktop tiger"
	HOMEPAGE_URL "https://taiga.moe/"
	LANGUAGES CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

list(APPEND CMAKE_PREFIX_PATH "%QTDIR%/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)
qt_standard_project_setup()

include(TaigaConfig)

add_subdirectory(deps)
add_subdirectory(src)
