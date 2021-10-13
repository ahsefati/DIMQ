#!/usr/bin/env python3

# This tests the default ACL type access behaviour for when no ACL matches.

from dimq_test_helper import *
import json
import shutil

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous false\n")
        f.write("plugin ../../plugins/dynamic-security/dimq_dynamic_security.so\n")
        f.write("plugin_opt_config_file %d/dynamic-security.json\n" % (port))

def command_check(sock, command_payload, expected_response):
    command_packet = dimq_test.gen_publish(topic="$CONTROL/dynamic-security/v1", qos=0, payload=json.dumps(command_payload))
    sock.send(command_packet)
    response = json.loads(dimq_test.read_publish(sock))
    if response != expected_response:
        print("Expected: %s" % (expected_response))
        print("Received: %s" % (response))
        raise ValueError(response)



port = dimq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

add_client_command = { "commands": [{
    "command": "createClient", "username": "user_one",
    "password": "password", "clientid": "cid",
    "correlationData": "2" }]
}
add_client_response = {'responses': [{'command': 'createClient', 'correlationData': '2'}]}

get_access_command = { "commands": [{"command": "getDefaultACLAccess", "correlationData": "3" }]}
get_access_response = {'responses': [
    {
        "command": "getDefaultACLAccess",
        'data': {'acls': [
            {'acltype': 'publishClientSend', 'allow': False},
            {'acltype': 'publishClientReceive', 'allow': True},
            {'acltype': 'subscribe', 'allow': False},
            {'acltype': 'unsubscribe', 'allow': True}
        ]},
        "correlationData": "3"
    }]
}

allow_subscribe_command = { "commands": [
    {
        "command": "setDefaultACLAccess",
        "acls":[
            { "acltype": "subscribe", "allow": True }
		],
        "correlationData": "4" }
    ]
}
allow_subscribe_response = {'responses': [{'command': 'setDefaultACLAccess', 'correlationData': '4'}]}

allow_publish_send_command = { "commands": [
    {
        "command": "setDefaultACLAccess",
        "acls":[
            { "acltype": "publishClientSend", "allow": True }
		],
        "correlationData": "5" }
    ]
}
allow_publish_send_response = {'responses': [{'command': 'setDefaultACLAccess', 'correlationData': '5'}]}

allow_publish_recv_command = { "commands": [
    {
        "command": "setDefaultACLAccess",
        "acls":[
            { "acltype": "publishClientReceive", "allow": False }
		],
        "correlationData": "6" }
    ]
}
allow_publish_recv_response = {'responses': [{'command': 'setDefaultACLAccess', 'correlationData': '6'}]}

allow_unsubscribe_command = { "commands": [
    {
        "command": "setDefaultACLAccess",
        "acls":[
            { "acltype": "unsubscribe", "allow": False }
		],
        "correlationData": "7" }
    ]
}
allow_unsubscribe_response = {'responses': [{'command': 'setDefaultACLAccess', 'correlationData': '7'}]}

rc = 1
keepalive = 10
connect_packet_admin = dimq_test.gen_connect("ctrl-test", keepalive=keepalive, username="admin", password="admin")
connack_packet_admin = dimq_test.gen_connack(rc=0)

mid = 2
subscribe_packet_admin = dimq_test.gen_subscribe(mid, "$CONTROL/dynamic-security/#", 1)
suback_packet_admin = dimq_test.gen_suback(mid, 1)

connect_packet = dimq_test.gen_connect("cid", keepalive=keepalive, username="user_one", password="password", proto_ver=5)
connack_packet = dimq_test.gen_connack(rc=0, proto_ver=5)

mid = 3
subscribe_packet = dimq_test.gen_subscribe(mid, "topic", 0, proto_ver=5)
suback_packet_fail = dimq_test.gen_suback(mid, mqtt5_rc.MQTT_RC_NOT_AUTHORIZED, proto_ver=5)
suback_packet_success = dimq_test.gen_suback(mid, 0, proto_ver=5)

mid = 4
unsubscribe_packet = dimq_test.gen_unsubscribe(mid, "topic", proto_ver=5)
unsuback_packet_fail = dimq_test.gen_unsuback(mid, mqtt5_rc.MQTT_RC_NOT_AUTHORIZED, proto_ver=5)
unsuback_packet_success = dimq_test.gen_unsuback(mid, proto_ver=5)

mid = 5
publish_packet = dimq_test.gen_publish(topic="topic", mid=mid, qos=1, payload="message", proto_ver=5)
puback_packet_fail = dimq_test.gen_puback(mid, proto_ver=5, reason_code=mqtt5_rc.MQTT_RC_NOT_AUTHORIZED)
puback_packet_success = dimq_test.gen_puback(mid, proto_ver=5)

publish_packet_recv = dimq_test.gen_publish(topic="topic", qos=0, payload="message", proto_ver=5)

try:
    os.mkdir(str(port))
    shutil.copyfile("dynamic-security-init.json", "%d/dynamic-security.json" % (port))
except FileExistsError:
    pass

broker = dimq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = dimq_test.do_client_connect(connect_packet_admin, connack_packet_admin, timeout=5, port=port)
    dimq_test.do_send_receive(sock, subscribe_packet_admin, suback_packet_admin, "admin suback")

    # Add client
    command_check(sock, add_client_command, add_client_response)
    command_check(sock, get_access_command, get_access_response)

    csock = dimq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)

    # Subscribe should fail because default access is deny
    dimq_test.do_send_receive(csock, subscribe_packet, suback_packet_fail, "suback fail")

    # Set default subscribe access to allow
    command_check(sock, allow_subscribe_command, allow_subscribe_response)

    # Subscribe should succeed because default access is now allowed
    dimq_test.do_send_receive(csock, subscribe_packet, suback_packet_success, "suback success")

    # Publish should fail because publishClientSend default is denied
    dimq_test.do_send_receive(csock, publish_packet, puback_packet_fail, "puback fail")

    # Set default publish send access to allow
    command_check(sock, allow_publish_send_command, allow_publish_send_response)

    # Publish should now succeed because publishClientSend default is allow
    # We also receive the message because publishClientReceive default is allow.
    csock.send(publish_packet)
    dimq_test.receive_unordered(csock, puback_packet_success, publish_packet_recv, "puback success / publish recv")

    # Set default publish receive access to deny
    command_check(sock, allow_publish_recv_command, allow_publish_recv_response)

    # Publish should succeed because publishClientSend default is allow
    # We should *not* receive the publish because it has been disabled.
    dimq_test.do_send_receive(csock, publish_packet, puback_packet_success, "puback success")
    dimq_test.do_ping(csock)

    # Unsubscribe should succeed because default access is allowed
    dimq_test.do_send_receive(csock, unsubscribe_packet, unsuback_packet_success, "unsuback success")

    # Set default unsubscribe access to allow
    command_check(sock, allow_unsubscribe_command, allow_unsubscribe_response)

    # Subscribe should succeed because default access is allowed
    dimq_test.do_send_receive(csock, subscribe_packet, suback_packet_success, "suback success 2")

    # Unsubscribe should fail because default access is no longer allowed
    dimq_test.do_send_receive(csock, unsubscribe_packet, unsuback_packet_fail, "unsuback fail")

    csock.close()

    rc = 0

    sock.close()
except dimq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    try:
        os.remove(f"{port}/dynamic-security.json")
    except FileNotFoundError:
        pass
    os.rmdir(f"{port}")
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))


exit(rc)
