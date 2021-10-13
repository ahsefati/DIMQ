#!/usr/bin/env python3

# Test whether a client will is transmitted when a client disconnects with DISCONNECT with will.
# MQTT 5

from dimq_test_helper import *

def do_test():
    rc = 1
    keepalive = 60

    mid = 1
    connect1_packet = dimq_test.gen_connect("will-qos0-test", keepalive=keepalive, proto_ver=5)
    connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    connect2_packet = dimq_test.gen_connect("will-helper", keepalive=keepalive, proto_ver=5, will_topic="will/test", will_payload=b"will delay", will_qos=2)
    connack2_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
    disconnect_packet = dimq_test.gen_disconnect(reason_code=4, proto_ver=5)

    subscribe_packet = dimq_test.gen_subscribe(mid, "will/test", 0, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    publish_packet = dimq_test.gen_publish("will/test", qos=0, payload="will delay", proto_ver=5)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock1 = dimq_test.do_client_connect(connect1_packet, connack1_packet, timeout=30, port=port)
        dimq_test.do_send_receive(sock1, subscribe_packet, suback_packet, "suback")

        sock2 = dimq_test.do_client_connect(connect2_packet, connack2_packet, timeout=30, port=port)
        sock2.send(disconnect_packet)

        dimq_test.expect_packet(sock1, "publish", publish_packet)
        rc = 0

        sock2.close()
        sock1.close()
    except dimq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)

do_test()
