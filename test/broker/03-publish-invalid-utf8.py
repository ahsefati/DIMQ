#!/usr/bin/env python3

# Test whether a PUBLISH to a topic with an invalid UTF-8 topic fails

from dimq_test_helper import *

def do_test(proto_ver):
    rc = 1
    mid = 53
    keepalive = 60
    connect_packet = dimq_test.gen_connect("publish-invalid-utf8", keepalive=keepalive, proto_ver=proto_ver)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    publish_packet = dimq_test.gen_publish("invalid/utf8", 1, mid=mid, proto_ver=proto_ver)
    b = list(struct.unpack("B"*len(publish_packet), publish_packet))
    b[11] = 0 # Topic should never have a 0x0000
    publish_packet = struct.pack("B"*len(b), *b)

    puback_packet = dimq_test.gen_puback(mid, proto_ver=proto_ver)

    port = dimq_test.get_port()
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        time.sleep(0.5)

        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        if proto_ver == 4:
            dimq_test.do_send_receive(sock, publish_packet, b"", "puback")
        else:
            disconnect_packet = dimq_test.gen_disconnect(proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_MALFORMED_PACKET)
            dimq_test.do_send_receive(sock, publish_packet, disconnect_packet, "puback")

        rc = 0

        sock.close()
    except dimq_test.TestError:
        pass
    finally:
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

