#!/usr/bin/env python3

# Test whether a client subscribed to a topic does not receive its own message
# sent to that topic if no local is set.
# MQTT v5

from dimq_test_helper import *

def do_test():
    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("subpub-qos1-test", keepalive=keepalive, proto_ver=5)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    mid = 530
    subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos1", 1 | mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

    mid = 531
    subscribe2_packet = dimq_test.gen_subscribe(mid, "subpub/receive", 1, proto_ver=5)
    suback2_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

    mid = 300
    publish_packet = dimq_test.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=5)
    puback_packet = dimq_test.gen_puback(mid, proto_ver=5)

    mid = 301
    publish2_packet = dimq_test.gen_publish("subpub/receive", qos=1, mid=mid, payload="success", proto_ver=5)
    puback2_packet = dimq_test.gen_puback(mid, proto_ver=5)

    mid = 1
    publish3_packet = dimq_test.gen_publish("subpub/receive", qos=1, mid=mid, payload="success", proto_ver=5)


    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

        dimq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")
        sock.send(publish2_packet)

        dimq_test.receive_unordered(sock, puback2_packet, publish3_packet, "puback2/publish3")
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


do_test()
exit(0)
