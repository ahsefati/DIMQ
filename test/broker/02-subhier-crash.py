#!/usr/bin/env python3

# Test related to https://github.com/eclipse/dimq/issues/505

from dimq_test_helper import *

rc = 1
keepalive = 60
connect_packet = dimq_test.gen_connect("subhier-crash", keepalive=keepalive)
connack_packet = dimq_test.gen_connack(rc=0)

mid = 1
subscribe1_packet = dimq_test.gen_subscribe(mid, "topic/a", 0)
suback1_packet = dimq_test.gen_suback(mid, 0)

mid = 2
subscribe2_packet = dimq_test.gen_subscribe(mid, "topic/b", 0)
suback2_packet = dimq_test.gen_suback(mid, 0)

mid = 3
unsubscribe1_packet = dimq_test.gen_unsubscribe(mid, "topic/a")
unsuback1_packet = dimq_test.gen_unsuback(mid)

disconnect_packet = dimq_test.gen_disconnect()


port = dimq_test.get_port()
broker = dimq_test.start_broker(filename=os.path.basename(__file__), port=port)

def test():
    sock = dimq_test.do_client_connect(connect_packet, connack_packet, port=port)
    dimq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback 1")
    dimq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback 2")
    dimq_test.do_send_receive(sock, unsubscribe1_packet, unsuback1_packet, "unsuback")

    sock.send(disconnect_packet)
    sock.close()


try:
    time.sleep(0.5)

    test()
    # Repeat test to check broker is still there
    test()

    rc = 0

except dimq_test.TestError:
    pass
finally:
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))

exit(rc)

