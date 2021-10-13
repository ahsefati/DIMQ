#!/usr/bin/env python3

# Test whether maximum packet size is honoured on a PUBLISH to a client
# MQTTv5

from dimq_test_helper import *

rc = 1

keepalive = 10
props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_MAXIMUM_PACKET_SIZE, 20)
connect_packet = dimq_test.gen_connect("test", proto_ver=5, keepalive=keepalive, properties=props)
connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

mid = 1
subscribe_packet = dimq_test.gen_subscribe(mid, "test/topic", 2, proto_ver=5)
suback_packet = dimq_test.gen_suback(mid, 2, proto_ver=5)

mid=1
publish1_packet = dimq_test.gen_publish(topic="test/topic", mid=mid, qos=2, payload="12345678901234567890", proto_ver=5)
pubrec1_packet = dimq_test.gen_pubrec(mid, proto_ver=5)
pubrel1_packet = dimq_test.gen_pubrel(mid, proto_ver=5)
pubcomp1_packet = dimq_test.gen_pubcomp(mid, proto_ver=5)

mid=2
publish2_packet = dimq_test.gen_publish(topic="test/topic", mid=mid, qos=2, payload="7890", proto_ver=5)
pubrec2_packet = dimq_test.gen_pubrec(mid, proto_ver=5)
pubrel2_packet = dimq_test.gen_pubrel(mid, proto_ver=5)
pubcomp2_packet = dimq_test.gen_pubcomp(mid, proto_ver=5)

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet, suback_packet)

    dimq_test.do_send_receive(sock, publish1_packet, pubrec1_packet, "pubrec 1")
    dimq_test.do_send_receive(sock, pubrel1_packet, pubcomp1_packet, "pubcomp 1")

    # We shouldn't receive the publish here because it is > MAXIMUM_PACKET_SIZE
    dimq_test.do_ping(sock)

    dimq_test.do_send_receive(sock, publish2_packet, pubrec2_packet, "pubrec 2")
    sock.send(pubrel2_packet)
    dimq_test.receive_unordered(sock, pubcomp2_packet, publish2_packet, "pubcomp 2/publish2")
    rc = 0
except dimq_test.TestError:
    pass
finally:
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))

exit(rc)

