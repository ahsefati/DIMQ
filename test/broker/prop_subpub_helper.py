#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.
# Does a given property get sent through?
# MQTT v5

from dimq_test_helper import *

def prop_subpub_helper(props_out, props_in, expect_proto_error=False):
    rc = 1
    mid = 53
    keepalive = 60
    connect_packet = dimq_test.gen_connect("subpub-qos0-test", keepalive=keepalive, proto_ver=5)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos0", 0, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    publish_packet_out = dimq_test.gen_publish("subpub/qos0", qos=0, payload="message", proto_ver=5, properties=props_out)

    publish_packet_expected = dimq_test.gen_publish("subpub/qos0", qos=0, payload="message", proto_ver=5, properties=props_in)

    disconnect_packet = dimq_test.gen_disconnect(reason_code=mqtt5_rc.MQTT_RC_PROTOCOL_ERROR, proto_ver=5)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        if expect_proto_error:
            dimq_test.do_send_receive(sock, publish_packet_out, disconnect_packet, "publish")
        else:
            dimq_test.do_send_receive(sock, publish_packet_out, publish_packet_expected, "publish")

        rc = 0

        sock.close()
    except dimq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))

    exit(rc)

