#include "SimpleSerial.h"
#include <string>
#include <string.h>
#include <stdlib.h>


#define DEBUG

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#else 
#include <fcnctl.h>
#include <errno.h>
#include <unistd.h>
#include <filesystem>
#include <fcnctl.h>
#endif


SimpleSerial::SimpleSerial()
: connected(false)
{
}

SimpleSerial::SimpleSerial(const std::string &port)
: connected(false)
{
   this->open(port);
}

SimpleSerial::SimpleSerial(const std::string &port, uint32_t baudRate)
: connected(false)
{
   if (this->open(port))
        this->setBaudrate(baudRate);
}

SimpleSerial::~SimpleSerial()
{
    if (this->connected)
    {
        this->close();
    }
}

bool SimpleSerial::setTotalTimeouts(uint32_t readTimeoutMs, uint32_t writeTimeoutMs) {
    if (!connected)
        return false;
#ifdef _WIN32
    COMMTIMEOUTS timeouts;
/*
    bool success = GetCommTimeouts(this->hCom, &timeouts);
    if (!success)
        return false;
*/
    timeouts.ReadIntervalTimeout = 1;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = readTimeoutMs;
    timeouts.WriteTotalTimeoutConstant = writeTimeoutMs;
    return SetCommTimeouts(this->hCom, &timeouts); 
#else
    this->tty.c_cc[VTIME] = readTimeoutMs / 100;
    this->tty.c_cc[VMIN] = 0;
    // TODO writeTimeoutMs not possible; remove?
#endif
}


std::string SimpleSerial::read(uint32_t maxBytes) {
    if (!connected) {
        return std::string("");
    }

    char* const buffer = new char[maxBytes+1];
    uint32_t bytesWritten;
    bool success;    
#ifdef _WIN32
    success = ReadFile(this->hCom, (void*)buffer, maxBytes, (DWORD*)&bytesWritten, NULL);
#else
    bytesWritten = read(this->fId, buffer, maxBytes);
    success = bytesWritten != -1;
#endif
    if (!success)
        return std::string("");

    buffer[bytesWritten] = '\0'; 
    std::string s(buffer);
    delete[] buffer;
    return s;
}


bool SimpleSerial::write(std::string data) {
    if (!connected) {
        return false;
    }

    const char *buffer = data.c_str();
    uint32_t length = (uint32_t)data.length();
    uint32_t bytesWritten;
    bool success; 
#ifdef _WIN32
    success = WriteFile(this->hCom, buffer, length, (DWORD*)&bytesWritten, NULL);
#else
    bytesWritten = write(this->fId, buffer, length);
    success = bytesWritten != -1; 
#endif
    if (bytesWritten < length) {
        // TODO try to write rest again?
#ifdef DEBUG
        printf("[Serial] write(): Wrote %u out of %u bytes.", bytesWritten, length);
#endif
    }
    return success;
}


