#!/usr/bin/env python3

# Does an interrupted QoS 1 flow from broker to client get handled correctly?

from dimq_test_helper import *


def helper(port):
    connect_packet = dimq_test.gen_connect("test-helper", keepalive=60)
    connack_packet = dimq_test.gen_connack(rc=0)

    mid = 312
    publish_packet = dimq_test.gen_publish("qos2/disconnect/test", qos=2, mid=mid, payload="disconnect-message")
    pubrec_packet = dimq_test.gen_pubrec(mid)
    pubrel_packet = dimq_test.gen_pubrel(mid)
    pubcomp_packet = dimq_test.gen_pubcomp(mid)

    sock = dimq_test.do_client_connect(connect_packet, connack_packet, connack_error="helper connack", port=port)

    dimq_test.do_send_receive(sock, publish_packet, pubrec_packet, "helper pubrec")
    dimq_test.do_send_receive(sock, pubrel_packet, pubcomp_packet, "helper pubcomp")

    sock.close()


def do_test(proto_ver):
    rc = 1
    mid = 3265
    keepalive = 60
    connect_packet = dimq_test.gen_connect("pub-qos2-disco-test", keepalive=keepalive, clean_session=False, proto_ver=proto_ver, session_expiry=60)
    connack1_packet = dimq_test.gen_connack(flags=0, rc=0, proto_ver=proto_ver)
    connack2_packet = dimq_test.gen_connack(flags=1, rc=0, proto_ver=proto_ver)

    subscribe_packet = dimq_test.gen_subscribe(mid, "qos2/disconnect/test", 2, proto_ver=proto_ver)
    suback_packet = dimq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 1
    publish_packet = dimq_test.gen_publish("qos2/disconnect/test", qos=2, mid=mid, payload="disconnect-message", proto_ver=proto_ver)
    publish_dup_packet = dimq_test.gen_publish("qos2/disconnect/test", qos=2, mid=mid, payload="disconnect-message", dup=True, proto_ver=proto_ver)
    pubrec_packet = dimq_test.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet = dimq_test.gen_pubcomp(mid, proto_ver=proto_ver)

    mid = 3266
    publish2_packet = dimq_test.gen_publish("qos1/outgoing", qos=1, mid=mid, payload="outgoing-message", proto_ver=proto_ver)
    puback2_packet = dimq_test.gen_puback(mid, proto_ver=proto_ver)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack1_packet, port=port)

        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        helper(port)
        # Should have now received a publish command

        dimq_test.expect_packet(sock, "publish", publish_packet)
        # Send our outgoing message. When we disconnect the broker
        # should get rid of it and assume we're going to retry.
        sock.send(publish2_packet)
        sock.close()

        sock = dimq_test.do_client_connect(connect_packet, connack2_packet, port=port)
        dimq_test.expect_packet(sock, "dup publish", publish_dup_packet)
        dimq_test.do_send_receive(sock, pubrec_packet, pubrel_packet, "pubrel")

        sock.close()

        sock = dimq_test.do_client_connect(connect_packet, connack2_packet, port=port)
        dimq_test.expect_packet(sock, "dup pubrel", pubrel_packet)
        sock.send(pubcomp_packet)
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

