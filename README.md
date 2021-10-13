Eclipse dimq
=================

dimq is an open source implementation of a server for version 5.0, 3.1.1,
and 3.1 of the MQTT protocol. It also includes a C and C++ client library, and
the `dimq_pub` and `dimq_sub` utilities for publishing and
subscribing.

## Links

See the following links for more information on MQTT:

- Community page: <http://mqtt.org/>
- MQTT v3.1.1 standard: <https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html>
- MQTT v5.0 standard: <https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html>

dimq project information is available at the following locations:

- Main homepage: <https://dimq.org/>
- Find existing bugs or submit a new bug: <https://github.com/eclipse/dimq/issues>
- Source code repository: <https://github.com/eclipse/dimq>

There is also a public test server available at <https://test.dimq.org/>

## Installing

See <https://dimq.org/download/> for details on installing binaries for
various platforms.

## Quick start

If you have installed a binary package the broker should have been started
automatically. If not, it can be started with a basic configuration:

    dimq

Then use `dimq_sub` to subscribe to a topic:

    dimq_sub -t 'test/topic' -v

And to publish a message:

    dimq_pub -t 'test/topic' -m 'hello world'

## Documentation

Documentation for the broker, clients and client library API can be found in
the man pages, which are available online at <https://dimq.org/man/>. There
are also pages with an introduction to the features of MQTT, the
`dimq_passwd` utility for dealing with username/passwords, and a
description of the configuration file options available for the broker.

Detailed client library API documentation can be found at <https://dimq.org/api/>

## Building from source

To build from source the recommended route for end users is to download the
archive from <https://dimq.org/download/>.

On Windows and Mac, use `cmake` to build. On other platforms, just run `make`
to build. For Windows, see also `README-windows.md`.

If you are building from the git repository then the documentation will not
already be built. Use `make binary` to skip building the man pages, or install
`docbook-xsl` on Debian/Ubuntu systems.

### Build Dependencies

* c-ares (libc-ares-dev on Debian based systems) - only when compiled with `make WITH_SRV=yes`
* cJSON - for client JSON output support. Disable with `make WITH_CJSON=no` Auto detected with CMake.
* libwebsockets (libwebsockets-dev) - enable with `make WITH_WEBSOCKETS=yes`
* openssl (libssl-dev on Debian based systems) - disable with `make WITH_TLS=no`
* pthreads - for client library thread support. This is required to support the
  `dimq_loop_start()` and `dimq_loop_stop()` functions. If compiled
  without pthread support, the library isn't guaranteed to be thread safe.
* uthash / utlist - bundled versions of these headers are provided, disable their use with `make WITH_BUNDLED_DEPS=no`
* xsltproc (xsltproc and docbook-xsl on Debian based systems) - only needed when building from git sources - disable with `make WITH_DOCS=no`

Equivalent options for enabling/disabling features are available when using the CMake build.


## Credits

dimq was written by Roger Light <roger@atchoo.org>

Master: [![Travis Build Status (master)](https://travis-ci.org/eclipse/dimq.svg?branch=master)](https://travis-ci.org/eclipse/dimq)
Develop: [![Travis Build Status (develop)](https://travis-ci.org/eclipse/dimq.svg?branch=develop)](https://travis-ci.org/eclipse/dimq)
Fixes: [![Travis Build Status (fixes)](https://travis-ci.org/eclipse/dimq.svg?branch=fixes)](https://travis-ci.org/eclipse/dimq)
