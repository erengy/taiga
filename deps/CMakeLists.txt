add_library(taiga-deps STATIC)

target_link_libraries(taiga-deps
	monolog
	nstd
	semaver
	taiga-config
)

add_subdirectory(src/monolog)
add_subdirectory(src/nstd)
add_subdirectory(src/semaver)
