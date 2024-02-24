#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <algorithm>

#include "SerialCpp/SimpleSerial.h"

#define TEST_BAUDRATE 0

using namespace std;


SimpleSerial serial;
int isExiting = false;

void handle_sigint(int sig) {
    cout << "\nReceived SIGINT, exiting..." << endl;
    isExiting = true;
    exit(0);
}

const char* getCmdOption(char ** begin, char ** end, const std::string & option, const std::string& alias = "")
{
    char ** itr = std::find(begin, end, option);
    if (itr == end && alias != "")
        itr = std::find(begin, end, alias);
    if (itr != end)
    {
        if (++itr != end)
            return *itr;
        return "";
    }
    return NULL;
}


std::vector<std::string> parseCommands(std::string& cmds) {
    std::vector<std::string> cmdArr;
    uint32_t endIdx = 0;
    cout << cmds << endl;
    for (uint32_t i = 0; i < cmds.length(); i = endIdx+1) {
        endIdx = cmds.find(';', i); 
        if (endIdx == std::string::npos)
            endIdx = cmds.length();
        std::string cmd = cmds.substr(i, endIdx-i); 
        cout << "Command: " << cmd << " idxs " << i << " " << endIdx << endl;

        char actionStr[20], argsStrs[5][20];
        uint32_t pin;
        int n = sscanf(cmd.c_str(), "%s %u %s %s %s %s %s", actionStr, &pin,
         argsStrs[0], argsStrs[1], argsStrs[2], argsStrs[3], argsStrs[4]);
        
        if (n < 2) {
            printf("Invalid command \"%s\": <action> <pin_nr> is wrong\n", cmd.c_str());
            return std::vector<std::string>(0);
        }

        char action = actionStr[0];
        if (action == 'm')
            action = 'p';
        if (action == 'a') {
            if (actionStr[1] == 'w')
                action = 'A';
            else if (actionStr[1] != '\0' && actionStr[1] != 'r')
                // Action is wrong
                action = '\0'; 
        }
        if (!(action == 'r' || action == 'w' || action == 'p' ||action == 'a' || action == 'A')) {
            printf("Invalid command \"%s\": Action \"%s\" is faulty\n", cmd.c_str(), actionStr);
            return std::vector<std::string>(0);
        }

        int argsIdx = 0;
        std::string value("");
        if (action != 'r' && action != 'a') {
            if (n < 3 || argsStrs[argsIdx][0] == '-') {
                printf("Invalid command \"%s\": action %s requires a value\n", cmd.c_str(), actionStr);
                return std::vector<std::string>(0);
            }
            value = argsStrs[argsIdx];
            argsIdx++;
        }
        // parse rest of command
        int repeat = 1;
        int delay = 50;
        for (;argsIdx + 2 < n; argsIdx++) {
            if (argsStrs[argsIdx][0] != '-') {
                printf("Invalid command \"%s\"\n", cmd.c_str());
                return std::vector<std::string>(0);
            }
            if (argsStrs[argsIdx][1] == 'n') {
               argsIdx++;
               repeat = atoi(argsStrs[argsIdx]); 
               if (repeat < 0) {
                    printf("Invalid command \"%s\": repeat value is wrong\n", cmd.c_str());
                    return std::vector<std::string>(0);
               }
            } else if (argsStrs[argsIdx][1] == 'd') {
               argsIdx++;
               delay = atoi(argsStrs[argsIdx]); 
               if (delay < 0) {
                    printf("Invalid command \"%s\": delay value is wrong\n", cmd.c_str());
                    return std::vector<std::string>(0);
               }
            }
        }

        std::stringstream stream;
        stream << action << ' ' << pin << ' ' << value << ' ' << repeat << ' ' << delay << '\n';
        cmdArr.push_back(stream.str());
    }
    return cmdArr;
}

