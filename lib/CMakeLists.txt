option(WITH_LIB_CPP "Build C++ library?" ON)
if (WITH_LIB_CPP)
	add_subdirectory(cpp)
endif (WITH_LIB_CPP)

include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/lib
			${dimq_SOURCE_DIR}/include
			${STDBOOL_H_PATH} ${STDINT_H_PATH}
			${OPENSSL_INCLUDE_DIR} ${PTHREAD_INCLUDE_DIR})
link_directories(${dimq_SOURCE_DIR}/lib)

if (WITH_BUNDLED_DEPS)
        include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/deps)
endif (WITH_BUNDLED_DEPS)

set(C_SRC
	actions.c
	callbacks.c
	connect.c
	handle_auth.c
	handle_connack.c
	handle_disconnect.c
	handle_ping.c
	handle_pubackcomp.c
	handle_publish.c
	handle_pubrec.c
	handle_pubrel.c
	handle_suback.c
	handle_unsuback.c
	helpers.c
	logging_dimq.c logging_dimq.h
	loop.c
	memory_dimq.c memory_dimq.h
	messages_dimq.c messages_dimq.h
	misc_dimq.c misc_dimq.h
	dimq.c ../include/dimq.h
	dimq_internal.h
	../include/mqtt_protocol.h
	net_dimq_ocsp.c net_dimq.c net_dimq.h
	options.c
	packet_datatypes.c
	packet_dimq.c packet_dimq.h
	property_dimq.c property_dimq.h
	read_handle.c read_handle.h
	send_connect.c
	send_disconnect.c
	send_dimq.c
	send_publish.c
	send_subscribe.c
	send_unsubscribe.c
	send_dimq.c send_dimq.h
	socks_dimq.c
	srv_dimq.c
	strings_dimq.c
	thread_dimq.c
	time_dimq.c
	tls_dimq.c
	utf8_dimq.c
	util_dimq.c util_topic.c util_dimq.h
	will_dimq.c will_dimq.h)

set (LIBRARIES ${OPENSSL_LIBRARIES} ${PTHREAD_LIBRARIES})

if (UNIX AND NOT APPLE AND NOT ANDROID)
	find_library(LIBRT rt)
	if (LIBRT)
		set (LIBRARIES ${LIBRARIES} rt)
	endif (LIBRT)
endif (UNIX AND NOT APPLE AND NOT ANDROID)

if (WIN32)
	set (LIBRARIES ${LIBRARIES} ws2_32)
endif (WIN32)

if (WITH_SRV)
	# Simple detect c-ares
	find_path(ARES_HEADER ares.h)
	if (ARES_HEADER)
		add_definitions("-DWITH_SRV")
		set (LIBRARIES ${LIBRARIES} cares)
	else (ARES_HEADER)
		message(WARNING "c-ares library not found.")
	endif (ARES_HEADER)
endif (WITH_SRV)

add_library(libdimq SHARED ${C_SRC})
set_target_properties(libdimq PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)

target_link_libraries(libdimq ${LIBRARIES})

set_target_properties(libdimq PROPERTIES
	OUTPUT_NAME dimq
	VERSION ${VERSION}
	SOVERSION 1
)

install(TARGETS libdimq RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")

if (WITH_STATIC_LIBRARIES)
	add_library(libdimq_static STATIC ${C_SRC})
	if (WITH_PIC)
		set_target_properties(libdimq_static PROPERTIES
			POSITION_INDEPENDENT_CODE 1
		)
	endif (WITH_PIC)

	target_link_libraries(libdimq_static ${LIBRARIES})

	set_target_properties(libdimq_static PROPERTIES
		OUTPUT_NAME dimq_static
		VERSION ${VERSION}
	)

	target_compile_definitions(libdimq_static PUBLIC "LIBdimq_STATIC")
	install(TARGETS libdimq_static ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif (WITH_STATIC_LIBRARIES)

install(FILES ../include/dimq.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES ../include/mqtt_protocol.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
