#!/usr/bin/env python3

# Test topic subscription. All SUBSCRIBE requests are denied. Check this
# produces the correct response, and check the client isn't disconnected (ref:
# issue #1016).

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("port %d\n" % (port))
        f.write("auth_plugin c/auth_plugin_acl_sub_denied.so\n")
        f.write("allow_anonymous false\n")

port = dimq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

rc = 1
keepalive = 10
connect_packet = dimq_test.gen_connect("sub-denied-test", keepalive=keepalive, username="denied")
connack_packet = dimq_test.gen_connack(rc=0)

mid = 53
subscribe_packet = dimq_test.gen_subscribe(mid, "qos0/test", 0)
suback_packet = dimq_test.gen_suback(mid, 128)

mid_pub = 54
publish_packet = dimq_test.gen_publish("topic", qos=1, payload="test", mid=mid_pub)
puback_packet = dimq_test.gen_puback(mid_pub)

broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    dimq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")

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


exit(rc)
