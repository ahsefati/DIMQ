include config.mk

DIRS=lib apps client plugins src
DOCDIRS=man
DISTDIRS=man
DISTFILES= \
	apps/ \
	client/ \
	cmake/ \
	deps/ \
	examples/ \
	include/ \
	installer/ \
	lib/ \
	logo/ \
	man/ \
	misc/ \
	plugins/ \
	security/ \
	service/ \
	snap/ \
	src/ \
	test/ \
	\
	CMakeLists.txt \
	CONTRIBUTING.md \
	ChangeLog.txt \
	LICENSE.txt \
	Makefile \
	about.html \
	aclfile.example \
	config.h \
	config.mk \
	edl-v10 \
	epl-v20 \
	libdimq.pc.in \
	libdimqpp.pc.in \
	dimq.conf \
	addresses.conf\
	NOTICE.md \
	pskfile.example \
	pwfile.example \
	README-compiling.md \
	README-letsencrypt.md \
	README-windows.txt \
	README.md

.PHONY : all dimq api docs binary check clean reallyclean test install uninstall dist sign copy localdocker

all : $(MAKE_ALL)

api :
	mkdir -p api p
	naturaldocs -o HTML api -i lib -p p
	rm -rf p

docs :
	set -e; for d in ${DOCDIRS}; do $(MAKE) -C $${d}; done

binary : dimq

dimq :
ifeq ($(UNAME),Darwin)
	$(error Please compile using CMake on Mac OS X)
endif

	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

clean :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} clean; done
	set -e; for d in ${DOCDIRS}; do $(MAKE) -C $${d} clean; done
	$(MAKE) -C test clean

reallyclean : 
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} reallyclean; done
	set -e; for d in ${DOCDIRS}; do $(MAKE) -C $${d} reallyclean; done
	$(MAKE) -C test reallyclean
	-rm -f *.orig

check : test

test : dimq
	$(MAKE) -C test test

ptest : dimq
	$(MAKE) -C test ptest

utest : dimq
	$(MAKE) -C test utest

install : all
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} install; done
ifeq ($(WITH_DOCS),yes)
	set -e; for d in ${DOCDIRS}; do $(MAKE) -C $${d} install; done
endif
	$(INSTALL) -d "${DESTDIR}/etc/dimq"
	$(INSTALL) -m 644 dimq.conf "${DESTDIR}/etc/dimq/dimq.conf.example"
	$(INSTALL) -m 644 addresses.conf "${DESTDIR}/etc/dimq/addresses.conf"
	$(INSTALL) -m 644 aclfile.example "${DESTDIR}/etc/dimq/aclfile.example"
	$(INSTALL) -m 644 pwfile.example "${DESTDIR}/etc/dimq/pwfile.example"
	$(INSTALL) -m 644 pskfile.example "${DESTDIR}/etc/dimq/pskfile.example"

uninstall :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} uninstall; done
	rm -f "${DESTDIR}/etc/dimq/dimq.conf.example"
	rm -f "${DESTDIR}/etc/dimq/aclfile.example"
	rm -f "${DESTDIR}/etc/dimq/pwfile.example"
	rm -f "${DESTDIR}/etc/dimq/pskfile.example"

dist : reallyclean
	set -e; for d in ${DISTDIRS}; do $(MAKE) -C $${d} dist; done
	
	mkdir -p dist/dimq-${VERSION}
	cp -r ${DISTFILES} dist/dimq-${VERSION}/
	cd dist; tar -zcf dimq-${VERSION}.tar.gz dimq-${VERSION}/

sign : dist
	cd dist; gpg --detach-sign -a dimq-${VERSION}.tar.gz

copy : sign
	cd dist; scp dimq-${VERSION}.tar.gz dimq-${VERSION}.tar.gz.asc dimq:site/dimq.org/files/source/
	scp ChangeLog.txt dimq:site/dimq.org/

coverage :
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory out

localdocker : reallyclean
	set -e; for d in ${DISTDIRS}; do $(MAKE) -C $${d} dist; done
	
	rm -rf dockertmp/
	mkdir -p dockertmp/dimq-${VERSION}
	cp -r ${DISTFILES} dockertmp/dimq-${VERSION}/
	cd dockertmp/; tar -zcf dimq.tar.gz dimq-${VERSION}/
	cp dockertmp/dimq.tar.gz docker/local
	rm -rf dockertmp/
	cd docker/local && docker build . -t eclipse-dimq:local

