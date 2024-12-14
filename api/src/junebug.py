import os
import sys
import csv
import time
import threading
import socket
import concurrent.futures
import signal

filepath = __file__.removesuffix("junebug.py")
roomData = {}
active = True
running = True

def runner():
    while running:
        c, addr = listener.accept()

def update_data():
    while active:
        os.system(f"{filepath}../bin/pull_lldp.out")

        with open("records_out.csv", "r") as csvfile:
            linereader = csv.reader(csvfile)
            for row in linereader:
                if row[1] in roomData.keys():
                    uptime = roomData[row[1]][2]
                    roomData.update({row[1]: [row[2], row[3], uptime+1]})
                else:
                    roomData.update({row[1]: [row[2], row[3], 1]})

def handle_request():
    return

def signal_handler(sig, frame):
    running = False
    active = False
    updateThread.join()
    runnerThread.join()
    exit(0)

os.system(f"gcc {filepath}pull_lldp.c -o {filepath}../bin/pull_lldp.out")

signal.signal(signal.SIGINT, signal_handler)
listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '127.0.0.1'
port = 8888
listener.bind((host, port))
listener.listen(5)
updateThread = threading.Thread(target=update_data)
updateThread.start()
time.sleep(1)

runnerThread = threading.Thread(target=runner)
runnerThread.start()

