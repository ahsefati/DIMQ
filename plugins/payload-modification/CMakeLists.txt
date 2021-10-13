include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
			${OPENSSL_INCLUDE_DIR} ${STDBOOL_H_PATH} ${STDINT_H_PATH})
link_directories(${dimq_SOURCE_DIR})

add_library(dimq_payload_modification MODULE dimq_payload_modification.c)
set_target_properties(dimq_payload_modification PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)
set_target_properties(dimq_payload_modification PROPERTIES PREFIX "")
if(WIN32)
	target_link_libraries(dimq_payload_modification dimq)
endif(WIN32)

# Don't install, these are example plugins only.
#install(TARGETS dimq_payload_modification RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
