#!/usr/bin/env python3

# Test whether a SUBSCRIBE to $SYS or $share succeeds

from dimq_test_helper import *

def do_test(proto_ver):
    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("subscribe-test", keepalive=keepalive, proto_ver=proto_ver)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe1_packet = dimq_test.gen_subscribe(mid, "$SYS/broker/missing", 0, proto_ver=proto_ver)
    suback1_packet = dimq_test.gen_suback(mid, 0, proto_ver=proto_ver)

    mid = 2
    subscribe2_packet = dimq_test.gen_subscribe(mid, "$share/share/#", 0, proto_ver=proto_ver)
    suback2_packet = dimq_test.gen_suback(mid, 0, proto_ver=proto_ver)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        dimq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
        dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

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

do_test(4)
do_test(5)
exit(0)
