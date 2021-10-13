#!/usr/bin/env python3

# Check whether a v5 client handles a v5 PUBREL with all combinations
# of with/without reason code and properties.

from dimq_test_helper import *

mid = 56

def len_test(test, pubrel_packet):
    port = dimq_test.get_lib_port()

    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("publish-qos2-test", keepalive=keepalive, proto_ver=5)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    disconnect_packet = dimq_test.gen_disconnect(proto_ver=5)

    publish_packet = dimq_test.gen_publish("len/qos2/test", qos=2, mid=mid, payload="message", proto_ver=5)
    pubrec_packet = dimq_test.gen_pubrec(mid, proto_ver=5)
    pubcomp_packet = dimq_test.gen_pubcomp(mid, proto_ver=5)

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
        conn.settimeout(15)

        dimq_test.expect_packet(conn, "connect", connect_packet)
        conn.send(connack_packet)

        dimq_test.do_send_receive(conn, publish_packet, pubrec_packet, "pubrec")
        dimq_test.do_send_receive(conn, pubrel_packet, pubcomp_packet, "pubcomp")
        dimq_test.expect_packet(conn, "disconnect", disconnect_packet)
        rc = 0

        conn.close()
    except dimq_test.TestError:
        pass
    finally:
        client.terminate()
        client.wait()
        sock.close()

    if rc != 0:
        print(test)
        exit(rc)


# No reason code, no properties
pubrel_packet = dimq_test.gen_pubrel(mid)
len_test("qos2 len 2", pubrel_packet)

# Reason code, no properties
pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=5, reason_code=0x00)
len_test("qos2 len 3", pubrel_packet)

# Reason code, empty properties
pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=5, reason_code=0x00, properties="")
len_test("qos2 len 4", pubrel_packet)

# Reason code, one property
props = mqtt5_props.gen_string_pair_prop(mqtt5_props.PROP_USER_PROPERTY, "key", "value")
pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=5, reason_code=0x00, properties=props)
len_test("qos2 len >5", pubrel_packet)
