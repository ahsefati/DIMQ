include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
			${OPENSSL_INCLUDE_DIR} ${STDBOOL_H_PATH} ${STDINT_H_PATH})

add_library(dimq_message_timestamp MODULE dimq_message_timestamp.c)
set_target_properties(dimq_message_timestamp PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)
set_target_properties(dimq_message_timestamp PROPERTIES PREFIX "")

# Don't install, these are example plugins only.
#install(TARGETS dimq_message_timestamp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
