#!/usr/bin/env python3

from dimq_test_helper import *

port = dimq_test.get_port()

rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("test-helper", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

publish_packet = dimq_test.gen_publish("bridge/ssl/test", qos=0, payload="message")

disconnect_packet = dimq_test.gen_disconnect()

sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port, connack_error="helper connack")
sock.send(publish_packet)
sock.send(disconnect_packet)
sock.close()

exit(0)
