#!/usr/bin/env python3

# Test whether max_keepalive violations are rejected for MQTT < 5.0.

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("max_keepalive 100\n")

def do_test(proto_ver):
    rc = 1

    connect_packet = dimq_test.gen_connect("max-keepalive", keepalive=101, proto_ver=proto_ver)
    connack_packet = dimq_test.gen_connack(rc=2, proto_ver=proto_ver)

    port = dimq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    socks = []
    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        sock.close()
        rc = 0
    except dimq_test.TestError:
        pass
    except Exception as err:
        print(err)
    finally:
        os.remove(conf_file)
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)

do_test(3)
do_test(4)
exit(0)
