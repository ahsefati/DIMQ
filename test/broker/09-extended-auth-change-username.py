#!/usr/bin/env python3

# Check whether an extended auth plugin can change the username of a client.

from dimq_test_helper import *

def write_config(filename, acl_file, port, per_listener):
    with open(filename, 'w') as f:
        f.write("per_listener_settings %s\n" % (per_listener))
        f.write("port %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("acl_file %s\n" % (acl_file))
        f.write("auth_plugin c/auth_plugin_extended_single.so\n")

def write_acl(filename):
    with open(filename, 'w') as f:
        f.write('user new_username\n')
        f.write('topic readwrite topic/one\n')

port = dimq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
acl_file = os.path.basename(__file__).replace('.py', '.acl')

def do_test(per_listener):
    write_config(conf_file, acl_file, port, per_listener)
    write_acl(acl_file)
    rc = 1

    # Connect without a username - this means no access
    connect1_packet = dimq_test.gen_connect("client-params-test1", keepalive=42, proto_ver=5)
    connack1_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

    mid = 1
    subscribe_packet = dimq_test.gen_subscribe(mid, "topic/one", 1, proto_ver=5)
    suback_packet = dimq_test.gen_suback(mid, 1, proto_ver=5)

    mid = 2
    publish1_packet = dimq_test.gen_publish("topic/one", qos=1, mid=mid, payload="message", proto_ver=5)
    puback1_packet = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NOT_AUTHORIZED)

    # Connect without a username, but have the plugin change it
    props = mqtt5_props.gen_string_prop(mqtt5_props.PROP_AUTHENTICATION_METHOD, "change")
    connect2_packet = dimq_test.gen_connect("client-params-test2", keepalive=42, proto_ver=5, properties=props)
    props = mqtt5_props.gen_string_prop(mqtt5_props.PROP_AUTHENTICATION_METHOD, "change")
    connack2_packet = dimq_test.gen_connack(rc=0, proto_ver=5, properties=props)

    mid = 2
    publish2s_packet = dimq_test.gen_publish("topic/one", qos=1, mid=mid, payload="message", proto_ver=5)
    puback2s_packet = dimq_test.gen_puback(mid, proto_ver=5)

    mid = 1
    publish2r_packet = dimq_test.gen_publish("topic/one", qos=1, mid=mid, payload="message", proto_ver=5)

    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    try:
        sock = dimq_test.do_client_connect(connect1_packet, connack1_packet, timeout=20, port=port)
        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback1")
        dimq_test.do_send_receive(sock, publish1_packet, puback1_packet, "puback1")
        dimq_test.do_ping(sock)
        sock.close()

        sock = dimq_test.do_client_connect(connect2_packet, connack2_packet, timeout=20, port=port)
        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback2")
        sock.send(publish2s_packet)
        dimq_test.receive_unordered(sock, puback2s_packet, publish2r_packet, "puback2/publish2")
        dimq_test.do_ping(sock)
        rc = 0

        sock.close()

    except dimq_test.TestError:
        pass
    finally:
        os.remove(conf_file)
        os.remove(acl_file)
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test("true")
do_test("false")
exit(0)
