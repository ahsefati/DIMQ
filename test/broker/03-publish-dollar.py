#!/usr/bin/env python3

# Test whether a PUBLISH to a topic starting with $ succeeds

from dimq_test_helper import *

rc = 1
mid = 19
keepalive = 60
connect_packet = dimq_test.gen_connect("pub-dollar-test", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

publish_packet = dimq_test.gen_publish("$test/test", qos=1, mid=mid, payload="message")
puback_packet = dimq_test.gen_puback(mid)

port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
    dimq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")

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

exit(rc)

