#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

from dimq_test_helper import *

def do_test(proto_ver):
    rc = 1
    mid = 530
    keepalive = 60
    connect_packet = dimq_test.gen_connect("subpub-qos2-test", keepalive=keepalive, proto_ver=proto_ver)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos2", 2, proto_ver=proto_ver)
    suback_packet = dimq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 301
    publish_packet = dimq_test.gen_publish("subpub/qos2", qos=2, mid=mid, payload="message", proto_ver=proto_ver)
    pubrec_packet = dimq_test.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet = dimq_test.gen_pubcomp(mid, proto_ver=proto_ver)

    mid = 1
    publish_packet2 = dimq_test.gen_publish("subpub/qos2", qos=2, mid=mid, payload="message", proto_ver=proto_ver)
    pubrec_packet2 = dimq_test.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet2 = dimq_test.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet2 = dimq_test.gen_pubcomp(mid, proto_ver=proto_ver)


    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        dimq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec")
        sock.send(pubrel_packet)

        dimq_test.receive_unordered(sock, pubcomp_packet, publish_packet2, "pubcomp/publish2")

        dimq_test.do_send_receive(sock, pubrec_packet2, pubrel_packet2, "pubrel2")
        sock.send(pubcomp_packet2)
        # Broker side of flow complete so can quit here.
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
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(proto_ver=4)
do_test(proto_ver=5)
exit(0)
