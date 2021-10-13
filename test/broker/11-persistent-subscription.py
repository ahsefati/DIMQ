#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("port %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("persistence true\n")
        f.write("persistence_file dimq-%d.db\n" % (port))

def do_test(proto_ver):
    port = dimq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)

    rc = 1
    mid = 530
    keepalive = 60
    connect_packet = dimq_test.gen_connect(
        "persistent-subscription-test", keepalive=keepalive, clean_session=False, proto_ver=proto_ver, session_expiry=60
    )
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)
    connack_packet2 = dimq_test.gen_connack(rc=0, flags=1, proto_ver=proto_ver)  # session present

    subscribe_packet = dimq_test.gen_subscribe(mid, "subpub/qos1", 1, proto_ver=proto_ver)
    suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=proto_ver)

    mid = 300
    publish_packet = dimq_test.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=proto_ver)
    puback_packet = dimq_test.gen_puback(mid, proto_ver=proto_ver)

    mid = 1
    publish_packet2 = dimq_test.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=proto_ver)

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
        if os.path.exists('dimq-%d.db' % (port)):
            os.unlink('dimq-%d.db' % (port))
        if rc:
            print(stde.decode('utf-8'))
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(proto_ver=4)
do_test(proto_ver=5)
exit(0)

