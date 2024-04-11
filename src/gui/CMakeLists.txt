add_library(taiga-gui STATIC)

target_sources(taiga-gui PRIVATE
	main/main_window.cpp
	main/navigation_item_delegate.cpp
	main/navigation_widget.cpp
	settings/settings_dialog.cpp
	utils/svg_icon_engine.cpp
	utils/theme.cpp
)

target_sources(taiga-gui PRIVATE
	main/main_window.ui
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