bool SimpleSerial::open(const std::string& port) {
    if (this->connected) {
    #ifdef DEBUG
        printf("[Serial] Error during open(): There is already an open connection.");
    #endif
        return false;
    }
#ifdef DEBUG
    printf("[Serial] Connecting to port %s...\n", port.c_str());
#endif

#ifdef _WIN32
    this->hCom = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE,
        0,      //  must be opened with exclusive-access
        NULL,   //  default security attributes
        OPEN_EXISTING, //  must use OPEN_EXISTING
        0,      //  not overlapped I/O
        NULL //  hTemplate must be NULL for comm devices
    );

    if (this->hCom != INVALID_HANDLE_VALUE) {
#else
    this->hCom = open(port.c_str(), O_RDWR);
    if (this->hCom < 0) {
#endif
        this->connected = true;
#ifdef DEBUG
        printf("[Serial] Connected.\n");
#endif


#ifdef _WIN32
        memset(&(this->dcb), 0, sizeof(DCB));
        if (GetCommState(hCom, &this->dcb) == 0) {
#else
        bool success = tcgetattr(this->fId, &this->tty) == 0;
        if (success) {
            this->tty.c_cflag &= ~CRTSCTS; 
            this->tty.c_cflag |= CREAD | CLOCAL; 
            // Disable canonical mode and signal chars
            this->tty.c_lflag &= ~ICANON & ~ISIG;
            this->tty.c_iflag &= ~IXON | ~IXOFF | ~IXANY;
            // Disable any special handling of received bytes
            this->tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
            this->tty.c_oflag &= ~OPOST & ~ONCLR;

            this->tty.c_cc[VTIME] = 20; 
            this->tty.c_cc[VMIN] = 0;
            success = tcsetattr(this->fId, &this->tty) == 0;
        }
        if (!success) {        
#endif
#ifdef DEBUG
            printf("[Serial] Error: Failed to get port config.\n");
#endif
            this->close();
            return false;
        }


        return true;
    } else {
#ifdef DEBUG
        printf("[Serial] Failed to connect.\n");
#endif
        return false;
    }
}

bool SimpleSerial::close() {
    if (!this->connected)  {
#ifdef DEBUG
        printf("[Serial] Error: close() has been called on an already closed connection\n");
#endif
        return false;
    }
    bool success;
#ifdef _WIN32
    success = CloseHandle(this->hCom);
#else
    success = close(this->fId) == 0;
#endif
#ifdef DEBUG
    printf("[Serial] Closed connection.\n");
#endif
    return success;
};


bool SimpleSerial::setPortConfig(uint32_t baudrate, uint32_t bytesize, 
        SimpleSerial::Parity parity, SimpleSerial::StopBits stopBits) {
    if (!this->connected)
        return false;

#ifdef _WIN32
    this->dcb.BaudRate = (DWORD)baudrate;
    this->dcb.ByteSize = (BYTE)bytesize;
    this->dcb.Parity = (BYTE)parity;
    this->dcb.StopBits = (BYTE)stopBits;

    return SetCommState(this->hCom, &this->dcb) != 0;
#else
    this->cfsetispeed(this->tty, baudrate);
    this->cfsetospeed(this->tty, baudrate);

    this->tty.c_cflag &= ~CSIZE;
    if (bytesize == 5) {
        this->tty.c_cflag |= CS5;
    } else if (bytesize == 6) {
        this->tty.c_cflag |= CS6;
    } else if (bytesize == 7) {
        this->tty.c_cflag |= CS7;
    }
    } else if (bytesize == 8) {
        this->tty.c_cflag |= CS8;
    } else {
        return false;
    }

    if (parity == Parity::NO)
        this->tty.c_cflag &= ~PARENB;
    else if (parity == PARITY::ODD) {
        this->tty.c_cflag |= PARENB;
    } else {
        return false;
    }

    if (stopBits == StopBits::ONE) {
        tty.c_cflag &= ~CSTOPB;
    } else if (stopBits == StopBits::TWO) {
        tty.c_cflag |= CSTOPB;
    } else {
        return false;
    }

    return tcsetattr(this->fId, TCSAFLUSH, this->tty) == 0;
#endif
}

bool  SimpleSerial::setBaudrate(uint32_t baudrate) {
    if (!this->connected)
        return false;
#ifdef _WIN32
    this->dcb.BaudRate = baudrate;
    return SetCommState(this->hCom, &this->dcb) != 0;
#else
    this->cfsetispeed(this->tty, baudrate);
    this->cfsetospeed(this->tty, baudrate);
    return tcsetattr(this->fId, TCSAFLUSH, this->tty) == 0;
#endif
}

uint32_t SimpleSerial::findBaudrate(const std::string& dataToSend, const std::string& dataToExpect) {
    if (!connected)
        return 0;
#ifdef DEBUG
    printf("[Serial] Testing baud rates by sending \"%s\" and expecting to receive \"%s\"...\n",
        dataToSend.c_str(), dataToExpect.c_str());
#endif
    static const uint32_t rates[] = {110, 300, 600, 1200, 2400, 4800, 9600,
    14400, 19200, 38400, 57600, 115200, 128000, 256000}; 

    for (uint32_t rate : rates) {
        this->setBaudrate(rate);
#ifdef DEBUG
        printf("[Serial] Testing baud rate %u ...\n", rate);
#endif
        this->write(dataToSend);
        std::string output = this->read(dataToSend.length()+10);
        if (output.compare(dataToExpect) == 0) {
#ifdef DEBUG
           printf("[Serial] Success for baud rate %u: Received \"%s\"\n", rate, output.c_str());
#endif
           return rate;  
        } else {
#ifdef DEBUG
           printf("[Serial] Failed: Received \"%s\"\n", output.c_str());
#endif
        }
    }
    // No correct baud rate found
    return 0;

}

unsigned int SimpleSerial::getBaudrate() {
    if (!this->connected)
        return 0;
#ifdef _WIN32
    return this->dcb.BaudRate;
#else
    return cfgetispeed(&this->tty);
#endif
}

void SimpleSerial::printPortConfig() {
    if (!connected) {
        printf("[Serial] Not connected.\n");
        return;
    }
    const char* parityStr = "";
    switch (dcb.Parity) {
    case Parity::NO:
        parityStr = "NoParity"; 
        break;
    case Parity::ODD:
        parityStr = "OddParity"; 
        break;
    case Parity::EVEN:
        parityStr = "EvenParity"; 
        break;
    case Parity::MARK:
        parityStr = "MarkParity"; 
        break;
    case Parity::SPACE:
        parityStr = "SpaceParity"; 
        break;
    } 
    float fStopBit = (dcb.StopBits == StopBits::ONE) ? 1.0f : 
        (dcb.StopBits == StopBits::ONE5) ? 1.5f : 2.0f;
    printf( "[Serial] BaudRate = %lu, ByteSize = %u, Parity = %s, StopBits = %g\n", 
        dcb.BaudRate, dcb.ByteSize, parityStr, fStopBit);
}


std::string SimpleSerial::getPortPath(std::string& port) {
    const uint32_t MAX_PATHSIZE = 100;
#ifdef _WIN32
    wchar_t lpTargetPath[MAX_PATHSIZE];
    DWORD pathlen = QueryDosDeviceW(std::wstring(port.begin(), port.end()).c_str(), lpTargetPath, MAX_PATHSIZE);
    if (pathlen == 0)
        return std::string("");
    std::wstring path(lpTargetPath);
    return std::string(path.begin(), path.end());
#else
    char filePath[MAX_PATHSIZE];
    if (fcnctl(this->fId, F_GETPATH, filePath) == -1)
        return std::string("");
    // TODO look into content of directory instead
    return std::string(filePath);
#endif
}

std::vector<std::string> SimpleSerial::getAvailablePorts() {
    std::vector<std::string> ports;
#ifdef _WIN32
    const uint32_t MAX_PATHSIZE = 100;
    wchar_t lpTargetPath[MAX_PATHSIZE];
    for (uint32_t i = 0; i < 255; i++) {
        std::string port = "COM" + std::to_string(i);
        DWORD pathlen = QueryDosDeviceW(std::wstring(port.begin(), port.end()).c_str(), lpTargetPath, MAX_PATHSIZE);
        if (pathlen != 0) {
            ports.push_back(port);
        }
    }
#else
    const char *dir = "/dev/";
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        cout << entry.path() << endl;
        if (entry.path().string().starts_with("tty")) {
            ports.push_back(dir + entry.path().string());
        }
    }
#endif

    return ports;
}

void SimpleSerial::printAvailablePorts() {
    std::vector<std::string> ports = SimpleSerial::getAvailablePorts();
    for (auto& p : ports) {
        printf("%s\t%s\n", p.c_str(), SimpleSerial::getPortPath(p).c_str()); 
    }
}
