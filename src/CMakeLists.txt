include(QtSetup)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(taiga)

target_sources(taiga PRIVATE
	base/chrono.cpp
	base/chrono.hpp
	base/file.cpp
	base/file.hpp
	base/log.hpp
	base/preprocessor.h
	base/rss.hpp
	base/settings.cpp
	base/settings.hpp
	base/string.cpp
	base/string.hpp
	base/xml.cpp
	base/xml.hpp

	compat/anime.cpp
	compat/anime.hpp
	compat/common.hpp
	compat/history.cpp
	compat/history.hpp
	compat/list.cpp
	compat/list.hpp
	compat/settings.cpp
	compat/settings.hpp

	media/anime_db.cpp
	media/anime_db.hpp
	media/anime_history.cpp
	media/anime_history.hpp
	media/anime_list_export.cpp
	media/anime_list_export.hpp
	media/anime_list_utils.cpp
	media/anime_list_utils.hpp
	media/anime_list.hpp
	media/anime_season.cpp
	media/anime_season.hpp
	media/anime_utils.cpp
	media/anime_utils.hpp
	media/anime.hpp

	sync/anilist.cpp
	sync/anilist.hpp
	sync/anilist_error.cpp
	sync/anilist_error.hpp
	sync/anilist_parsers.cpp
	sync/anilist_parsers.hpp
	sync/anilist_ratings.cpp
	sync/anilist_ratings.hpp
	sync/anilist_utils.cpp
	sync/anilist_utils.hpp
	sync/kitsu_parsers.cpp
	sync/kitsu_parsers.hpp
	sync/kitsu_ratings.cpp
	sync/kitsu_ratings.hpp
	sync/kitsu_utils.cpp
	sync/kitsu_utils.hpp
	sync/myanimelist.hpp
	sync/myanimelist_parsers.cpp
	sync/myanimelist_parsers.hpp
	sync/myanimelist_utils.cpp
	sync/myanimelist_utils.hpp
	sync/service.cpp
	sync/service.hpp

	taiga/accounts.cpp
	taiga/accounts.hpp
	taiga/application.cpp
	taiga/application.hpp
	taiga/config.h
	taiga/network.cpp
	taiga/network.hpp
	taiga/orange.cpp
	taiga/orange.hpp
	taiga/path.cpp
	taiga/path.hpp
	taiga/session.cpp
	taiga/session.hpp
	taiga/settings.cpp
	taiga/settings.hpp
	taiga/version.cpp
	taiga/version.hpp

	track/episode.cpp
	track/episode.hpp
	track/media.cpp
	track/media.hpp
	track/play.cpp
	track/play.hpp
	track/recognition.cpp
	track/recognition.hpp
	track/recognition_cache.cpp
	track/recognition_cache.hpp
	track/recognition_normalize.cpp
	track/recognition_normalize.hpp
	track/scanner.cpp
	track/scanner.hpp

	main.cpp
)

target_link_libraries(taiga PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Network
	Qt6::Sql
	Qt6::Widgets
	taiga-config
	taiga-deps
	taiga-gui
	taiga-resources
)

if (TAIGA_PORTABLE)
	target_compile_definitions(taiga PRIVATE TAIGA_PORTABLE)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(taiga PROPERTIES OUTPUT_NAME Taiga)
	set_target_properties(taiga PROPERTIES WIN32_EXECUTABLE ON)
endif()

add_subdirectory(gui)
add_subdirectory(resources)

qt_add_translations(taiga
	SOURCE_TARGETS taiga-gui
	TS_FILE_BASE taiga
	TS_FILE_DIR resources/translations
	LUPDATE_OPTIONS -no-obsolete -locations none
)
