include ../config.mk

.PHONY : all clean install uninstall dist 

MANPAGES = \
	libdimq.3 \
	dimq-tls.7 \
	dimq.8 \
	dimq.conf.5 \
	dimq_ctrl.1 \
	dimq_ctrl_dynsec.1 \
	dimq_passwd.1 \
	dimq_pub.1 \
	dimq_rr.1 \
	dimq_sub.1 \
	mqtt.7

all :  ${MANPAGES}

clean :

reallyclean : clean
	-rm -f *.orig
	-rm -f ${MANPAGES}

dist : ${MANPAGES}

install :
	$(INSTALL) -d "${DESTDIR}$(mandir)/man8"
	$(INSTALL) -m 644 dimq.8 "${DESTDIR}${mandir}/man8/dimq.8"
	$(INSTALL) -d "${DESTDIR}$(mandir)/man5"
	$(INSTALL) -m 644 dimq.conf.5 "${DESTDIR}${mandir}/man5/dimq.conf.5"
	$(INSTALL) -d "${DESTDIR}$(mandir)/man1"
	$(INSTALL) -m 644 dimq_ctrl.1 "${DESTDIR}${mandir}/man1/dimq_ctrl.1"
	$(INSTALL) -m 644 dimq_ctrl_dynsec.1 "${DESTDIR}${mandir}/man1/dimq_ctrl_dynsec.1"
	$(INSTALL) -m 644 dimq_passwd.1 "${DESTDIR}${mandir}/man1/dimq_passwd.1"
	$(INSTALL) -m 644 dimq_pub.1 "${DESTDIR}${mandir}/man1/dimq_pub.1"
	$(INSTALL) -m 644 dimq_sub.1 "${DESTDIR}${mandir}/man1/dimq_sub.1"
	$(INSTALL) -m 644 dimq_rr.1 "${DESTDIR}${mandir}/man1/dimq_rr.1"
	$(INSTALL) -d "${DESTDIR}$(mandir)/man7"
	$(INSTALL) -m 644 mqtt.7 "${DESTDIR}${mandir}/man7/mqtt.7"
	$(INSTALL) -m 644 dimq-tls.7 "${DESTDIR}${mandir}/man7/dimq-tls.7"
	$(INSTALL) -d "${DESTDIR}$(mandir)/man3"
	$(INSTALL) -m 644 libdimq.3 "${DESTDIR}${mandir}/man3/libdimq.3"

uninstall :
	-rm -f "${DESTDIR}${mandir}/man8/dimq.8"
	-rm -f "${DESTDIR}${mandir}/man5/dimq.conf.5"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_ctrl.1"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_ctrl_dynsec.1"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_passwd.1"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_pub.1"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_sub.1"
	-rm -f "${DESTDIR}${mandir}/man1/dimq_rr.1"
	-rm -f "${DESTDIR}${mandir}/man7/mqtt.7"
	-rm -f "${DESTDIR}${mandir}/man7/dimq-tls.7"
	-rm -f "${DESTDIR}${mandir}/man3/libdimq.3"

dimq.8 : dimq.8.xml manpage.xsl
	$(XSLTPROC) $<

dimq.conf.5 : dimq.conf.5.xml manpage.xsl
	$(XSLTPROC) $<

dimq_ctrl.1 : dimq_ctrl.1.xml manpage.xsl
	$(XSLTPROC) $<

dimq_ctrl_dynsec.1 : dimq_ctrl_dynsec.1.xml manpage.xsl
	$(XSLTPROC) $<

dimq_passwd.1 : dimq_passwd.1.xml manpage.xsl
	$(XSLTPROC) $<

dimq_pub.1 : dimq_pub.1.xml manpage.xsl
	$(XSLTPROC) $<

dimq_sub.1 : dimq_sub.1.xml manpage.xsl
	$(XSLTPROC) $<

dimq_rr.1 : dimq_rr.1.xml manpage.xsl
	$(XSLTPROC) $<

mqtt.7 : mqtt.7.xml manpage.xsl
	$(XSLTPROC) $<

dimq-tls.7 : dimq-tls.7.xml manpage.xsl
	$(XSLTPROC) $<

libdimq.3 : libdimq.3.xml manpage.xsl
	$(XSLTPROC) $<

html : *.xml
	set -e; for m in *.xml; \
		do \
		hfile=$$(echo $${m} | sed -e 's#\(.*\)\.xml#\1#' | sed -e 's/\./-/g'); \
		$(XSLTPROC) html.xsl $${m} > $${hfile}.html; \
	done

potgen :
	 xml2po -o po/dimq/dimq.8.pot dimq.8.xml
	 xml2po -o po/dimq.conf/dimq.conf.5.pot dimq.conf.5.xml
	 xml2po -o po/dimq_ctrl/dimq_ctrl.1.pot dimq_ctrl.1.xml
	 xml2po -o po/dimq_ctrl/dimq_ctrl_dynsec.1.pot dimq_ctrl_dynsec.1.xml
	 xml2po -o po/dimq_passwd/dimq_passwd.1.pot dimq_passwd.1.xml
	 xml2po -o po/dimq_pub/dimq_pub.1.pot dimq_pub.1.xml
	 xml2po -o po/dimq_sub/dimq_sub.1.pot dimq_sub.1.xml
	 xml2po -o po/dimq_sub/dimq_rr.1.pot dimq_rr.1.xml
	 xml2po -o po/mqtt/mqtt.7.pot mqtt.7.xml
	 xml2po -o po/dimq-tls/dimq-tls.7.pot dimq-tls.7.xml
	 xml2po -o po/libdimq/libdimq.3.pot libdimq.3.xml

# To merge new translations do:
# /usr/bin/xml2po -p de.po chapter1.xml > chapter1.de.xml
