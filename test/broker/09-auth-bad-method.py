#!/usr/bin/env python3

# Test whether sending an Authentication Method produces the correct response
# when no auth methods are defined.

from dimq_test_helper import *

rc = 1
keepalive = 10
props = mqtt5_props.gen_string_prop(mqtt5_props.PROP_AUTHENTICATION_METHOD, "basic")
connect_packet = dimq_test.gen_connect("connect-test", proto_ver=5, keepalive=keepalive, properties=props)
connack_packet = dimq_test.gen_connack(rc=mqtt5_rc.MQTT_RC_BAD_AUTHENTICATION_METHOD, proto_ver=5, properties=None)

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
    sock.close()
    rc = 0
except dimq_test.TestError:
    pass
finally:
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))

exit(rc)

