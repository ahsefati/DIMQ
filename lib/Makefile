include ../config.mk

.PHONY : really clean install

dimq_OBJS=dimq.o \
		  actions.o \
		  callbacks.o \
		  connect.o \
		  handle_auth.o \
		  handle_connack.o \
		  handle_disconnect.o \
		  handle_ping.o \
		  handle_pubackcomp.o \
		  handle_publish.o \
		  handle_pubrec.o \
		  handle_pubrel.o \
		  handle_suback.o \
		  handle_unsuback.o \
		  helpers.o \
		  logging_dimq.o \
		  loop.o \
		  memory_dimq.o \
		  messages_dimq.o \
		  misc_dimq.o \
		  net_dimq_ocsp.o \
		  net_dimq.o \
		  options.o \
		  packet_datatypes.o \
		  packet_dimq.o \
		  property_dimq.o \
		  read_handle.o \
		  send_connect.o \
		  send_disconnect.o \
		  send_dimq.o \
		  send_publish.o \
		  send_subscribe.o \
		  send_unsubscribe.o \
		  socks_dimq.o \
		  srv_dimq.o \
		  strings_dimq.o \
		  thread_dimq.o \
		  time_dimq.o \
		  tls_dimq.o \
		  utf8_dimq.o \
		  util_dimq.o \
		  util_topic.o \
		  will_dimq.o

ALL_DEPS:=

ifeq ($(WITH_SHARED_LIBRARIES),yes)
	ALL_DEPS+=libdimq.so.${SOVERSION}
endif

ifeq ($(WITH_STATIC_LIBRARIES),yes)
	ALL_DEPS+=libdimq.a
endif

all : ${ALL_DEPS}
ifeq ($(WITH_SHARED_LIBRARIES),yes)
	$(MAKE) -C cpp
endif

install : all
	$(INSTALL) -d "${DESTDIR}${libdir}/"
ifeq ($(WITH_SHARED_LIBRARIES),yes)
	$(INSTALL) ${STRIP_OPTS} libdimq.so.${SOVERSION} "${DESTDIR}${libdir}/libdimq.so.${SOVERSION}"
	ln -sf libdimq.so.${SOVERSION} "${DESTDIR}${libdir}/libdimq.so"
endif
ifeq ($(WITH_STATIC_LIBRARIES),yes)
	$(INSTALL) ${STRIP_OPTS} libdimq.a "${DESTDIR}${libdir}/libdimq.a"
endif
	$(INSTALL) -d "${DESTDIR}${incdir}/"
	$(INSTALL) ../include/dimq.h "${DESTDIR}${incdir}/dimq.h"
	$(INSTALL) ../include/mqtt_protocol.h "${DESTDIR}${incdir}/mqtt_protocol.h"
	$(INSTALL) -d "${DESTDIR}${libdir}/pkgconfig"
	$(INSTALL) -m644 ../libdimq.pc.in "${DESTDIR}${libdir}/pkgconfig/libdimq.pc"
	sed ${SEDINPLACE} -e "s#@CMAKE_INSTALL_PREFIX@#${prefix}#" -e "s#@VERSION@#${VERSION}#" "${DESTDIR}${libdir}/pkgconfig/libdimq.pc"
ifeq ($(WITH_SHARED_LIBRARIES),yes)
	$(MAKE) -C cpp install
endif

uninstall :
	-rm -f "${DESTDIR}${libdir}/libdimq.so.${SOVERSION}"
	-rm -f "${DESTDIR}${libdir}/libdimq.so"
	-rm -f "${DESTDIR}${libdir}/libdimq.a"
	-rm -f "${DESTDIR}${incdir}/dimq.h"

reallyclean : clean

clean :
	-rm -f *.o libdimq.so.${SOVERSION} libdimq.so libdimq.a *.gcno *.gcda
	$(MAKE) -C cpp clean

libdimq.so.${SOVERSION} : ${dimq_OBJS}
	${CROSS_COMPILE}$(CC) -shared $(LIB_LDFLAGS) $^ -o $@ ${LIB_LIBADD}

libdimq.a : ${dimq_OBJS}
	${CROSS_COMPILE}$(AR) cr $@ $^

dimq.o : dimq.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

actions.o : actions.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

callbacks.o : callbacks.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

connect.o : connect.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_auth.o : handle_auth.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_connack.o : handle_connack.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_disconnect.o : handle_disconnect.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_publish.o : handle_publish.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_ping.o : handle_ping.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_pubackcomp.o : handle_pubackcomp.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_pubrec.o : handle_pubrec.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_pubrel.o : handle_pubrel.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_suback.o : handle_suback.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

handle_unsuback.o : handle_unsuback.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

helpers.o : helpers.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

logging_dimq.o : logging_dimq.c logging_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

loop.o : loop.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

messages_dimq.o : messages_dimq.c messages_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

memory_dimq.o : memory_dimq.c memory_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

misc_dimq.o : misc_dimq.c misc_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

net_dimq_ocsp.o : net_dimq_ocsp.c net_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

net_dimq.o : net_dimq.c net_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

options.o : options.c ../include/dimq.h dimq_internal.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

packet_datatypes.o : packet_datatypes.c packet_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

packet_dimq.o : packet_dimq.c packet_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

property_dimq.o : property_dimq.c property_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

read_handle.o : read_handle.c read_handle.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_connect.o : send_connect.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_disconnect.o : send_disconnect.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_dimq.o : send_dimq.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_publish.o : send_publish.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_subscribe.o : send_subscribe.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

send_unsubscribe.o : send_unsubscribe.c send_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

socks_dimq.o : socks_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

srv_dimq.o : srv_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

strings_dimq.o : strings_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

thread_dimq.o : thread_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

time_dimq.o : time_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

tls_dimq.o : tls_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

utf8_dimq.o : utf8_dimq.c
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

util_dimq.o : util_dimq.c util_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

util_topic.o : util_topic.c util_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

will_dimq.o : will_dimq.c will_dimq.h
	${CROSS_COMPILE}$(CC) $(LIB_CPPFLAGS) $(LIB_CFLAGS) -c $< -o $@

