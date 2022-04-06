import sys 
import random
import string
import paho.mqtt.client as mqtt
import time
import random


class DIMQClient():
    def __init__(self):
        self.clientId = "AHS_" + "".join(random.choices(string.ascii_uppercase + string.digits, k=6))
        self.client = mqtt.Client(client_id=self.clientId)
        self.host = ""
        self.port = 0

    def connect(self, host, port):
        self.host = host
        self.port = port
        self.client.on_connect = self.onConnect
        self.client.on_message = self.onMessage
        self.client.on_publish = self.onPublish
        self.client.connect(self.host,self.port)

        # Connect to itself Id
        self.client.subscribe(self.clientId + "/#")

        self.client.loop_start()

    def onConnect(self, client, userdata, flags, rc):
        print(self.clientId + " is connected to: " + self.host + " and " + str(self.port))     

    def onMessage(self, client, userdata, msg):
        print("To: " + self.clientId + ", Topic is: " + msg.topic + " " + "Content is: " + str(msg.payload))

    def onPublish(self, client,userdata,result):
        print("Message Sent from " + self.clientId)


    def subscribe(self, topicToSub):
        self.client.subscribe(topicToSub)
    
    def publish(self, topicToSub, msgToSend):
        self.client.publish(topicToSub, msgToSend)       



class Simulator():
    def __init__(self, numberOfClients, numberOfBrokers, endTime):
        self.numberOfClients = numberOfClients
        self.numberOfBrokers = numberOfBrokers
        self.endTime = endTime
        self.host = ""
        self.ports = []
        self.dimqClients = []
    
    def createBrokers(self):
        self.host = "127.0.0.1"
        self.ports = [i+1883 for i in range(self.numberOfBrokers)]

    def createClients(self):
        for i in range(self.numberOfClients):
            dimqClient = DIMQClient()
            dimqClient.connect(self.host, self.ports[random.randint(0,self.numberOfBrokers-1)])
            
            self.dimqClients.append(dimqClient)

            # print("Client number " + i + " created!")


    def simulateOneStep(self):
        pass


if __name__ == "__main__":
    print("\t\t______________Welcome to DIMQ PA______________")
    numberOfBrokers = 3
    numberOfClients = 10
    endTime = 100
    simulator = Simulator(numberOfClients, numberOfBrokers, endTime)
    simulator.createBrokers()

    simulator.createClients()

    start = 0
    while(start < endTime):
        simulator.simulateOneStep()
        start+=1
        