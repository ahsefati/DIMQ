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
subscribe_packet = dimq_test.gen_subscribe(mid, "test/topic", 1, proto_ver=5)
suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

mid=1
publish1_packet = dimq_test.gen_publish(topic="test/topic", mid=mid, qos=1, payload="12345678901234567890", proto_ver=5)
puback1_packet = dimq_test.gen_puback(mid, proto_ver=5)

mid=2
publish2_packet = dimq_test.gen_publish(topic="test/topic", mid=mid, qos=1, payload="7890", proto_ver=5)
puback2_packet = dimq_test.gen_puback(mid, proto_ver=5)

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet, suback_packet)

    dimq_test.do_send_receive(sock, publish1_packet, puback1_packet, "puback 1")

    # We shouldn't receive the publish here because it is > MAXIMUM_PACKET_SIZE
    dimq_test.do_ping(sock)

    sock.send(publish2_packet)
    dimq_test.receive_unordered(sock, puback2_packet, publish2_packet, "puback 2/publish2")
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

