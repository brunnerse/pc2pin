#pragma once
#include <vector>
#include <string>
#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <termios.h>
#endif

class SimpleSerial {
    private:
        bool connected;

#ifdef _WIN32
        HANDLE hCom;
        DCB dcb;
#else 
        int fId;
        struct termios tty;
#endif

    public:
        enum Parity {
#ifdef _WIN32
            NO=NOPARITY, ODD=ODDPARITY, EVEN=EVENPARITY, MARK=MARKPARITY, SPACE=SPACEPARITY
#else
            NO, ODD;
#endif
        };
        enum StopBits {
            ONE=0, ONE5 = 1, TWO=2 
        };

        static std::vector<std::string> getAvailablePorts();
        static std::string getPortPath(std::string& port);
        static void printAvailablePorts();

        void printPortConfig();
        bool setPortConfig(uint32_t baudrate, uint32_t bytesize = 8, 
        SimpleSerial::Parity parity=NO, SimpleSerial::StopBits stopBits=ONE);
        bool setBaudrate(uint32_t baudrate);
        unsigned int getBaudrate();

        bool setTotalTimeouts(uint32_t readTimeoutMS, uint32_t writeTimeoutMS);

        // sets different baud rates, sends data and checks if it gets a valid answer
        uint32_t findBaudrate(const std::string& dataToSend, const std::string& dataToExpect);

        SimpleSerial(); 
        SimpleSerial(const std::string& port); 
        SimpleSerial(const std::string& port, uint32_t baudRate); 
        ~SimpleSerial();

        std::string read(uint32_t maxBytes);

        bool write(std::string data);
        bool open(const std::string& port);
        bool close();


        //parseInt(); readStringUntil(); ...

        bool isConnected() const {return connected;}
};