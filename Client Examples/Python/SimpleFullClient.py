import sys 
import random
import string
import paho.mqtt.client as mqtt
import time
##############################################################################################
def on_connect(client, userdata, flags, rc):
    print("We are connected to the HOST: " + host + " and PORT: " + str(port))

def on_message(client, userdata, msg):
    print("Topic is: " + msg.topic + " " + "Content is: " + str(msg.payload))

def on_publish(client,userdata,result):
    print("Message Sent.")
##############################################################################################


##############################################################################################
def init_connection(client_id, host, port):
    client = mqtt.Client(client_id=client_id)
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_publish = on_publish
    client.connect(host, port, 60)

    return client
##############################################################################################

##############################################################################################
def init_subscribtions(myClient):
    # Subscribe to itself
    myClient[0].subscribe(myClient[1]+"/#")
    # Sub to general network to get brokers IPs and Ports
    myClient[0].subscribe("brokers/#")
##############################################################################################

##############################################################################################
def show_commands():
    print("Commands: \n\t1-`S`: Subscribe\n\t2-`P`: Publish\n\t3-`E`: Exit")
##############################################################################################

if __name__ == "__main__":
    host = "127.0.0.1"
    if sys.argv[1] == "-h":
        host = str(sys.argv[2])

    port = 1883
    if sys.argv[3] == "-p":
        port = int(sys.argv[4])

    client_id = "AHS_" + "".join(random.choices(string.ascii_uppercase + string.digits, k=6))
    client = init_connection(client_id, host, port)
    myClient = [client, client_id]
    init_subscribtions(myClient)

    myClient[0].loop_start()

    show_commands()
    command="S"
    while (command!="E"):
        command = input()
        if command=="S":
            topicToSub = input("Topic to Subscribe: ")
            myClient[0].subscribe(topicToSub)
        elif command=="P":
            topicToPub = input("Topic to Publish: ")
            contentToSend = input("Content to Publish: ")
            myClient[0].publish(topicToPub, contentToSend)