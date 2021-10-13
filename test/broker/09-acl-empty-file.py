#!/usr/bin/env python3

# Test for CVE-2018-xxxxx

from dimq_test_helper import *
import signal

def write_config(filename, port, per_listener):
    with open(filename, 'w') as f:
        f.write("per_listener_settings %s\n" % (per_listener))
        f.write("port %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("acl_file %s\n" % (filename.replace('.conf', '.acl')))

def write_acl(filename):
    with open(filename, 'w') as f:
        f.write('#comment\n')
        f.write('\n')


def do_test(port, per_listener):
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port, per_listener)

    acl_file = os.path.basename(__file__).replace('.py', '.acl')
    write_acl(acl_file)

    rc = 1
    keepalive = 60
    connect_packet = dimq_test.gen_connect("acl-check", keepalive=keepalive)
    connack_packet = dimq_test.gen_connack(rc=0)

    mid = 1
    publish_packet = dimq_test.gen_publish("test/topic", qos=0, payload="message")
    subscribe_packet = dimq_test.gen_subscribe(mid, "test/topic", 0)
    suback_packet = dimq_test.gen_suback(mid, 0)

    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    try:
        sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
        dimq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        sock.send(publish_packet)

        # If we receive the message, this will fail.
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

port = dimq_test.get_port()
do_test(port, "false")
do_test(port, "true")
