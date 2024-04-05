add_library(taiga-gui STATIC)

target_sources(taiga-gui PRIVATE
	main/main_window.cpp
)

target_sources(taiga-gui PRIVATE
	main/main_window.ui
)

target_link_libraries(taiga-gui
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	taiga-config
)