int main(int argc, char *argv[]) {

    /*Parse arguments*/
    if (getCmdOption(argv, argv + argc, "-h", "--help")) {
        printf("Usage: %s [-h] [-p PORT] [-b BAUDRATE] [-v] [-c]\n", argv[0]);
        printf( "  -h, --help            show this help message and exit\n"
                "  -p PORT, --port PORT  The serial port for the communication\n"
                "  -b BAUDRATE, --baudrate BAUDRATE  The baud rate for the serial port\n"
                "  -v, --verbose Print detailed information\n"
                "  -c, --close   Close the program immediately after sending the given command\n");
        printf("A single command has the following structure: \n"
        "    <action> <pin> [value] [-n repeats] [-n delays]\n\n" 
        "action is:\n"
        "   w for writing (value = 0/1/t/n with t for toggling and n for nothing,\n"
        "   aw for analog writing (value=0-255),\n"
        "   r for reading,\n"
        "   ar for analog reading,\n"
        "   p or m for setting the mode of the pin (value = out/in/pullup).\n"
        "   the command is repeated <repeat> times (default 1).\n"
        "   Between each repeat and after the command, the controller waits <delay> msec. (default 50)\n\n"
        "Examples:\n"
        "   p 23 out; w 23 1; w 23 t -n 50 -n 100;   (set pin 23 to output, write 1, then toggle it 50 times) \n"
        "   ra 4;   (read analog value from pin 4)\n"
        "   p 22 in; r 22 -n 10   (set pin 22 to input and read its value 10 times)\n"
        "   p 23 out; wa 23 128 -d 500; wa 23 255 -d 500; wa 23 0\n");
        return 0;
    }

    bool keepOpen = getCmdOption(argv, argv+argc, "-c", "--close") == NULL;
    bool verbose = getCmdOption(argv, argv+argc, "-v", "--verbose") != NULL;
    const char *portArg = getCmdOption(argv, argv + argc, "-p", "--port");
    const char *rateArg = getCmdOption(argv, argv + argc, "-b", "--baudrate");


    // Find if any commands are given in argv after all options  
    int commandIdx = argc;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'p' || argv[i][1] == 'b')
                i++;
        } else {
            commandIdx = i;
            break;
        }
    }

    string cmdInput;
    for (int i = commandIdx; i < argc; i++) {
        cmdInput += argv[i];
    }



    signal(SIGINT, handle_sigint);

    std::string port;
    if (portArg != NULL) {
        port = portArg;
        if (!serial.open(port)) {
            printf("[Serial] Error: Failed to connect to serial port \"%s\"\n", port.c_str());
            port = "";
        }
    }
    if (port == "") {
        std::vector<std::string> ports = SimpleSerial::getAvailablePorts();
        std::cout << "Available serial ports:" << std::endl;
        SimpleSerial::printAvailablePorts();
        std::cout << "\nChoose port:" << std::endl;
        while (!serial.isConnected())  {
            printf(">> ");
            std::getline(cin, port);
            if (isExiting || port.compare("exit") == 0)
                return 0;
            if (!serial.open(port)) 
                printf("[Serial] Error: Failed to open serial port \"%s\"\n", port.c_str());
        }
    } 

    if (rateArg != NULL) {
        serial.setBaudrate(atol(rateArg));
//        serial.setPortConfig(19200, 8, SimpleSerial::Parity::NO,
//            SimpleSerial::StopBits::ONE );
    }
    if (verbose) {
        printf("Connected to port %s:\n", port.c_str());
        serial.printPortConfig();
    }

    serial.setTotalTimeouts(2000, 1000);


#if TEST_BAUDRATE
    uint32_t baudrate = serial.findBaudrate("test", "test");
    if (baudrate == 0) {
        printf("Finding baudrate failed!\n");
        return 1;
    }
#endif


    if (cmdInput.length() > 0)
        printf(">> %s\n", cmdInput.c_str());
    while (!isExiting) {
        auto cmds = parseCommands(cmdInput);
        for (std::string cmd : cmds) {
            // get repeat argument
            uint32_t idx = 0;
            for (uint32_t i = 0; i < 3; i++)
                idx = cmd.find(' ', idx) + 1;
            uint32_t repeat = atoi(cmd.substr(idx).c_str()); 
            if (verbose)
                printf("Sending %s (Receiving %d times)\n", cmd.c_str(), repeat);
            // Write cmd
            if (!serial.write(cmd)) {
                cout << "[Serial] Failed to send." << endl;
                break;
            }
            // Read from serial n times
            for (uint32_t i = 0; i < repeat; i++) {
                std::string resp = serial.read(100);
                std::cout << resp;
            }
        }
        if (!keepOpen)
            break;
        
        cout << ">> ";
        std::getline(cin, cmdInput);

        if (cmdInput.compare("exit") == 0 || !serial.isConnected() || isExiting)
            break;
    }

    return 0;
}