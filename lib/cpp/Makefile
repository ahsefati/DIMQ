include ../../config.mk

ifneq ($(UNAME),SunOS)
	LIB_LDFLAGS:=$(LDFLAGS) -Wl,-soname,libdimqpp.so.${SOVERSION}
endif

.PHONY : clean install

ALL_DEPS=libdimqpp.so.${SOVERSION}

ifeq ($(WITH_STATIC_LIBRARIES),yes)
    ALL_DEPS+=libdimqpp.a
endif

all : ${ALL_DEPS}

install : all
	$(INSTALL) -d "${DESTDIR}${libdir}/"
	$(INSTALL) ${STRIP_OPTS} libdimqpp.so.${SOVERSION} "${DESTDIR}${libdir}/libdimqpp.so.${SOVERSION}"
	ln -sf libdimqpp.so.${SOVERSION} "${DESTDIR}${libdir}/libdimqpp.so"
ifeq ($(WITH_STATIC_LIBRARIES),yes)
	$(INSTALL) libdimqpp.a "${DESTDIR}${libdir}/libdimqpp.a"
	${CROSS_COMPILE}${STRIP} -g --strip-unneeded "${DESTDIR}${libdir}/libdimqpp.a"
endif
	$(INSTALL) -d "${DESTDIR}${incdir}/"
	$(INSTALL) dimqpp.h "${DESTDIR}${incdir}/dimqpp.h"
	$(INSTALL) -d "${DESTDIR}${libdir}/pkgconfig/"
	$(INSTALL) -m644 ../../libdimqpp.pc.in "${DESTDIR}${libdir}/pkgconfig/libdimqpp.pc"
	sed ${SEDINPLACE} -e "s#@CMAKE_INSTALL_PREFIX@#${prefix}#" -e "s#@VERSION@#${VERSION}#" "${DESTDIR}${libdir}/pkgconfig/libdimqpp.pc"

uninstall :
	-rm -f "${DESTDIR}${libdir}/libdimqpp.so.${SOVERSION}"
	-rm -f "${DESTDIR}${libdir}/libdimqpp.so"
	-rm -f "${DESTDIR}${libdir}/libdimqpp.a"
	-rm -f "${DESTDIR}${incdir}/dimqpp.h"

clean :
	-rm -f *.o libdimqpp.so.${SOVERSION} libdimqpp.a

libdimqpp.so.${SOVERSION} : dimqpp.o
	${CROSS_COMPILE}$(CXX) -shared $(LIB_LDFLAGS) $< -o $@ ../libdimq.so.${SOVERSION} $(LIB_LIDADD)

libdimqpp.a : dimqpp.o
	${CROSS_COMPILE}$(AR) cr $@ $^

dimqpp.o : dimqpp.cpp dimqpp.h
	${CROSS_COMPILE}$(CXX) $(LIB_CPPFLAGS) $(LIB_CXXFLAGS) -c $< -o $@

