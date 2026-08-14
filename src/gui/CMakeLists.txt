add_library(taiga-gui STATIC)

qt_add_ui(taiga-gui SOURCES
	main/main_window.ui
	media/media_dialog.ui
	settings/settings_dialog.ui
)

target_sources(taiga-gui PRIVATE
	common/anime_list_item_delegate.cpp
	common/anime_list_item_delegate.hpp
	common/anime_list_item_delegate_cards.cpp
	common/anime_list_item_delegate_cards.hpp
	common/anime_list_view.cpp
	common/anime_list_view.hpp
	common/anime_list_view_base.cpp
	common/anime_list_view_base.hpp
	common/anime_list_view_cards.cpp
	common/anime_list_view_cards.hpp
	common/clickable_label.cpp
	common/clickable_label.hpp
	common/combobox.cpp
	common/combobox.hpp
	common/page_widget.cpp
	common/page_widget.hpp
	common/spinner_widget.cpp
	common/spinner_widget.hpp

	history/history_widget.cpp
	history/history_widget.hpp

	library/library_menu.cpp
	library/library_menu.hpp
	library/library_widget.cpp
	library/library_widget.hpp

	list/list_widget.cpp
	list/list_widget.hpp

	main/about_dialog.cpp
	main/about_dialog.hpp
	main/main_window.cpp
	main/main_window.hpp
	main/navigation_item_delegate.cpp
	main/navigation_item_delegate.hpp
	main/navigation_widget.cpp
	main/navigation_widget.hpp
	main/now_playing_widget.cpp
	main/now_playing_widget.hpp
	main/status_bar.cpp
	main/status_bar.hpp
	main/status_bar_controller.cpp
	main/status_bar_controller.hpp

	media/media_dialog.cpp
	media/media_dialog.hpp
	media/media_menu.cpp
	media/media_menu.hpp

	models/anime_list_model.cpp
	models/anime_list_model.hpp
	models/anime_list_proxy_model.cpp
	models/anime_list_proxy_model.hpp
	models/history_model.cpp
	models/history_model.hpp
	models/history_proxy_model.cpp
	models/history_proxy_model.hpp
	models/library_model.cpp
	models/library_model.hpp

	search/search_widget.cpp
	search/search_widget.hpp

	settings/settings_dialog.cpp
	settings/settings_dialog.hpp

	utils/format.cpp
	utils/format.hpp
	utils/image_provider.cpp
	utils/image_provider.hpp
	utils/painter_state_saver.hpp
	utils/painters.cpp
	utils/painters.hpp
	utils/rating.cpp
	utils/rating.hpp
	utils/svg_icon_engine.cpp
	utils/svg_icon_engine.hpp
	utils/theme.cpp
	utils/theme.hpp
	utils/tray_icon.cpp
	utils/tray_icon.hpp
	utils/widgets.cpp
	utils/widgets.hpp
)

target_link_libraries(taiga-gui
	Qt6::Concurrent
	Qt6::Core
	Qt6::Gui
	Qt6::Network
	Qt6::Sql
	Qt6::Widgets
	taiga-config
	taiga-deps
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_sources(taiga-gui PRIVATE platforms/windows.cpp)
	target_link_libraries(taiga-gui Dwmapi.lib)
endif()
