# If we are building from a release tarball, the man pages should already be built, so them.
# If we are building from git, then the man pages will not be built. In this
# case, attempt to find xsltproc, and if found build the man pages. If xsltproc
# could not be found, then the man pages will not be built or installed -
# because the install is optional.

if(NOT WIN32)
	find_program(XSLTPROC xsltproc OPTIONAL)
	if(XSLTPROC)
		function(compile_manpage page)
			add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/man/${page}
				COMMAND xsltproc ${CMAKE_SOURCE_DIR}/man/${page}.xml -o ${CMAKE_SOURCE_DIR}/man/
				MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/man/${page}.xml)
			add_custom_target(${page} ALL DEPENDS ${CMAKE_SOURCE_DIR}/man/${page})
		endfunction()

		compile_manpage("dimq_ctrl.1")
		compile_manpage("dimq_ctrl_dynsec.1")
		compile_manpage("dimq_passwd.1")
		compile_manpage("dimq_pub.1")
		compile_manpage("dimq_sub.1")
		compile_manpage("dimq_rr.1")
		compile_manpage("libdimq.3")
		compile_manpage("dimq.conf.5")
		compile_manpage("dimq-tls.7")
		compile_manpage("mqtt.7")
		compile_manpage("dimq.8")
	else()
		message(FATAL_ERROR "xsltproc not found: manpages cannot be built")
	endif()

endif()

install(FILES
	dimq_ctrl.1
	dimq_ctrl_dynsec.1
	dimq_passwd.1
	dimq_pub.1
	dimq_sub.1
	dimq_rr.1
	DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
	OPTIONAL)

install(FILES libdimq.3 DESTINATION ${CMAKE_INSTALL_MANDIR}/man3 OPTIONAL)
install(FILES dimq.conf.5 DESTINATION ${CMAKE_INSTALL_MANDIR}/man5 OPTIONAL)
install(FILES dimq-tls.7 mqtt.7 DESTINATION ${CMAKE_INSTALL_MANDIR}/man7 OPTIONAL)
install(FILES dimq.8 DESTINATION ${CMAKE_INSTALL_MANDIR}/man8 OPTIONAL)
