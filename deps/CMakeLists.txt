add_library(taiga-deps STATIC)

target_link_libraries(taiga-deps
	anisthesia
	anitomy
	nstd
	semaver
	taiga-config
	utf8proc
)

add_subdirectory(anisthesia)
add_subdirectory(anitomy)
add_subdirectory(nstd)
add_subdirectory(semaver)
add_subdirectory(utf8proc)
