add_library(taiga-gui STATIC)

target_sources(taiga-gui PRIVATE
	list/list_item_delegate.cpp
	list/list_model.cpp
	list/list_view.cpp
	list/list_widget.cpp
	main/main_window.cpp
	main/navigation_item_delegate.cpp
	main/navigation_widget.cpp
	main/now_playing_widget.cpp
	media/media_dialog.cpp
	media/media_menu.cpp
	search/search_list_item_delegate.cpp
	search/search_list_model.cpp
	search/search_list_widget.cpp
	search/search_widget.cpp
	settings/settings_dialog.cpp
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
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(taiga-gui PRIVATE platforms/windows.cpp)
	target_link_libraries(taiga-gui Dwmapi.lib)
endif()
