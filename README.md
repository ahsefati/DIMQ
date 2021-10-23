DimQ 
=================

DimQ is an open source fork of mosquitto broker for use in IoT applications and also every other messaging apps.
DimQ stands for Distributed Intelligent MQTT. It is built above MQTT protocol which use Pub/Sub method for its communicatins. Current implementation supports version 5.0, 3.1.1 and 3.1 of the MQTT protocol. It also includes a C, C++, python and reacti native client open-source codes to be used in your applications.
Also, there is a C library for use as command line which called `dimq_pub` and `dimq_sub` for publishing and subscribing.

## Links

See the following links for more information on MQTT:

- Community page: <http://mqtt.org/>
- MQTT v3.1.1 standard: <https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html>
- MQTT v5.0 standard: <https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html>

DimQ project information is available at the Instructions folder in this repository.
There is also a public test server available at 74.208.35.55 on the default port(1883). It should work with any MQTT clients.



## Installing
### git clone https://github.com/ahsefati/dimq.git
   ### Installing dependencies
   #### `apt-get install docbook-xsl`
   #### `apt-get install xsltproc`
   #### `cd cjson`
   #### `sudo make install`
   #### `cd ..`
   ### `sudo make install`
   

## Quick start
If you installed DimQ with previous steps, you can now run it using following commands everywhere in your terminal:
    
### dimq
    for running DimQ broker with default and secure configurations. It would limit access from remote machines.

### dimq -c dimq.conf
    for running DimQ broker with all of configurations needed to be intelligent and distributed.

Then you can use `dimq_sub` to subscribe to a topic:

#### dimq_sub -h ip -p port -t 'test/topic'

And to publish a message use `dimq_pub`:

#### dimq_pub -h ip -p port -t 'test/topic' -m "Hello Distributed Intelligent World!"

## Documentation (to be completed)


### Other Build Dependencies

* c-ares (libc-ares-dev on Debian based systems) - only when compiled with `make WITH_SRV=yes`
* libwebsockets (libwebsockets-dev) - enable with `make WITH_WEBSOCKETS=yes`
* openssl (libssl-dev on Debian based systems) - disable with `make WITH_TLS=no`
* pthreads - for client library thread support. This is required to support the
  `dimq_loop_start()` and `dimq_loop_stop()` functions. If compiled
  without pthread support, the library isn't guaranteed to be thread safe.
* uthash / utlist - bundled versions of these headers are provided, disable their use with `make WITH_BUNDLED_DEPS=no`

Equivalent options for enabling/disabling features are available when using the CMake build.


## Credits
MQTT was written by Andy Stanford-Clark and Arlen Nipper (From IBM)
Mosquitto was written by Roger Light
