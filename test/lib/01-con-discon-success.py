#!/usr/bin/env python3

# Test whether a client produces a correct connect and subsequent disconnect.

# The client should connect to port 1888 with keepalive=60, clean session set,
# and client id 01-con-discon-success
# The test will send a CONNACK message to the client with rc=0. Upon receiving
# the CONNACK and verifying that rc=0, the client should send a DISCONNECT
# message. If rc!=0, the client should exit with an error.

from dimq_test_helper import *

port = dimq_test.get_lib_port()

rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("01-con-discon-success", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

disconnect_packet = dimq_test.gen_disconnect()

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
    dimq_test.expect_packet(conn, "disconnect", disconnect_packet)
    rc = 0

    conn.close()
except dimq_test.TestError:
    pass
finally:
    #client.terminate()
    client.wait()
    sock.close()

exit(rc)

