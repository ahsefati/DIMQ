#!/usr/bin/env python3

from dimq_test_helper import *

port = dimq_test.get_lib_port()

rc = 1
keepalive = 5
connect_packet = dimq_test.gen_connect("publish-qos2-test", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

disconnect_packet = dimq_test.gen_disconnect()

mid = 13423
pubcomp_packet = dimq_test.gen_pubcomp(mid)
pingreq_packet = dimq_test.gen_pingreq()

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

    if dimq_test.expect_packet(conn, "connect", connect_packet):
        conn.send(connack_packet)
        conn.send(pubcomp_packet)

        if dimq_test.expect_packet(conn, "pingreq", pingreq_packet):
            rc = 0

    conn.close()
finally:
    for i in range(0, 5):
        if client.returncode != None:
            break
        time.sleep(0.1)

    try:
        client.terminate()
    except OSError:
        pass

    client.wait()
    sock.close()
    if rc != 0 or client.returncode != 0:
        exit(1)

exit(rc)
