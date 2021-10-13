#!/usr/bin/env python3

# Test switching between persistence and a clean session.
#
# Bug #874:
#
#
# dimq_sub -i sub -t 'topic' -v -p 29883 -q 1 -d -c
# ^C
# dimq_sub -i sub -t 'topic' -v -p 29883 -q 1 -d
# ^C
# 
# SUBSCRIBE to topic is no longer respected by dimq
# 
# run:
# 
# dimq_sub -i sub -t 'topic' -v -p 29883 -q 1 -d -c
# 
# and in a separate shell
# 
# dimq_pub -i pub -t topic -m 'hello' -p 29883 -q 1
# 
# sub does not receive the message

from dimq_test_helper import *

def do_test(proto_ver):
    rc = 1
    keepalive = 60
    connect_packet_sub_persistent = dimq_test.gen_connect("flipflop-test", keepalive=keepalive, clean_session=False, proto_ver=proto_ver)
    connect_packet_sub_clean = dimq_test.gen_connect("flipflop-test", keepalive=keepalive, clean_session=True, proto_ver=proto_ver)
    connack_packet_sub = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    connect_packet_pub = dimq_test.gen_connect("flipflop-test-pub", keepalive=keepalive, proto_ver=proto_ver)
    connack_packet_pub = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid=1
    subscribe_packet = dimq_test.gen_subscribe(mid, "flipflop/test", 1, proto_ver=proto_ver)
    suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=proto_ver)

    mid=1
    publish_packet = dimq_test.gen_publish("flipflop/test", qos=1, mid=mid, payload="message", proto_ver=proto_ver)
    puback_packet = dimq_test.gen_puback(mid, proto_ver=proto_ver)


    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        # dimq_sub -i sub -t 'topic' -q 1 -d -c
        sub_sock = dimq_test.do_client_connect(connect_packet_sub_persistent, connack_packet_sub, port=port)
        dimq_test.do_send_receive(sub_sock, subscribe_packet, suback_packet, "subscribe persistent 1")
        # And disconnect
        sub_sock.close()

        # dimq_sub -i sub -t 'topic' -q 1 -d
        sub_sock = dimq_test.do_client_connect(connect_packet_sub_clean, connack_packet_sub, port=port)
        dimq_test.do_send_receive(sub_sock, subscribe_packet, suback_packet, "subscribe clean")
        # And disconnect
        sub_sock.close()

        # dimq_sub -i sub -t 'topic' -v -q 1 -d -c
        sub_sock = dimq_test.do_client_connect(connect_packet_sub_persistent, connack_packet_sub, port=port)
        dimq_test.do_send_receive(sub_sock, subscribe_packet, suback_packet, "subscribe persistent 2")

        # and in a separate shell
        # 
        # dimq_pub -i pub -t topic -m 'hello' -p 29883 -q 1
        pub_sock = dimq_test.do_client_connect(connect_packet_pub, connack_packet_pub, port=port)
        dimq_test.do_send_receive(pub_sock, publish_packet, puback_packet, "publish")

        dimq_test.expect_packet(sub_sock, "publish receive", publish_packet)
        rc = 0

        sub_sock.close()
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
