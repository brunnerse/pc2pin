#1/usr/bin/python

import serial
import serial.tools.list_ports
import argparse

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
            if (vals[0] in ["r", "read"]):
                action = "r"
            elif (vals[0] in ["w", "write"]):
                action = "w"
            elif (vals[0] in ["p", "pin", "mode", "pinmode"]):
                action = "p" 
            else:
                raise Exception(f"Action \"{vals[0]}\" invalid")
            pin = vals[1]
            if action != "w":
                toWrite = vals[2]
            cmds.append(" ".join([action, pin, toWrite]))
        except:
            raise Exception(f"Invalid command \"{cmd}\"!")
    return cmds


parser = argparse.ArgumentParser(
    prog='pin2serial',
    description='Reads and writes Microcontroller Pins over USB serial connection',
    epilog='')#Text at the bottom of help')

parser.add_argument('-p', '--port', help="The serial port for the communication")
parser.add_argument('-b', '--baudrate', help="The baud rate for the serial port")
parser.add_argument('-v', '--verbose', action="store_true")
parser.add_argument('-k', '--keep', help="Keep the program open to receive more commands", action="store_true")
parser.add_argument('commands', nargs='*', help="[[read/write] [Pin] [Value]*];+,"+
                     " e.g.: pin 0 in; pin 3 out; read 1; write 3 high; read 1")


args = parser.parse_args()

# First check data if in correct format
commands = []
if args.commands:
    commands = parseCommands(" ".join(args.commands))
else:
    args.keep = True

if not args.port:
    print("Available serial ports: ")
    portsArr = serial.tools.list_ports.comports()
    ports = []
    for port, description, hwid in sorted(portsArr):
        print(f"\t{port}: {description}\t[{hwid}]")
        ports.append(port)
    while (not args.port or args.port not in ports):
        args.port = input("Choose port:")

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

if args.commands:
    print(">> " + " ".join(args.commands))
while True:
    for cmd in commands:
        ser.write(bytes(cmd + "\n", "utf8"))
        resp = ser.readline()
        print(resp)
    if not args.keep:
        break
    try:
        # TODO react correctly to CTRL+C
        cmdStr = input(">> ")
        if (cmdStr == "exit"):
            break
        elif (cmdStr == ""):
            commands = []
            continue
        commands = parseCommands(cmdStr.split(" "))
    except KeyboardInterrupt:
        print("Received SIGINT")
        break
    except:
        print("Invalid command!")
        commands = []

if (args.verbose):
    print("Closing serial connection...")
ser.close()



