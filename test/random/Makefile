include ../../config.mk

.PHONY: all test

ifeq ($(WITH_SHARED_LIBRARIES),yes)
LIB_DEP:=../../lib/libdimq.so.${SOVERSION}
else
LIB_DEP:=../../lib/libdimq.a
endif

all : auth_plugin.so

auth_plugin.so : auth_plugin.c
	$(CC) ${CFLAGS} -fPIC -shared $< -o $@ -I../../lib -I../../src

../lib/libdimq.so.${SOVERSION} :
	$(MAKE) -C ../../lib

../lib/libdimq.a :
	$(MAKE) -C ../../lib libdimq.a

clean : 
	-rm -f *.o random_client *.gcda *.gcno

test : all
	./test.py
