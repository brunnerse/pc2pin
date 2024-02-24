#include "SimpleSerial.h"
#include <string>
#include <string.h>
#include <stdlib.h>


#define DEBUG

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#else 
#error "No WIN32!"
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
#endif
}


std::string SimpleSerial::read(uint32_t maxBytes) {
    if (!connected) {
        return std::string("");
    }

    char* const buffer = new char[maxBytes+1];
    uint32_t bytesWritten;
    
    bool success = ReadFile(this->hCom, (void*)buffer, maxBytes, (DWORD*)&bytesWritten, NULL);
    if (!success)
        return std::string("");

    buffer[bytesWritten] = '\0'; 
    std::string s(buffer);
    delete[] buffer;
    return s;
}

// read without blocking. TODO or check if sth available?
uint32_t SimpleSerial::available() {
    return false;
}


bool SimpleSerial::write(std::string data) {
    if (!connected) {
        return false;
    }

    const char *buffer = data.c_str();
    uint32_t length = (uint32_t)data.length();
    uint32_t bytesWritten;
    
    bool success = WriteFile(this->hCom, buffer, length, (DWORD*)&bytesWritten, NULL);
    if (bytesWritten < length) {
        // TODO write rest again?
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

    //std::wstring wport(port.begin(), port.end());

    this->hCom = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE,
        0,      //  must be opened with exclusive-access
        NULL,   //  default security attributes
        OPEN_EXISTING, //  must use OPEN_EXISTING
        0,      //  not overlapped I/O
        NULL //  hTemplate must be NULL for comm devices
    );

    if (this->hCom != INVALID_HANDLE_VALUE) {
        this->connected = true;
#ifdef DEBUG
        printf("[Serial] Connected.\n");
#endif

        memset(&(this->dcb), 0, sizeof(DCB));
        if (GetCommState(hCom, &this->dcb) == 0) {
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
    CloseHandle(hCom);
#ifdef DEBUG
    printf("[Serial] Closed connection.\n");
#endif
    return true;
};


bool SimpleSerial::setPortConfig(uint32_t baudrate, uint32_t bytesize, 
        SimpleSerial::Parity parity, SimpleSerial::StopBits stopBits) {
    if (!this->connected)
        return false;
    this->dcb.BaudRate = (DWORD)baudrate;
    this->dcb.ByteSize = (BYTE)bytesize;
    this->dcb.Parity = (BYTE)parity;
    this->dcb.StopBits = (BYTE)stopBits;

    return SetCommState(this->hCom, &this->dcb) != 0;
}

bool  SimpleSerial::setBaudrate(uint32_t baudrate) {
    if (!this->connected)
        return false;
    this->dcb.BaudRate = baudrate;

    return SetCommState(this->hCom, &this->dcb) != 0;
}

unsigned int SimpleSerial::getBaudrate() {
    if (!this->connected)
        return 0;
    return this->dcb.BaudRate;
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
    wchar_t lpTargetPath[MAX_PATHSIZE];
    DWORD pathlen = QueryDosDeviceW(std::wstring(port.begin(), port.end()).c_str(), lpTargetPath, MAX_PATHSIZE);
    if (pathlen == 0)
        return std::string("");
    //lpTargetPath[pathlen] = (wchar_t)0;
    std::wstring path(lpTargetPath);
    return std::string(path.begin(), path.end());
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

#endif

    return ports;
}

void SimpleSerial::printAvailablePorts() {
    std::vector<std::string> ports = SimpleSerial::getAvailablePorts();
    for (auto& p : ports) {
        printf("%s\t%s\n", p.c_str(), SimpleSerial::getPortPath(p).c_str()); 
    }
}
