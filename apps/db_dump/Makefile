include ../../config.mk

CFLAGS_FINAL=${CFLAGS} -I../../include -I../../ -I../../lib -I../../src -I../../deps -DWITH_BROKER -DWITH_PERSISTENCE

OBJS = \
	   db_dump.o \
	   print.o \
	   \
	   memory_dimq.o \
	   memory_public.o \
	   packet_datatypes.o \
	   packet_dimq.o \
	   persist_read.o \
	   persist_read_v234.o \
	   persist_read_v5.o \
	   property_dimq.o \
	   send_disconnect.o \
	   stubs.o \
	   time_dimq.o \
	   topic_tok.o \
	   utf8_dimq.o

.PHONY: all clean reallyclean

all : dimq_db_dump

dimq_db_dump : ${OBJS}
	${CROSS_COMPILE}${CC} $^ -o $@ ${LDFLAGS} ${LIBS}

db_dump.o : db_dump.c db_dump.h ../../src/persist.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

print.o : print.c db_dump.h ../../src/persist.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

memory_dimq.o : ../../lib/memory_dimq.c ../../lib/memory_dimq.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

memory_public.o : ../../src/memory_public.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

net_dimq.o : ../../lib/net_dimq.c ../../lib/net_dimq.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

packet_datatypes.o : ../../lib/packet_datatypes.c ../../lib/packet_dimq.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

packet_dimq.o : ../../lib/packet_dimq.c ../../lib/packet_dimq.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

persist_read.o : ../../src/persist_read.c ../../src/persist.h ../../src/dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

persist_read_v234.o : ../../src/persist_read_v234.c ../../src/persist.h ../../src/dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

persist_read_v5.o : ../../src/persist_read_v5.c ../../src/persist.h ../../src/dimq_broker_internal.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

property_dimq.o : ../../lib/property_dimq.c ../../lib/property_dimq.h
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

read_handle.o : ../../src/read_handle.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

stubs.o : stubs.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

send_disconnect.o : ../../lib/send_disconnect.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

time_dimq.o : ../../lib/time_dimq.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

topic_tok.o : ../../src/topic_tok.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

utf8_dimq.o : ../../lib/utf8_dimq.c
	${CROSS_COMPILE}${CC} $(CFLAGS_FINAL) -c $< -o $@

reallyclean: clean

clean :
	-rm -f *.o dimq_db_dump

install:

uninstall:
