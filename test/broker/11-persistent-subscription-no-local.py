#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.
# And whether the no-local option is persisted.

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
connect_packet = dimq_test.gen_connect(
    "persistent-subscription-test", keepalive=keepalive, clean_session=False, proto_ver=5, session_expiry=60
)
connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
connack_packet2 = dimq_test.gen_connack(rc=0, flags=1, proto_ver=5)  # session present

mid = 1
subscribe1_packet = dimq_test.gen_subscribe(mid, "subpub/nolocal", 5, proto_ver=5)
suback1_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

mid = 2
subscribe2_packet = dimq_test.gen_subscribe(mid, "subpub/local", 1, proto_ver=5)
suback2_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

mid = 1
publish1_packet = dimq_test.gen_publish("subpub/nolocal", qos=1, mid=mid, payload="message", proto_ver=5)
puback1_packet = dimq_test.gen_puback(mid, proto_ver=5)

mid = 2
publish2s_packet = dimq_test.gen_publish("subpub/local", qos=1, mid=mid, payload="message", proto_ver=5)
puback2s_packet = dimq_test.gen_puback(mid, proto_ver=5)

mid = 1
publish2a_packet = dimq_test.gen_publish("subpub/local", qos=1, mid=mid, payload="message", proto_ver=5)
puback2a_packet = dimq_test.gen_puback(mid, proto_ver=5)

mid = 2
publish2b_packet = dimq_test.gen_publish("subpub/local", qos=1, mid=mid, payload="message", proto_ver=5)
puback2b_packet = dimq_test.gen_puback(mid, proto_ver=5)

if os.path.exists('dimq-%d.db' % (port)):
    os.unlink('dimq-%d.db' % (port))

broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

(stdo1, stde1) = ("", "")
try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)
    dimq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
    dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

    dimq_test.do_send_receive(sock, publish1_packet, puback1_packet, "puback1a")
    sock.send(publish2s_packet)
    dimq_test.receive_unordered(sock, puback2s_packet, publish2a_packet, "puback2a/publish2a")

    sock.send(puback2a_packet)

    broker.terminate()
    broker.wait()
    (stdo1, stde1) = broker.communicate()
    sock.close()

    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    sock = dimq_test.do_client_connect(connect_packet, connack_packet2, timeout=20, port=port)

    dimq_test.do_send_receive(sock, publish1_packet, puback1_packet, "puback1b")
    sock.send(publish2s_packet)
    dimq_test.receive_unordered(sock, puback2s_packet, publish2b_packet, "puback2b/publish2b")

    rc = 0

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

