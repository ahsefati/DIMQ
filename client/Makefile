include ../config.mk

.PHONY: all install uninstall reallyclean clean static static_pub static_sub static_rr

ifeq ($(WITH_SHARED_LIBRARIES),yes)
SHARED_DEP:=../lib/libdimq.so.${SOVERSION}
endif

ifeq ($(WITH_SHARED_LIBRARIES),yes)
ALL_DEPS:= dimq_pub dimq_sub dimq_rr
else
ifeq ($(WITH_STATIC_LIBRARIES),yes)
ALL_DEPS:= static_pub static_sub static_rr
endif
endif

all : ${ALL_DEPS}

static : static_pub static_sub static_rr
	# This makes dimq_pub/sub/rr versions that are statically linked with
	# libdimq only.

static_pub : pub_client.o pub_shared.o client_props.o client_shared.o ../lib/libdimq.a
	${CROSS_COMPILE}${CC} $^ -o dimq_pub ${CLIENT_LDFLAGS} ${STATIC_LIB_DEPS} ${CLIENT_STATIC_LDADD}

static_sub : sub_client.o sub_client_output.o client_props.o client_shared.o ../lib/libdimq.a
	${CROSS_COMPILE}${CC} $^ -o dimq_sub ${CLIENT_LDFLAGS} ${STATIC_LIB_DEPS} ${CLIENT_STATIC_LDADD}

static_rr : rr_client.o client_props.o client_shared.o pub_shared.o sub_client_output.o ../lib/libdimq.a
	${CROSS_COMPILE}${CC} $^ -o dimq_rr ${CLIENT_LDFLAGS} ${STATIC_LIB_DEPS} ${CLIENT_STATIC_LDADD}

dimq_pub : pub_client.o pub_shared.o client_shared.o client_props.o
	${CROSS_COMPILE}${CC} $(CLIENT_LDFLAGS) $^ -o $@ $(CLIENT_LDADD)

dimq_sub : sub_client.o sub_client_output.o client_shared.o client_props.o
	${CROSS_COMPILE}${CC} $(CLIENT_LDFLAGS) $^ -o $@ $(CLIENT_LDADD)

dimq_rr : rr_client.o client_shared.o client_props.o pub_shared.o sub_client_output.o
	${CROSS_COMPILE}${CC} $(CLIENT_LDFLAGS) $^ -o $@ $(CLIENT_LDADD)

pub_client.o : pub_client.c ${SHARED_DEP}
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

pub_shared.o : pub_shared.c ${SHARED_DEP}
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

sub_client.o : sub_client.c ${SHARED_DEP}
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

sub_client_output.o : sub_client_output.c sub_client_output.h ${SHARED_DEP}
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

rr_client.o : rr_client.c ${SHARED_DEP}
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

client_shared.o : client_shared.c client_shared.h
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

client_props.o : client_props.c client_shared.h
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@

# The "testing" target is intended to make it easy to compile a quick client
# for testing purposes. testing.c should not be committed as a file.
testing : testing.o
	${CROSS_COMPILE}${CC} $(CLIENT_LDFLAGS) $^ -o $@ $(CLIENT_LDADD) $(CLIENT_LDFLAGS)

testing.o : testing.c
	${CROSS_COMPILE}${CC} $(CLIENT_CPPFLAGS) $(CLIENT_CFLAGS) -c $< -o $@


../lib/libdimq.so.${SOVERSION} :
	$(MAKE) -C ../lib

../lib/libdimq.a :
	$(MAKE) -C ../lib libdimq.a

install : all
	$(INSTALL) -d "${DESTDIR}$(prefix)/bin"
	$(INSTALL) ${STRIP_OPTS} dimq_pub "${DESTDIR}${prefix}/bin/dimq_pub"
	$(INSTALL) ${STRIP_OPTS} dimq_sub "${DESTDIR}${prefix}/bin/dimq_sub"
	$(INSTALL) ${STRIP_OPTS} dimq_rr "${DESTDIR}${prefix}/bin/dimq_rr"

uninstall :
	-rm -f "${DESTDIR}${prefix}/bin/dimq_pub"
	-rm -f "${DESTDIR}${prefix}/bin/dimq_sub"
	-rm -f "${DESTDIR}${prefix}/bin/dimq_rr"

reallyclean : clean

clean : 
	-rm -f *.o dimq_pub dimq_sub dimq_rr *.gcda *.gcno
