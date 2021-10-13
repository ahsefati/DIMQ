#!/usr/bin/env python3

# Test whether the broker reduces the message expiry interval when republishing.
# MQTT v5, with a broker restart and persistence.

# Client connects with clean session set false, subscribes with qos=1, then disconnects
# Helper publishes two messages, one with a short expiry and one with a long expiry
# We wait until the short expiry will have expired but the long one not.
# Client reconnects, expects delivery of the long expiry message with a reduced
# expiry interval property.

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("port %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("persistence true\n")
        f.write("persistence_file dimq-%d.db\n" % (port))

port = dimq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)


rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("subpub-qos0-test", keepalive=keepalive, proto_ver=5, clean_session=False, session_expiry=60)
connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
connack2_packet = dimq_test.gen_connack(rc=0, proto_ver=5, flags=1)

mid = 53
subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos1", 1, proto_ver=5)
suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)



helper_connect = dimq_test.gen_connect("helper", proto_ver=5)
helper_connack = dimq_test.gen_connack(rc=0, proto_ver=5)

mid=1
props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_MESSAGE_EXPIRY_INTERVAL, 5)
publish1s_packet = dimq_test.gen_publish("subpub/qos1", mid=mid, qos=1, payload="message1", proto_ver=5, properties=props)
puback1s_packet = dimq_test.gen_puback(mid)

mid=2
props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_MESSAGE_EXPIRY_INTERVAL, 100)
publish2s_packet = dimq_test.gen_publish("subpub/qos1", mid=mid, qos=1, payload="message2", proto_ver=5, properties=props)
puback2s_packet = dimq_test.gen_puback(mid)

mid=3
publish3_packet = dimq_test.gen_publish("subpub/qos1", mid=mid, qos=1, payload="message3", proto_ver=5)
puback3_packet = dimq_test.gen_puback(mid)


if os.path.exists('dimq-%d.db' % (port)):
    os.unlink('dimq-%d.db' % (port))

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack1_packet, timeout=20, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
    sock.close()

    helper = dimq_test.do_client_connect(helper_connect, helper_connack, timeout=20, port=port)
    dimq_test.do_send_receive(helper, publish1s_packet, puback1s_packet, "puback 1")
    dimq_test.do_send_receive(helper, publish2s_packet, puback2s_packet, "puback 2")
    dimq_test.do_send_receive(helper, publish3_packet, puback3_packet, "puback 3")

    broker.terminate()
    broker.wait()
    (stdo1, stde1) = broker.communicate()
    sock.close()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    time.sleep(7)
    
    sock = dimq_test.do_client_connect(connect_packet, connack2_packet, timeout=20, port=port)
    packet = sock.recv(len(publish2s_packet))
    for i in range(100, 1, -1):
        props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_MESSAGE_EXPIRY_INTERVAL, i)
        publish2r_packet = dimq_test.gen_publish("subpub/qos1", mid=2, qos=1, payload="message2", proto_ver=5, properties=props)
        if packet == publish2r_packet:
            dimq_test.expect_packet(sock, "publish3", publish3_packet)
            rc = 0
            break

    sock.close()
except dimq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))
    if os.path.exists('dimq-%d.db' % (port)):
        os.unlink('dimq-%d.db' % (port))

exit(rc)

