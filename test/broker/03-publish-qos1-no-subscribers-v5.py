#!/usr/bin/env python3

# Test whether a PUBLISH to a topic with QoS 1 results in the correct PUBACK
# packet when there are no subscribers.

from dimq_test_helper import *

rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("pub-qos1-test", keepalive=keepalive, proto_ver=5)
connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)


mid = 1
publish1_packet = dimq_test.gen_publish("pub", qos=1, mid=mid, payload="message", proto_ver=5)
puback1_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 2
publish2_packet = dimq_test.gen_publish("pub/qos1", qos=1, mid=mid, payload="message", proto_ver=5)
puback2_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 3
publish3_packet = dimq_test.gen_publish("pub/qos1/test", qos=1, mid=mid, payload="message", proto_ver=5)
puback3_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 4
publish4_packet = dimq_test.gen_publish("pub/qos1/test", qos=1, mid=mid, payload="message", proto_ver=5, retain=True)
puback4_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 5
publish1b_packet = dimq_test.gen_publish("pub", qos=1, mid=mid, payload="message", proto_ver=5)
puback1b_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 6
publish2b_packet = dimq_test.gen_publish("pub/qos1", qos=1, mid=mid, payload="message", proto_ver=5)
puback2b_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

mid = 7
publish3b_packet = dimq_test.gen_publish("pub/qos1/test", qos=1, mid=mid, payload="message", proto_ver=5)
puback3b_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)

    # None of the pub/qos1/test topic tree exists here
    dimq_test.do_send_receive(sock, publish1_packet, puback1_packet, "puback1a")
    dimq_test.do_send_receive(sock, publish2_packet, puback2_packet, "puback2a")
    dimq_test.do_send_receive(sock, publish3_packet, puback3_packet, "puback3a")

    # This publish sets a retained message, which means the topic tree exists
    dimq_test.do_send_receive(sock, publish4_packet, puback4_packet, "puback4")

    # So now test again
    dimq_test.do_send_receive(sock, publish1b_packet, puback1b_packet, "puback1b")
    dimq_test.do_send_receive(sock, publish2b_packet, puback2b_packet, "puback2b")
    dimq_test.do_send_receive(sock, publish3b_packet, puback3b_packet, "puback3b")

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

