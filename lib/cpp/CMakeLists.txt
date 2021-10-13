include_directories(${dimq_SOURCE_DIR}/lib ${dimq_SOURCE_DIR}/lib/cpp
			${dimq_SOURCE_DIR}/include
			${STDBOOL_H_PATH} ${STDINT_H_PATH})
link_directories(${dimq_BINARY_DIR}/lib)

set(CPP_SRC dimqpp.cpp dimqpp.h)

add_library(dimqpp SHARED ${CPP_SRC})
set_target_properties(dimqpp PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)
target_link_libraries(dimqpp libdimq)
set_target_properties(dimqpp PROPERTIES
	VERSION ${VERSION}
	SOVERSION 1
)
install(TARGETS dimqpp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")

if (WITH_STATIC_LIBRARIES)
	add_library(dimqpp_static STATIC
		${C_SRC}
		${CPP_SRC}
	)
	if (WITH_PIC)
		set_target_properties(dimqpp_static PROPERTIES
			POSITION_INDEPENDENT_CODE 1
		)
	endif (WITH_PIC)

	target_link_libraries(dimqpp_static ${LIBRARIES})

	set_target_properties(dimqpp_static PROPERTIES
		OUTPUT_NAME dimqpp_static
		VERSION ${VERSION}
	)

	target_compile_definitions(dimqpp_static PUBLIC "LIBdimq_STATIC")
	install(TARGETS dimqpp_static ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif (WITH_STATIC_LIBRARIES)

install(FILES dimqpp.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
