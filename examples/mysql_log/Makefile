CFLAGS=-Wall -ggdb
LDFLAGS=../../lib/libdimq.so.1 -lmysqlclient

.PHONY: all clean

all : dimq_mysql_log

dimq_mysql_log : mysql_log.o
	${CC} $^ -o $@ ${LDFLAGS}

mysql_log.o : mysql_log.c
	${CC} -c $^ -o $@ ${CFLAGS} -I../../lib

clean : 
	-rm -f *.o dimq_mysql_log
