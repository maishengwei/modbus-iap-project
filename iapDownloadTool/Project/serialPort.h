#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "framework.h"
#include <string.h>
#include <vector>
#include <SetupAPI.h>
#include <initguid.h>

using namespace std;

#ifndef GUID_DEVINTERFACE_COMPORT
DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86E0D1E0L, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73);
#endif

#define FACTORY_READ_TIMEOUT        1000    // ms
#define MAX_RW_BUFFER_SIZE          256     // byte

struct SerialPortInfo
{
    std::string portName;
    std::string description;
};

typedef struct tagSerialPortParam {
    std::string comPort;
    std::string baudrate;
    std::string stopBit;
    std::string dataBit;
    std::string parity;
}SerialPortParam, *LPSerialPortParam;

BOOL connectSerialPort(SerialPortParam param);
VOID disconnectSerialPort(VOID);
VOID serialPortTransmit(PUINT8 buf, UINT len);
VOID serialPortReceive(PUINT8 buf, PUINT8 len, UINT timeout);
BOOL isSerialPortConnected(VOID);
std::wstring stringToWstring(std::string str);
std::string wstringToString(const std::wstring& wstr);
bool enumDetailsSerialPorts(vector<SerialPortInfo>& portInfoList);

#endif //SERIALPORT_H
