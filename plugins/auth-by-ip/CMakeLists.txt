include_directories(${dimq_SOURCE_DIR} ${dimq_SOURCE_DIR}/include
			${OPENSSL_INCLUDE_DIR} ${STDBOOL_H_PATH} ${STDINT_H_PATH})

add_library(dimq_auth_by_ip MODULE dimq_auth_by_ip.c)
set_target_properties(dimq_auth_by_ip PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)
set_target_properties(dimq_auth_by_ip PROPERTIES PREFIX "")

# Don't install, these are example plugins only.
#install(TARGETS dimq_auth_by_ip RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
