#!/usr/bin/env python3

# Test whether a PUBLISH with a retain set when retains are disabled results in
# the correct DISCONNECT.

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("retain_available false\n")


def do_test(proto_ver):
    port = dimq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)

    rc = 1
    mid = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("pub-qos1-test", keepalive=keepalive, proto_ver=5)

    props = mqtt5_props.gen_byte_prop(mqtt5_props.PROP_RETAIN_AVAILABLE, 0)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5, properties=props)

    publish_packet = dimq_test.gen_publish("pub/qos1/test", qos=1, mid=mid, payload="message", retain=True, proto_ver=5)
    puback_packet = dimq_test.gen_puback(mid, proto_ver=5)

    disconnect_packet = dimq_test.gen_disconnect(reason_code=154, proto_ver=5)

    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        dimq_test.do_send_receive(sock, publish_packet, disconnect_packet, "disconnect")

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
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(proto_ver=4)
do_test(proto_ver=5)
exit(0)

