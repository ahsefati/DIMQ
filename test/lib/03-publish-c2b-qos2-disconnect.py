#!/usr/bin/env python3

# Test whether a client sends a correct PUBLISH to a topic with QoS 2 and responds to a disconnect.

from dimq_test_helper import *

port = dimq_test.get_lib_port()

rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("publish-qos2-test", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

disconnect_packet = dimq_test.gen_disconnect()

mid = 1
publish_packet = dimq_test.gen_publish("pub/qos2/test", qos=2, mid=mid, payload="message")
publish_dup_packet = dimq_test.gen_publish("pub/qos2/test", qos=2, mid=mid, payload="message", dup=True)
pubrec_packet = dimq_test.gen_pubrec(mid)
pubrel_packet = dimq_test.gen_pubrel(mid)
pubcomp_packet = dimq_test.gen_pubcomp(mid)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.settimeout(10)
sock.bind(('', port))
sock.listen(5)

client_args = sys.argv[1:]
env = dict(os.environ)
env['LD_LIBRARY_PATH'] = '../../lib:../../lib/cpp'
try:
    pp = env['PYTHONPATH']
except KeyError:
    pp = ''
env['PYTHONPATH'] = '../../lib/python:'+pp
client = dimq_test.start_client(filename=sys.argv[1].replace('/', '-'), cmd=client_args, env=env, port=port)

try:
    (conn, address) = sock.accept()
    conn.settimeout(10)

    dimq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")

    dimq_test.expect_packet(conn, "publish", publish_packet)
    # Disconnect client. It should reconnect.
    conn.close()

    (conn, address) = sock.accept()
    conn.settimeout(15)

    dimq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")
    dimq_test.do_receive_send(conn, publish_dup_packet, pubrec_packet, "retried publish")

    dimq_test.expect_packet(conn, "pubrel", pubrel_packet)
    # Disconnect client. It should reconnect.
    conn.close()

    (conn, address) = sock.accept()
    conn.settimeout(15)

    # Complete connection and message flow.
    dimq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")
    dimq_test.do_receive_send(conn, pubrel_packet, pubcomp_packet, "retried pubrel")

    dimq_test.expect_packet(conn, "disconnect", disconnect_packet)
    rc = 0

    conn.close()
except dimq_test.TestError:
    pass
finally:
    client.terminate()
    client.wait()
    sock.close()

exit(rc)
