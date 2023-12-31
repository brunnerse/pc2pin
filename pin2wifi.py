#!/usr/bin/env python

import socket
import time
import argparse
from argparse import RawTextHelpFormatter
import threading


DEFAULT_IP = "192.168.0.1"
DEFAULT_PORT = 1337
VERSION = "1.0"
ACK = b"."

stopACKSend = False

def arrayRemoveEmpty(arr):
    for i in range(len(arr)-1, -1, -1):
        if arr[i] == "":
            del arr[i]
    return arr

def parseCommands(cmdStr):
    cmds = []
    idx = 0
    cmdArr = cmdStr.split(";")
    arrayRemoveEmpty(cmdArr)
    for cmd in cmdArr:
        vals = cmd.split(" ")
        arrayRemoveEmpty(vals)
        try:
            action = "r"
            pin = ""
            toWrite = ""
            repeat = "1"
            delay = "50"
            if (vals[0] in ["r", "read"]):
                action = "r"
            elif (vals[0] in ["w", "write"]):
                action = "w"
            elif (vals[0] in ["p", "pin", "m", "mode", "pinmode"]):
                action = "p" 
            elif (vals[0] in ["a", "ar", "AR", "ra", "RA", "analogRead"]):
                action = "a"
            elif (vals[0] in ["A", "aw", "AW", "wa", "WA", "analogWrite"]):
                action = "A"
            else:
                raise Exception(f"Action \"{vals[0]}\" invalid")
            pin = str(int(vals[1]))  # Make sure value is convertable to int
            if action not in  ["r", "a"]:
                toWrite = vals[2]
            if "-n" in vals:
                repeat = str(int(vals[vals.index("-n")+1])) # Make sure value is convertable to int
            if "-d" in vals:
                delay = str(int(vals[vals.index("-d")+1])) # Make sure value is convertable to int
            cmds.append(" ".join([action, pin, toWrite, repeat, delay]))
        except Exception as e:
            raise Exception(f"Invalid command \"{cmd}\": {e}")
    return cmds

def sendACKPeriodically(sock):
    while not stopACKSend:
        sock.sendall(ACK, socket.TCP_NODELAY)
        time.sleep(0.5)

parser = argparse.ArgumentParser(
    prog='pin2serial',
    description='Reads and writes Microcontroller Pins over WiFI connection',
    formatter_class=RawTextHelpFormatter,
    epilog=
        '''A single command has the following structure:
        <action> <pin> [value] [-n repeats] [-n delays] 

        action is:
        w for writing (value = 0/1/t for toggling), wa for analog writing (value=0-255),
        r for reading (value empty), ra for analog reading (value empty),
        p or m for setting the mode of the pin (value = out/in/pullup).
        the command is repeated <repeat> times (default 1).
        Between each repeat and after the command, the controller waits <delay> msec. (default 50)
        
        Examples:
        p 23 out; w 23 1; w 23 t -n 50 -n 100;
        ra 4;
        p 22 in; r 22 -n 10
        p 23 out; wa 23 128 -d 500; wa 23 255 -d 500; wa 23 0''')

parser.add_argument('-i', '--ip', help="The ip address of the microcontroller server")
parser.add_argument('-p', '--port', help="The port of the microcontroller server")
parser.add_argument('-v', '--verbose', action="store_true")
parser.add_argument('-k', '--keep', help="Keep the program open to receive more commands", action="store_true")
parser.add_argument('commands', nargs='*', help="[[action] [pin] [Value]* [-n repeats] [-d delay];]+"
                     )

args = parser.parse_args()

# First check data if in correct format
commands = []
if args.commands:
    commands = parseCommands(" ".join(args.commands))
else:
    args.keep = True


if not args.ip:
    args.ip = DEFAULT_IP
if not args.port:
    args.port = DEFAULT_PORT


# open port
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.settimeout(3)


if (args.verbose):
    print(f"Connecting to server {args.ip}:{args.port}...")
try:
    sock.connect((args.ip, args.port))
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
except KeyboardInterrupt:
    print("Received SIGINT")
    sock.close()
    exit(1)
except Exception as e:
    print(f"Failed to connect: {e}")
    exit(1)
print(f"Connected.")

sock.send(bytes("ESP_CLIENT " + VERSION + "\n", "utf8"))
serverVersion = sock.recv(20)
if (args.verbose):
    print("Server version: " + serverVersion.decode("utf-8"), end="")

if args.commands:
    print(">> " + " ".join(args.commands))
while True:
    for cmd in commands:
        repeat = 1
        if len(cmd) > 2:
            repeat = int(cmd.split(" ")[3])
        msg = bytes(cmd + "\n", "utf-8")
        totalsent = 0
        while totalsent < len(msg):
            sent = sock.send(msg[totalsent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent
        for i in range(repeat):
            resp = b""
            while True:
                try:
                    c = sock.recv(1)
                    resp += c
                except socket.timeout as e:
                    print("timeout")
                    break
                if b"\n" in c:
                    break
            resp = resp.decode("utf-8")
            print(resp, end="")
            if resp.startswith("[ERROR]"):
                break
    if not args.keep:
        break
    try:
        # send ACK to signal we didn't time out
        stopACKSend = False
        #threading.Thread(target=sendACKPeriodically, args=(sock,)).start()
        # TODO check whether server didn't time out
        cmdStr = input(">> ")
        if (cmdStr == "exit"):
            break
        elif (cmdStr == ""):
            commands = [""]
            continue
        commands = parseCommands(cmdStr)
    except KeyboardInterrupt:
        print("Received SIGINT")
        break
    except Exception as e:
        print(e)
        commands = []
    finally:
        stopACKSend = True

if (args.verbose):
    print("Closing connection...")
sock.close()



