#!/usr/bin/python -u

import dimq
import serial

usb = serial.Serial(port='/dev/ttyUSB0', baudrate=57600)

dimq = dimq.dimq()
dimq.connect("localhost")
dimq.loop_start()

running = True
try:
    while running:
        line = usb.readline()
        dimq.publish("sensors/cc128/raw", line)
except usb.SerialException, e:
    running = False

dimq.disconnect()
dimq.loop_stop()

