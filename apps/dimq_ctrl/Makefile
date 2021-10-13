include ../../config.mk

.PHONY: all install uninstall clean reallyclean

ifeq ($(WITH_SHARED_LIBRARIES),yes)
LIBdimq:=../../lib/libdimq.so.${SOVERSION}
else
ifeq ($(WITH_THREADING),yes)
LIBdimq:=../../lib/libdimq.a -lpthread -lssl -lcrypto
else
LIBdimq:=../../lib/libdimq.a
endif
endif

LOCAL_CPPFLAGS:=-I../dimq_passwd -DWITH_CJSON

OBJS=	dimq_ctrl.o \
		client.o \
		dynsec.o \
		dynsec_client.o \
		dynsec_group.o \
		dynsec_role.o \
		get_password.o \
		memory_dimq.o \
		memory_public.o \
		options.o \
		password_dimq.o

EXAMPLE_OBJS= example.o

ifeq ($(WITH_TLS),yes)
ifeq ($(WITH_CJSON),yes)
TARGET:=dimq_ctrl dimq_ctrl_example.so
else
TARGET:=
endif

else
TARGET:=
endif

all : ${TARGET}

dimq_ctrl : ${OBJS} ${LIBdimq}
	${CROSS_COMPILE}${CC} ${APP_LDFLAGS} $^ -o $@ $(PASSWD_LDADD) $(LOCAL_LDFLAGS) $(LIBdimq) -lcjson -ldl

dimq_ctrl_example.so : ${EXAMPLE_OBJS}
	$(CROSS_COMPILE)$(CC) $(PLUGIN_CPPFLAGS) $(PLUGIN_CFLAGS) $(PLUGIN_LDFLAGS) -shared $< -o $@

dimq_ctrl.o : dimq_ctrl.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

client.o : client.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

dynsec.o : dynsec.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

dynsec_client.o : dynsec_client.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

dynsec_group.o : dynsec_group.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

dynsec_role.o : dynsec_role.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

example.o : example.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(PLUGIN_CPPFLAGS) $(PLUGIN_CFLAGS) -c $< -o $@

get_password.o : ../dimq_passwd/get_password.c ../dimq_passwd/get_password.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

memory_dimq.o : ../../lib/memory_dimq.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

memory_public.o : ../../src/memory_public.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

options.o : options.c dimq_ctrl.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

misc_dimq.o : ../../lib/misc_dimq.c ../../lib/misc_dimq.h
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

password_dimq.o : ../../src/password_dimq.c ../../src/password_dimq.h
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

../../lib/libdimq.so.${SOVERSION} :
	$(MAKE) -C ../../lib

../../lib/libdimq.a :
	$(MAKE) -C ../../lib libdimq.a

install : all
ifeq ($(WITH_TLS),yes)
ifeq ($(WITH_CJSON),yes)
	$(INSTALL) -d "${DESTDIR}$(prefix)/bin"
	$(INSTALL) ${STRIP_OPTS} dimq_ctrl "${DESTDIR}${prefix}/bin/dimq_ctrl"
endif
endif

uninstall :
	-rm -f "${DESTDIR}${prefix}/bin/dimq_ctrl"

clean :
	-rm -f *.o dimq_ctrl *.gcda *.gcno *.so

reallyclean : clean
	-rm -rf *.orig *.db
