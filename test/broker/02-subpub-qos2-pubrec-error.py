#!/usr/bin/env python3

# Test whether a PUBREC with reason code >= 0x80 is handled correctly

from dimq_test_helper import *

def helper(port):
    connect_packet = dimq_test.gen_connect("test-helper", keepalive=60)
    connack_packet = dimq_test.gen_connack(rc=0)

    mid = 1
    publish_1_packet = dimq_test.gen_publish("qos2/pubrec/rejected", qos=2, mid=mid, payload="rejected-message")
    pubrec_1_packet = dimq_test.gen_pubrec(mid)
    pubrel_1_packet = dimq_test.gen_pubrel(mid)
    pubcomp_1_packet = dimq_test.gen_pubcomp(mid)

    mid = 2
    publish_2_packet = dimq_test.gen_publish("qos2/pubrec/accepted", qos=2, mid=mid, payload="accepted-message")
    pubrec_2_packet = dimq_test.gen_pubrec(mid)
    pubrel_2_packet = dimq_test.gen_pubrel(mid)
    pubcomp_2_packet = dimq_test.gen_pubcomp(mid)

    sock = dimq_test.do_client_connect(connect_packet, connack_packet, connack_error="helper connack", port=port)

    dimq_test.do_send_receive(sock, publish_1_packet, pubrec_1_packet, "helper pubrec")
    dimq_test.do_send_receive(sock, pubrel_1_packet, pubcomp_1_packet, "helper pubcomp")

    dimq_test.do_send_receive(sock, publish_2_packet, pubrec_2_packet, "helper pubrec")
    dimq_test.do_send_receive(sock, pubrel_2_packet, pubcomp_2_packet, "helper pubcomp")
    sock.close()


def do_test():
    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("pub-qo2-timeout-test", keepalive=keepalive, proto_ver=5)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    mid = 1
    subscribe_packet = dimq_test.gen_subscribe(mid, "qos2/pubrec/+", 2, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 2, proto_ver=5)

    mid = 1
    publish_1_packet = dimq_test.gen_publish("qos2/pubrec/rejected", qos=2, mid=mid, payload="rejected-message", proto_ver=5)
    pubrec_1_packet = dimq_test.gen_pubrec(mid, proto_ver=5, reason_code=0x80)

    mid = 2
    publish_2_packet = dimq_test.gen_publish("qos2/pubrec/accepted", qos=2, mid=mid, payload="accepted-message", proto_ver=5)
    pubrec_2_packet = dimq_test.gen_pubrec(mid, proto_ver=5)
    pubrel_2_packet = dimq_test.gen_pubrel(mid, proto_ver=5)
    pubcomp_2_packet = dimq_test.gen_pubcomp(mid, proto_ver=5)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        helper(port)

        # Should have now received a publish command
        dimq_test.expect_packet(sock, "publish 1", publish_1_packet)
        sock.send(pubrec_1_packet)

        dimq_test.expect_packet(sock, "publish 2", publish_2_packet)
        dimq_test.do_send_receive(sock, pubrec_2_packet, pubrel_2_packet, "pubrel 2")
        sock.send(pubcomp_2_packet)
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


do_test()
exit(0)
