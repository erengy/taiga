include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(taiga)

target_sources(taiga PRIVATE
	taiga/application.cpp
	main.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(taiga PRIVATE
		resources/taiga.manifest
		resources/taiga.rc
	)
endif()

target_link_libraries(taiga PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	taiga-config
	taiga-gui
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(taiga PROPERTIES OUTPUT_NAME Taiga)
	set_target_properties(taiga PROPERTIES WIN32_EXECUTABLE ON)
endif()

add_subdirectory(gui)
