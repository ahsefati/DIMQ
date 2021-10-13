set(shared_src client_shared.c client_shared.h client_props.c)

if (WITH_SRV)
	add_definitions("-DWITH_SRV")
endif (WITH_SRV)

set( CLIENT_INC ${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
            ${STDBOOL_H_PATH} ${STDINT_H_PATH} ${PTHREAD_INCLUDE_DIR}
            ${OPENSSL_INCLUDE_DIR})

set( CLIENT_DIR ${dimq_BINARY_DIR}/lib)

if (CJSON_FOUND)
	add_definitions("-DWITH_CJSON")
	set( CLIENT_DIR "${CLIENT_DIR};${CJSON_DIR}" )
	set( CLIENT_INC "${CLIENT_INC};${CJSON_INCLUDE_DIRS}" )
endif()

include_directories(${CLIENT_INC})
link_directories(${CLIENT_DIR})

add_executable(dimq_pub pub_client.c pub_shared.c ${shared_src})
add_executable(dimq_sub sub_client.c sub_client_output.c ${shared_src})
add_executable(dimq_rr rr_client.c pub_shared.c sub_client_output.c ${shared_src})

if (CJSON_FOUND)
	target_link_libraries(dimq_pub ${CJSON_LIBRARIES})
	target_link_libraries(dimq_sub ${CJSON_LIBRARIES})
	target_link_libraries(dimq_rr ${CJSON_LIBRARIES})
endif()

if (WITH_STATIC_LIBRARIES)
	target_link_libraries(dimq_pub libdimq_static)
	target_link_libraries(dimq_sub libdimq_static)
	target_link_libraries(dimq_rr libdimq_static)
else()
	target_link_libraries(dimq_pub libdimq)
	target_link_libraries(dimq_sub libdimq)
	target_link_libraries(dimq_rr libdimq)
endif()

if (QNX)
    target_link_libraries(dimq_pub socket)
    target_link_libraries(dimq_sub socket)
    target_link_libraries(dimq_rr socket)
endif()

install(TARGETS dimq_pub RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(TARGETS dimq_sub RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(TARGETS dimq_rr RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
