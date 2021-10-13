include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
		${dimq_SOURCE_DIR}/lib ${dimq_SOURCE_DIR}/src
		${OPENSSL_INCLUDE_DIR} ${STDBOOL_H_PATH} ${STDINT_H_PATH})

if (WITH_TLS)
	add_executable(dimq_passwd
		dimq_passwd.c
		get_password.c get_password.h
		../../lib/memory_dimq.c ../../lib/memory_dimq.h
		../../src/memory_public.c
		../../lib/misc_dimq.c
		../../src/password_dimq.c ../../src/password_dimq.h
		)


	target_link_libraries(dimq_passwd ${OPENSSL_LIBRARIES})
	install(TARGETS dimq_passwd RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif (WITH_TLS)
