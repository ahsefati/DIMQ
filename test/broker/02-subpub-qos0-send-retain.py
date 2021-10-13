#!/usr/bin/env python3

# Test whether "send retain" subscribe options work
# MQTT v5

from dimq_test_helper import *

def do_test():
    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("subpub-test", keepalive=keepalive, proto_ver=5)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    mid = 530
    subscribe1_packet = dimq_test.gen_subscribe(mid, "subpub/always", 0 | mqtt5_opts.MQTT_SUB_OPT_SEND_RETAIN_ALWAYS, proto_ver=5)
    suback1_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    mid = 531
    subscribe2_packet = dimq_test.gen_subscribe(mid, "subpub/new", 0 | mqtt5_opts.MQTT_SUB_OPT_SEND_RETAIN_NEW, proto_ver=5)
    suback2_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)

    mid = 532
    subscribe3_packet = dimq_test.gen_subscribe(mid, "subpub/never", 0 | mqtt5_opts.MQTT_SUB_OPT_SEND_RETAIN_NEVER, proto_ver=5)
    suback3_packet = dimq_test.gen_suback(mid, 0, proto_ver=5)


    publish1_packet = dimq_test.gen_publish("subpub/always", qos=0, retain=True, payload="message", proto_ver=5)
    publish2_packet = dimq_test.gen_publish("subpub/new", qos=0, retain=True, payload="message", proto_ver=5)
    publish3_packet = dimq_test.gen_publish("subpub/never", qos=0, retain=True, payload="message", proto_ver=5)

    publish1r1_packet = dimq_test.gen_publish("subpub/always", qos=0, retain=True, payload="message", proto_ver=5)
    publish1r2_packet = dimq_test.gen_publish("subpub/always", qos=0, retain=True, payload="message", proto_ver=5)
    publish2r1_packet = dimq_test.gen_publish("subpub/new", qos=0, retain=True, payload="message", proto_ver=5)
    publish2r2_packet = dimq_test.gen_publish("subpub/new", qos=0, retain=False, payload="message", proto_ver=5)
    publish3r1_packet = dimq_test.gen_publish("subpub/never", qos=0, retain=False, payload="message", proto_ver=5)
    publish3r2_packet = dimq_test.gen_publish("subpub/never", qos=0, retain=False, payload="message", proto_ver=5)


    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)

        sock.send(publish1_packet)
        sock.send(publish2_packet)
        sock.send(publish3_packet)

        # Don't expect a message after this
        dimq_test.do_send_receive(sock, subscribe3_packet, suback3_packet, "suback3")
        # Don't expect a message after this
        dimq_test.do_send_receive(sock, subscribe3_packet, suback3_packet, "suback3")

        # Expect a message after this, because it is the first subscribe
        dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")
        dimq_test.expect_packet(sock, "publish2r1", publish2r1_packet)
        # Don't expect a message after this, it is the second subscribe
        dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

        # Always expect a message after this
        dimq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
        dimq_test.expect_packet(sock, "publish1r1", publish1r1_packet)
        # Always expect a message after this
        dimq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
        dimq_test.expect_packet(sock, "publish1r1", publish1r2_packet)
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
