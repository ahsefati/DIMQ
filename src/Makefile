include ../config.mk

.PHONY: all install uninstall clean reallyclean

all : dimq

OBJS=	dimq.o \
		alias_dimq.o \
		bridge.o \
		bridge_topic.o \
		conf.o \
		conf_includedir.o \
		context.o \
		control.o \
		database.o \
		handle_auth.o \
		handle_connack.o \
		handle_connect.o \
		handle_disconnect.o \
		handle_ping.o \
		handle_pubackcomp.o \
		handle_publish.o \
		handle_pubrec.o \
		handle_pubrel.o \
		handle_suback.o \
		handle_subscribe.o \
		handle_unsuback.o \
		handle_unsubscribe.o \
		keepalive.o \
		logging.o \
		loop.o \
		memory_dimq.o \
		memory_public.o \
		misc_dimq.o \
		mux.o \
		mux_epoll.o \
		mux_poll.o \
		net.o \
		net_dimq.o \
		net_dimq_ocsp.o \
		packet_datatypes.o \
		packet_dimq.o \
		password_dimq.o \
		property_broker.o \
		property_dimq.o \
		persist_read.o \
		persist_read_v234.o \
		persist_read_v5.o \
		persist_write.o \
		persist_write_v5.o \
		plugin.o \
		plugin_public.o \
		read_handle.o \
		retain.o \
		security.o \
		security_default.o \
		send_auth.o \
		send_connack.o \
		send_connect.o \
		send_disconnect.o \
		send_dimq.o \
		send_publish.o \
		send_suback.o \
		send_subscribe.o \
		send_unsuback.o \
		send_unsubscribe.o \
		service.o \
		session_expiry.o \
		signals.o \
		strings_dimq.o \
		subs.o \
		sys_tree.o \
		time_dimq.o \
		topic_tok.o \
		tls_dimq.o \
		utf8_dimq.o \
		util_dimq.o \
		util_topic.o \
		websockets.o \
		will_delay.o \
		will_dimq.o \
		xtreport.o

dimq : ${OBJS}
	${CROSS_COMPILE}${CC} ${BROKER_LDFLAGS} $^ -o $@ $(BROKER_LDADD)

dimq.o : dimq.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

alias_dimq.o : ../lib/alias_dimq.c ../lib/alias_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

bridge.o : bridge.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

bridge_topic.o : bridge_topic.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

conf.o : conf.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

conf_includedir.o : conf_includedir.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

context.o : context.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

control.o : control.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

database.o : database.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_auth.o : handle_auth.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_connack.o : handle_connack.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_connect.o : handle_connect.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_disconnect.o : handle_disconnect.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_ping.o : ../lib/handle_ping.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_pubackcomp.o : ../lib/handle_pubackcomp.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_publish.o : handle_publish.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_pubrec.o : ../lib/handle_pubrec.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_pubrel.o : ../lib/handle_pubrel.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_suback.o : ../lib/handle_suback.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_subscribe.o : handle_subscribe.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_unsuback.o : ../lib/handle_unsuback.c ../lib/read_handle.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

handle_unsubscribe.o : handle_unsubscribe.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

keepalive.o : keepalive.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

logging.o : logging.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

loop.o : loop.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

memory_dimq.o : ../lib/memory_dimq.c ../lib/memory_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

memory_public.o : memory_public.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

misc_dimq.o : ../lib/misc_dimq.c ../lib/misc_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

mux.o : mux.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

mux_epoll.o : mux_epoll.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

mux_poll.o : mux_poll.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

net.o : net.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

net_dimq_ocsp.o : ../lib/net_dimq_ocsp.c ../lib/net_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

net_dimq.o : ../lib/net_dimq.c ../lib/net_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

password_dimq.o : password_dimq.c password_dimq.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

persist_read.o : persist_read.c persist.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

persist_read_v234.o : persist_read_v234.c persist.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

persist_read_v5.o : persist_read_v5.c persist.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

persist_write.o : persist_write.c persist.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

persist_write_v5.o : persist_write_v5.c persist.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

packet_datatypes.o : ../lib/packet_datatypes.c ../lib/packet_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

packet_dimq.o : ../lib/packet_dimq.c ../lib/packet_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

property_broker.o : property_broker.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

property_dimq.o : ../lib/property_dimq.c ../lib/property_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

plugin.o : plugin.c ../include/dimq_plugin.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

plugin_public.o : plugin_public.c ../include/dimq_plugin.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

read_handle.o : read_handle.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

retain.o : retain.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

security.o : security.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

security_default.o : security_default.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_auth.o : send_auth.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_connect.o : ../lib/send_connect.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_disconnect.o : ../lib/send_disconnect.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_connack.o : send_connack.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_dimq.o : ../lib/send_dimq.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_publish.o : ../lib/send_publish.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_suback.o : send_suback.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_subscribe.o : ../lib/send_subscribe.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_unsuback.o : send_unsuback.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

send_unsubscribe.o : ../lib/send_unsubscribe.c ../lib/send_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

service.o : service.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

session_expiry.o : session_expiry.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

signals.o : signals.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

strings_dimq.o : ../lib/strings_dimq.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

subs.o : subs.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

sys_tree.o : sys_tree.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

time_dimq.o : ../lib/time_dimq.c ../lib/time_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

tls_dimq.o : ../lib/tls_dimq.c
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

topic_tok.o : topic_tok.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

util_dimq.o : ../lib/util_dimq.c ../lib/util_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

util_topic.o : ../lib/util_topic.c ../lib/util_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

utf8_dimq.o : ../lib/utf8_dimq.c
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

websockets.o : websockets.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

will_delay.o : will_delay.c dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

will_dimq.o : ../lib/will_dimq.c ../lib/will_dimq.h
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

xtreport.o : xtreport.c
	${CROSS_COMPILE}${CC} $(BROKER_CPPFLAGS) $(BROKER_CFLAGS) -c $< -o $@

plugin_defer.so : plugin_defer.c ../include/dimq_plugin.h ../include/dimq_broker.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} -I. -I../lib -fPIC -shared $< -o $@

plugin_debug.so : plugin_debug.c ../include/dimq_plugin.../include/h dimq_broker.h dimq_broker_internal.h
	${CROSS_COMPILE}${CC} -I. -I../lib -fPIC -shared $< -o $@

install : all
	$(INSTALL) -d "${DESTDIR}$(prefix)/sbin"
	$(INSTALL) ${STRIP_OPTS} dimq "${DESTDIR}${prefix}/sbin/dimq"
	$(INSTALL) -d "${DESTDIR}$(prefix)/include"
	$(INSTALL) ../include/dimq_broker.h "${DESTDIR}${prefix}/include/dimq_broker.h"
	$(INSTALL) ../include/dimq_plugin.h "${DESTDIR}${prefix}/include/dimq_plugin.h"

uninstall :
	-rm -f "${DESTDIR}${prefix}/sbin/dimq"
	-rm -f "${DESTDIR}${prefix}/include/dimq_broker.h"
	-rm -f "${DESTDIR}${prefix}/include/dimq_plugin.h"

clean : 
	-rm -f *.o dimq *.gcda *.gcno

reallyclean : clean
	-rm -rf *.orig *.db
