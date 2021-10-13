if (WITH_TLS AND CJSON_FOUND)
	add_definitions("-DWITH_CJSON")

	include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
			${dimq_SOURCE_DIR}/lib ${dimq_SOURCE_DIR}/src
			${OPENSSL_INCLUDE_DIR} ${STDBOOL_H_PATH} ${STDINT_H_PATH}
			${CJSON_INCLUDE_DIRS} ${dimq_SOURCE_DIR}/apps/dimq_passwd)

	link_directories(${CJSON_DIR})

	add_executable(dimq_ctrl
		dimq_ctrl.c dimq_ctrl.h
		client.c
		dynsec.c
		dynsec_client.c
		dynsec_group.c
		dynsec_role.c
		../dimq_passwd/get_password.c ../dimq_passwd/get_password.h
		../../lib/memory_dimq.c ../../lib/memory_dimq.h
		../../src/memory_public.c
		options.c
		../../src/password_dimq.c ../../src/password_dimq.h
		)

	if (WITH_STATIC_LIBRARIES)
		target_link_libraries(dimq_ctrl libdimq_static)
	else()
		target_link_libraries(dimq_ctrl libdimq)
	endif()

	if (UNIX)
		if (APPLE)
			target_link_libraries(dimq_ctrl dl)
		elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
			#
		elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
			#
		elseif(QNX)
			#
		else(APPLE)
			target_link_libraries(dimq_ctrl dl)
		endif (APPLE)
	endif (UNIX)

	target_link_libraries(dimq_ctrl ${OPENSSL_LIBRARIES} ${CJSON_LIBRARIES})

	install(TARGETS dimq_ctrl RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif (WITH_TLS AND CJSON_FOUND)
