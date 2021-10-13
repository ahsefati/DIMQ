#!/usr/bin/env python3

# Does a bridge resend a QoS=1 message correctly after a disconnect?

from dimq_test_helper import *

def write_config(filename, port1, port2, protocol_version):
    with open(filename, 'w') as f:
        f.write("port %d\n" % (port2))
        f.write("allow_anonymous true\n")
        f.write("\n")
        f.write("connection bridge_sample\n")
        f.write("address 127.0.0.1:%d\n" % (port1))
        f.write("topic bridge/# both 2\n")
        f.write("notifications false\n")
        f.write("restart_timeout 5\n")
        f.write("bridge_protocol_version %s\n" % (protocol_version))


def do_test(proto_ver):
    if proto_ver == 4:
        bridge_protocol = "mqttv311"
        proto_ver_connect = 128+4
    else:
        bridge_protocol = "mqttv50"
        proto_ver_connect = 5

    (port1, port2) = dimq_test.get_port(2)
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port1, port2, bridge_protocol)

    rc = 1
    keepalive = 60
    client_id = socket.gethostname()+".bridge_sample"
    connect_packet = dimq_test.gen_connect(client_id, keepalive=keepalive, clean_session=False, proto_ver=proto_ver_connect)
    connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

    if proto_ver == 5:
        opts = mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL | mqtt5_opts.MQTT_SUB_OPT_RETAIN_AS_PUBLISHED
    else:
        opts = 0

    mid = 1
    subscribe_packet = dimq_test.gen_subscribe(mid, "bridge/#", 2 | opts, proto_ver=proto_ver)
    suback_packet = dimq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 3
    subscribe2_packet = dimq_test.gen_subscribe(mid, "bridge/#", 2 | opts, proto_ver=proto_ver)
    suback2_packet = dimq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 4
    subscribe3_packet = dimq_test.gen_subscribe(mid, "bridge/#", 2 | opts, proto_ver=proto_ver)
    suback3_packet = dimq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 2
    publish_packet = dimq_test.gen_publish("bridge/disconnect/test", qos=2, mid=mid, payload="disconnect-message", proto_ver=proto_ver)
    publish_dup_packet = dimq_test.gen_publish("bridge/disconnect/test", qos=2, mid=mid, payload="disconnect-message", dup=True, proto_ver=proto_ver)
    pubrec_packet = dimq_test.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet = dimq_test.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet = dimq_test.gen_pubcomp(mid, proto_ver=proto_ver)

    ssock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ssock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    ssock.settimeout(40)
    ssock.bind(('', port1))
    ssock.listen(5)

    broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

    try:
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        dimq_test.expect_packet(bridge, "connect", connect_packet)
        bridge.send(connack_packet)

        dimq_test.expect_packet(bridge, "subscribe", subscribe_packet)
        bridge.send(suback_packet)

        # Helper
        helper_connect_packet = dimq_test.gen_connect("test-helper", keepalive=keepalive, proto_ver=proto_ver)
        helper_connack_packet = dimq_test.gen_connack(rc=0, proto_ver=proto_ver)

        mid = 312
        helper_publish_packet = dimq_test.gen_publish("bridge/disconnect/test", qos=2, mid=mid, payload="disconnect-message", proto_ver=proto_ver)
        helper_pubrec_packet = dimq_test.gen_pubrec(mid=mid, proto_ver=proto_ver)
        helper_pubrel_packet = dimq_test.gen_pubrel(mid=mid, proto_ver=proto_ver)
        helper_pubcomp_packet = dimq_test.gen_pubcomp(mid=mid, proto_ver=proto_ver)

        helper_sock = dimq_test.do_client_connect(helper_connect_packet, helper_connack_packet, port=port2, connack_error="helper connack")
        dimq_test.do_send_receive(helper_sock, helper_publish_packet, helper_pubrec_packet, "helper pubrec")
        dimq_test.do_send_receive(helper_sock, helper_pubrel_packet, helper_pubcomp_packet, "helper pubcomp")
        helper_sock.close()
        # End helper


        dimq_test.expect_packet(bridge, "publish", publish_packet)
        bridge.close()

        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        dimq_test.expect_packet(bridge, "connect", connect_packet)
        bridge.send(connack_packet)

        dimq_test.expect_packet(bridge, "2nd subscribe", subscribe2_packet)
        bridge.send(suback2_packet)

        dimq_test.expect_packet(bridge, "2nd publish", publish_dup_packet)
        bridge.send(pubrec_packet)

        dimq_test.expect_packet(bridge, "pubrel", pubrel_packet)
        bridge.close()

        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        dimq_test.expect_packet(bridge, "connect", connect_packet)
        bridge.send(connack_packet)

        dimq_test.expect_packet(bridge, "3rd subscribe", subscribe3_packet)
        bridge.send(suback3_packet)

        dimq_test.expect_packet(bridge, "2nd pubrel", pubrel_packet)
        bridge.send(pubcomp_packet)
        rc = 0

        bridge.close()
    except dimq_test.TestError:
        pass
    finally:
        os.remove(conf_file)
        try:
            bridge.close()
        except NameError:
            pass

        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        ssock.close()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test(proto_ver=4)
do_test(proto_ver=5)

exit(0)

