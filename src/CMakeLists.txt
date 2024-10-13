include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(taiga)

target_sources(taiga PRIVATE
	base/chrono.cpp
	compat/anime.cpp
	compat/settings.cpp
	media/anime_db_2.cpp
	media/anime_season.cpp
	resources/icons.qrc
	resources/styles.qrc
	taiga/application.cpp
	taiga/orange.cpp
	taiga/path.cpp
	taiga/version.cpp
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
	taiga-deps
	taiga-gui
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(taiga PROPERTIES OUTPUT_NAME Taiga)
	set_target_properties(taiga PROPERTIES WIN32_EXECUTABLE ON)
endif()

add_subdirectory(gui)
