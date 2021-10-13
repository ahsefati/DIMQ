#!/usr/bin/env python3

# Test whether a client will is transmitted with a delay correctly.
# MQTT 5

from dimq_test_helper import *

def do_test(clean_session):
    rc = 1
    keepalive = 60

    mid = 1
    connect1_packet = dimq_test.gen_connect("will-qos0-test", keepalive=keepalive, proto_ver=5)
    connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_WILL_DELAY_INTERVAL, 3)
    connect2_packet = dimq_test.gen_connect("will-helper", keepalive=keepalive, proto_ver=5, will_topic="will/test", will_payload=b"will delay", will_qos=2, will_properties=props, clean_session=clean_session)
    connack2_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    subscribe_packet = dimq_test.gen_subscribe(mid, "will/test", 0, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    publish_packet = dimq_test.gen_publish("will/test", qos=0, payload="will delay", proto_ver=5)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock1 = dimq_test.do_client_connect(connect1_packet, connack1_packet, timeout=30, port=port)
        dimq_test.do_send_receive(sock1, subscribe_packet, suback_packet, "suback")

        sock2 = dimq_test.do_client_connect(connect2_packet, connack2_packet, timeout=30, port=port)
        sock2.close()

        t_start = time.time()
        dimq_test.expect_packet(sock1, "publish", publish_packet)
        t_finish = time.time()
        if t_finish - t_start > 2 and t_finish - t_start < 5:
            rc = 0

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

do_test(clean_session=True)
do_test(clean_session=False)
