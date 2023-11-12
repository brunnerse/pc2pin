import serial
import serial.tools.list_ports
import argparse


def parseCommands(cmdArr):
    cmds = []
    idx = 0
    while idx < len(cmdArr):
        try:
            read = False
            pin = ""
            toWrite = ""
            if (cmdArr[idx] in ["r", "read"]):
                read = True
            elif (cmdArr[idx] in ["w", "write"]):
                read = False
            else:
                raise Exception()
            idx += 1
            pin = cmdArr[idx]
            idx += 1
            if not read:
                toWrite = cmdArr[idx]
                idx += 1
            cmds.append((read, pin, toWrite))
        except:
            raise Exception(f"Command invalid!")
    return cmds

        

parser = argparse.ArgumentParser(
    prog='pin2serial',
    description='Reads and writes Microcontroller Pins over USB serial connection',
    epilog='')#Text at the bottom of help')

parser.add_argument('-p', '--port', help="The serial port for the communication")
parser.add_argument('-v', '--verbose')
parser.add_argument('commands', nargs='*', help="[[read/write] [Pin] [Value]*]+, e.g. read 1 write 3 high read 2")


args = parser.parse_args()

print("Port:", args.port)
print("Commands: ", args.commands)

# First check data if in correct format
commands = ""
if args.commands:
    commands = parseCommands(args.commands)

print(commands)

if not args.port:
    print("Available serial ports: ")
    portsArr = serial.tools.list_ports.comports()
    ports = []
    for port, description, hwid in sorted(portsArr):
        print(f"\t{port}: {description}\t[{hwid}]")
        ports.append(port)
    while (not args.port or args.port not in ports):
        args.port = input("Choose port:")





