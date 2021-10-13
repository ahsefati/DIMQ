#!/usr/bin/env python3

# Test whether max_connections works with repeated connections

from dimq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("max_connections 10\n")

def do_test():
    rc = 1

    connect_packets_ok = []
    connack_packets_ok = []
    for i in range(0, 10):
        connect_packets_ok.append(dimq_test.gen_connect("max-conn-%d"%i, proto_ver=5))
        connack_packets_ok.append(dimq_test.gen_connack(rc=0, proto_ver=5))

    connect_packet_bad = dimq_test.gen_connect("max-conn-bad", proto_ver=5)
    connack_packet_bad = b""

    port = dimq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    socks = []
    try:
        # Open all allowed connections, a limit of 10
        for i in range(0, 10):
            socks.append(dimq_test.do_client_connect(connect_packets_ok[i], connack_packets_ok[i], port=port))

        # Try to open an 11th connection
        try:
            sock_bad = dimq_test.do_client_connect(connect_packet_bad, connack_packet_bad, port=port)
        except ConnectionResetError:
            # Expected behaviour
            pass

        # Close all allowed connections
        for i in range(0, 10):
            socks[i].close()

        ## Now repeat - check it works as before

        # Open all allowed connections, a limit of 10
        for i in range(0, 10):
            socks.append(dimq_test.do_client_connect(connect_packets_ok[i], connack_packets_ok[i], port=port))

        # Try to open an 11th connection
        try:
            sock_bad = dimq_test.do_client_connect(connect_packet_bad, connack_packet_bad, port=port)
        except ConnectionResetError:
            # Expected behaviour
            pass

        # Close all allowed connections
        for i in range(0, 10):
            socks[i].close()

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

do_test()
exit(0)
