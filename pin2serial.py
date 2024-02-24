#!/usr/bin/env python

import serial
import serial.tools.list_ports
import argparse
from argparse import RawTextHelpFormatter

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


parser = argparse.ArgumentParser(
    prog='pin2serial',
    description='Reads and writes Microcontroller Pins over USB serial connection',
    formatter_class=RawTextHelpFormatter,
    epilog=
        '''A single command has the following structure:
        <action> <pin> [value] [-n repeats] [-n delays] 

        action is:
        w for writing (value = 0/1/t/n with t for toggling and n for nothing (basically printing the output val)),
        wa for analog writing (value=0-255),
        r for reading (value empty), ra for analog reading (value empty),
        p or m for setting the mode of the pin (value = out/in/pullup).
        the command is repeated <repeat> times (default 1).
        Between each repeat and after the command, the controller waits <delay> msec. (default 50)
        
        Examples:
        p 23 out; w 23 1; w 23 t -n 50 -n 100;
        ra 4;
        p 22 in; r 22 -n 10
        p 23 out; wa 23 128 -d 500; wa 23 255 -d 500; wa 23 0''')

parser.add_argument('-p', '--port', help="The serial port for the communication")
parser.add_argument('-b', '--baudrate', help="The baud rate for the serial port")
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


portsArr = serial.tools.list_ports.comports()
ports = []
for port, __, __ in portsArr:
    ports.append(port)

if args.port and not args.port in ports:
    print(f"[ERROR] Port {args.port} not available")
    args.port = None
if not args.port:
    if len(ports) == 0:
        print("No serial ports available")
        exit(2)
    print("Available serial ports: ")
    for port, description, hwid in sorted(portsArr):
        print(f"\t{port}: {description}\t[{hwid}]")
    while (not args.port or args.port not in ports):
        args.port = input("Choose port: ")
        if (args.port == "exit"):
            exit(0)

if not args.baudrate:
    args.baudrate = 19200

# open port
ser = serial.Serial(args.port, int(args.baudrate), timeout=1)
if (args.verbose):
    print(f"Opening connection to port {args.port}...")

if not ser.is_open:
    print("Failed to open serial port. Parameter were:")
    print(ser)
    exit(1)
print(f"Connected. ({ser})")

if args.commands:
    print(">> " + " ".join(args.commands))
while True:
    for cmd in commands:
        repeat = 1
        if len(cmd) > 2:
            repeat = int(cmd.split(" ")[3])
        ser.flush()
        ser.write(bytes(cmd + "\n", "ascii"))
        for i in range(repeat):
            try:
                resp = ser.readline()
                for j in range(len(resp)):
                    if resp[j] > 127:
                        resp = resp[:j]+b'?'+resp[j+1:]
                resp = resp.decode("ascii")
                print(resp, end="")
                if resp.startswith("[ERROR]"):
                    break
            except UnicodeDecodeError as e:
                print(e)
    if not args.keep:
        break
    try:
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

if (args.verbose):
    print("Closing serial connection...")
ser.close()



