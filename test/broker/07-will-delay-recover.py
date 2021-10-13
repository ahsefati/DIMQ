#!/usr/bin/env python3

# Test whether a client with a will delay recovers on the client reconnecting
# MQTT 5

from dimq_test_helper import *


def do_test(clean_session):
    rc = 1
    keepalive = 60

    mid = 1
    connect1_packet = dimq_test.gen_connect("will-qos0-test", keepalive=keepalive, proto_ver=5)
    connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    connect_props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_SESSION_EXPIRY_INTERVAL, 30)
    props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_WILL_DELAY_INTERVAL, 3)
    connect2_packet = dimq_test.gen_connect("will-helper", keepalive=keepalive, proto_ver=5, will_topic="will/test", will_payload=b"will delay", will_properties=props, clean_session=clean_session, properties=connect_props)
    connack2a_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
    if clean_session == True:
        connack2b_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
    else:
        connack2b_packet = dimq_test.gen_connack(rc=0, proto_ver=5, flags=1)

    subscribe_packet = dimq_test.gen_subscribe(mid, "will/test", 0, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock1 = dimq_test.do_client_connect(connect1_packet, connack1_packet, timeout=30, port=port)
        dimq_test.do_send_receive(sock1, subscribe_packet, suback_packet, "suback")

        sock2 = dimq_test.do_client_connect(connect2_packet, connack2a_packet, timeout=30, port=port)
        sock2.close()

        time.sleep(1)
        sock2 = dimq_test.do_client_connect(connect2_packet, connack2b_packet, timeout=30, port=port)
        time.sleep(3)

        # The client2 has reconnected within the will delay interval, which has now
        # passed. We should not have received the will at this point.
        dimq_test.do_ping(sock1)
        rc = 0

        sock1.close()
        sock2.close()
    except dimq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test(clean_session=True)
do_test(clean_session=False)
