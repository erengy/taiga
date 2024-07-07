add_library(taiga-deps STATIC)

target_link_libraries(taiga-deps
	semaver
	taiga-config
)

add_subdirectory(src/semaver)
