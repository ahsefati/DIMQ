import paho.mqtt.client as mqtt
import random
import string

host = "127.0.0.1"
port = 1883
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("We are connected to the HOST: " + host + " and PORT: " + str(port))

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print("Topic is: " + msg.topic + " " + "Content is: " + str(msg.payload))


def on_publish(client,userdata,result):
    print("Message sent")

client_id = "AHS_" + "".join(random.choices(string.ascii_uppercase + string.digits, k=6))
client = mqtt.Client(client_id= client_id)
client.on_connect = on_connect
client.on_message = on_message
client.on_publish = on_publish

client.connect(host, port, 60)

def subscribe(topicToSub):
    client.subscribe(topicToSub)
    print("Client: " + client_id + " Subscribed to the topic: " + topicToSub)

def publsih(topicToPub, contentToPub):
    client.publish(topicToPub, contentToPub)

subscribe(client_id + "/#")

publsih(client_id+"/toMe", "Hi!")


# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()