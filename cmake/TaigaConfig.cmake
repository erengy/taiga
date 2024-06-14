set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTORCC ON)

add_library(taiga-config INTERFACE)

target_compile_definitions(taiga-config INTERFACE
	QT_DISABLE_DEPRECATED_UP_TO=0x060700
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(taiga-config INTERFACE
		_WIN32_WINNT=0x0A00
		WIN32_LEAN_AND_MEAN
		NOMINMAX
		_UNICODE
		UNICODE
	)
endif()

if (MSVC)
	target_compile_options(taiga-config INTERFACE
		/guard:cf
		/MP
		/permissive-
		/utf-8
		/W3
		/Zc:__cplusplus
	)
else()
	target_compile_options(taiga-config INTERFACE
		-Wall
		-Wextra
	)
endif()
