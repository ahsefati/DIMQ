if (CJSON_FOUND AND WITH_TLS)
	add_definitions("-DWITH_CJSON")

	set( CLIENT_INC ${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
		${STDBOOL_H_PATH} ${STDINT_H_PATH} ${PTHREAD_INCLUDE_DIR}
		${OPENSSL_INCLUDE_DIR} ${dimq_SOURCE_DIR}/deps
		${dimq_SOURCE_DIR}/src
		${CJSON_INCLUDE_DIRS} )

	set( CLIENT_DIR ${dimq_BINARY_DIR}/lib ${CJSON_DIR})

	include_directories(${CLIENT_INC})
	link_directories(${CLIENT_DIR} ${dimq_SOURCE_DIR})

	add_library(dimq_dynamic_security MODULE
		acl.c
		auth.c
		clients.c
		clientlist.c
		dynamic_security.h
		groups.c
		grouplist.c
		json_help.c
		json_help.h
		plugin.c
		roles.c
		rolelist.c
		sub_matches_sub.c)

	set_target_properties(dimq_dynamic_security PROPERTIES
		POSITION_INDEPENDENT_CODE 1
	)
	set_target_properties(dimq_dynamic_security PROPERTIES PREFIX "")

	target_link_libraries(dimq_dynamic_security ${CJSON_LIBRARIES} ${OPENSSL_LIBRARIES})
	if(WIN32)
		target_link_libraries(dimq_dynamic_security dimq)
	endif(WIN32)

	install(TARGETS dimq_dynamic_security RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
