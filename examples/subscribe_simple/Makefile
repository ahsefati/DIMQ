include ../../config.mk

.PHONY: all

all : sub_callback sub_single sub_multiple

sub_callback : callback.o
	${CROSS_COMPILE}${CC} $^ -o $@ ../../lib/libdimq.so.${SOVERSION}

sub_single : single.o
	${CROSS_COMPILE}${CC} $^ -o $@ ../../lib/libdimq.so.${SOVERSION}

sub_multiple : multiple.o
	${CROSS_COMPILE}${CC} $^ -o $@ ../../lib/libdimq.so.${SOVERSION}

callback.o : callback.c ../../lib/libdimq.so.${SOVERSION}
	${CROSS_COMPILE}${CC} -c $< -o $@ -I../../lib ${CFLAGS}

single.o : single.c ../../lib/libdimq.so.${SOVERSION}
	${CROSS_COMPILE}${CC} -c $< -o $@ -I../../lib ${CFLAGS}

multiple.o : multiple.c ../../lib/libdimq.so.${SOVERSION}
	${CROSS_COMPILE}${CC} -c $< -o $@ -I../../lib ${CFLAGS}

../../lib/libdimq.so.${SOVERSION} :
	$(MAKE) -C ../../lib

clean : 
	-rm -f *.o sub_single sub_multiple
