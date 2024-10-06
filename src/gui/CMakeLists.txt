add_library(taiga-gui STATIC)

target_sources(taiga-gui PRIVATE
	common/anime_list_item_delegate.cpp
	common/anime_list_item_delegate_cards.cpp
	common/anime_list_view.cpp
	common/anime_list_view_cards.cpp
	list/list_widget.cpp
	main/main_window.cpp
	main/navigation_item_delegate.cpp
	main/navigation_widget.cpp
	main/now_playing_widget.cpp
	media/media_dialog.cpp
	media/media_menu.cpp
	models/anime_list_model.cpp
	models/anime_list_proxy_model.cpp
	search/search_widget.cpp
	settings/settings_dialog.cpp
	utils/format.cpp
	utils/image_provider.cpp
	utils/svg_icon_engine.cpp
	utils/theme.cpp
	utils/tray_icon.cpp
)

target_sources(taiga-gui PRIVATE
	list/list_widget.ui
	main/main_window.ui
	media/media_dialog.ui
	search/search_widget.ui
	settings/settings_dialog.ui
)

target_link_libraries(taiga-gui
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	taiga-config
	taiga-deps
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(taiga-gui PRIVATE platforms/windows.cpp)
	target_link_libraries(taiga-gui Dwmapi.lib)
endif()
