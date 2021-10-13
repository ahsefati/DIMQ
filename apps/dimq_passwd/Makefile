include ../../config.mk

.PHONY: all install uninstall clean reallyclean

OBJS=	dimq_passwd.o \
		get_password.o \
		memory_dimq.o \
		memory_public.o \
		misc_dimq.o \
		password_dimq.o

ifeq ($(WITH_TLS),yes)
all: dimq_passwd
else
all:
endif

dimq_passwd : ${OBJS}
	${CROSS_COMPILE}${CC} ${APP_LDFLAGS} $^ -o $@ $(PASSWD_LDADD)

dimq_passwd.o : dimq_passwd.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

get_password.o : get_password.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

memory_dimq.o : ../../lib/memory_dimq.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

memory_public.o : ../../src/memory_public.c
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

misc_dimq.o : ../../lib/misc_dimq.c ../../lib/misc_dimq.h
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

password_dimq.o : ../../src/password_dimq.c ../../src/password_dimq.h
	${CROSS_COMPILE}${CC} $(APP_CPPFLAGS) $(APP_CFLAGS) -c $< -o $@

install : all
ifeq ($(WITH_TLS),yes)
	$(INSTALL) -d "${DESTDIR}$(prefix)/bin"
	$(INSTALL) ${STRIP_OPTS} dimq_passwd "${DESTDIR}${prefix}/bin/dimq_passwd"
endif

uninstall :
	-rm -f "${DESTDIR}${prefix}/bin/dimq_passwd"

clean :
	-rm -f *.o dimq_passwd *.gcda *.gcno

reallyclean : clean
	-rm -rf *.orig *.db
