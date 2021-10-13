#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

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
mid = 530
keepalive = 60

connect_packet = dimq_test.gen_connect(
    "persistent-subscription-test", keepalive=keepalive, clean_session=False, proto_ver=5, session_expiry=60
)
connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)
connack_packet2 = dimq_test.gen_connack(rc=0, flags=1, proto_ver=5)  # session present

subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos1", 1, proto_ver=5)
suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

mid = 300
publish_packet = dimq_test.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=5)
puback_packet = dimq_test.gen_puback(mid, proto_ver=5)

mid = 1
publish_packet2 = dimq_test.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=5)

if os.path.exists('dimq-%d.db' % (port)):
    os.unlink('dimq-%d.db' % (port))

broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

(stdo1, stde1) = ("", "")
try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    broker.terminate()
    broker.wait()
    (stdo1, stde1) = broker.communicate()
    sock.close()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    sock = dimq_test.do_client_connect(connect_packet, connack_packet2, timeout=20, port=port)

    sock.send(publish_packet)
    dimq_test.receive_unordered(sock, puback_packet, publish_packet2, "puback/publish2")
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

