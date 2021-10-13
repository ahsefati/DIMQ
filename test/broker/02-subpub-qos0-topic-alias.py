#!/usr/bin/env python3

# Test whether "topic alias" works to the broker
# MQTT v5

from dimq_test_helper import *

def do_test():
    rc = 1
    keepalive = 60
    connect1_packet = dimq_test.gen_connect("sub-test", keepalive=keepalive, proto_ver=5)
    connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    connect2_packet = dimq_test.gen_connect("pub-test", keepalive=keepalive, proto_ver=5)
    connack2_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    mid = 1
    subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/alias", 0, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    props = mqtt5_props.gen_uint16_prop(mqtt5_props.PROP_TOPIC_ALIAS, 3)
    publish1_packet = dimq_test.gen_publish("subpub/alias", qos=0, payload="message", proto_ver=5, properties=props)

    props = mqtt5_props.gen_uint16_prop(mqtt5_props.PROP_TOPIC_ALIAS, 3)
    publish2s_packet = dimq_test.gen_publish("", qos=0, payload="message", proto_ver=5, properties=props)
    publish2r_packet = dimq_test.gen_publish("subpub/alias", qos=0, payload="message", proto_ver=5)


    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock1 = dimq_test.do_client_connect(connect1_packet, connack1_packet, timeout=5, port=port)
        sock2 = dimq_test.do_client_connect(connect2_packet, connack2_packet, timeout=5, port=port)

        sock1.send(publish1_packet)

        dimq_test.do_send_receive(sock2, subscribe_packet, suback_packet, "suback")

        sock1.send(publish2s_packet)

        dimq_test.expect_packet(sock2, "publish2r", publish2r_packet)
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


do_test()
exit()
