include_directories(${ETVDB_SOURCE_DIR}/external/html_entities)

add_library(etvdb SHARED etvdb.c aux.c episodes.c infra.c series.c)
target_link_libraries(etvdb entities ${EINA_LIBRARIES} ${ECORE_LIBRARIES}
	${ECORE-CON_LIBRARIES} ${CURL_LIBRARIES})

install(TARGETS etvdb LIBRARY DESTINATION lib)
install(FILES etvdb.h DESTINATION include)
