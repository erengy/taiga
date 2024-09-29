add_library(taiga-deps STATIC)

target_link_libraries(taiga-deps
	monolog
	semaver
	taiga-config
)

add_subdirectory(src/monolog)
add_subdirectory(src/semaver)
