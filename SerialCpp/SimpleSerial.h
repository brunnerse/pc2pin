#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

class SimpleSerial {
    private:
        bool connected;

#ifdef _WIN32
        HANDLE hCom;
        DCB dcb;
#endif

    public:
        enum Parity {
            NO=0, ODD=1, EVEN=2, MARK=3, SPACE=4
        };
        enum StopBits {
            ONE=0, ONE5 = 1, TWO=2 
        };

        static std::vector<std::string> getAvailablePorts();
        static std::string getPortPath(std::string& port);
        static void printAvailablePorts();

        void printPortConfig();
        bool setPortConfig(uint32_t baudrate, uint32_t bytesize = 8, 
        SimpleSerial::Parity parityBit=NO, SimpleSerial::StopBits stopBits=ONE);
        bool setBaudrate(uint32_t baudrate);
        unsigned int getBaudrate();

        // sets different baud rates, sends data and checks if it gets a valid answer
        bool findBaudrate(const std::string& dataToSend);

        SimpleSerial(); 
        SimpleSerial(const std::string& port); 
        SimpleSerial(const std::string& port, uint32_t baudRate); 
        ~SimpleSerial();

        std::string read();

        
        uint32_t available();
        bool write(std::string data);
        bool open(const std::string& port);
        bool close();


        //parseInt(); readStringUntil(); ...

        bool isConnected() const {return connected;}
};