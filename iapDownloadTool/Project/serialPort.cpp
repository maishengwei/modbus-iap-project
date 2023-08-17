#include "pch.h"
#include "serialPort.h"

#pragma comment (lib, "setupapi.lib")

static HANDLE serialPortHandle;
static BOOL isConnected = FALSE;

BOOL connectSerialPort(SerialPortParam param) {
    UINT portNum = atoi(param.comPort.substr(param.comPort.find("COM")+3).c_str());
    if (portNum >= 10) {
        param.comPort = "\\\\.\\" + param.comPort;
    }
    serialPortHandle = CreateFile(stringToWstring(param.comPort).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (serialPortHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //设置读超时
    COMMTIMEOUTS timeouts;
    GetCommTimeouts(serialPortHandle, &timeouts);
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = FACTORY_READ_TIMEOUT;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(serialPortHandle, &timeouts);

    //设置读写缓冲区大小
    if (!SetupComm(serialPortHandle, MAX_RW_BUFFER_SIZE, MAX_RW_BUFFER_SIZE))
    {
        CloseHandle(serialPortHandle);
        return false;
    }

    //设置串口配置信息
    DCB dcb;
    if (!GetCommState(serialPortHandle, &dcb))
    {
        CloseHandle(serialPortHandle);
        return false;
    }
    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = atoi(param.baudrate.c_str());
    wstring tempWStr = stringToWstring(param.parity);
    if (tempWStr == TEXT("无")) {
        dcb.Parity = NOPARITY;
    }
    else if (tempWStr == TEXT("奇校验")) {
        dcb.Parity = ODDPARITY;
    }
    else if (tempWStr == TEXT("偶校验")) {
        dcb.Parity = EVENPARITY;
    }
    dcb.ByteSize = atoi(param.dataBit.c_str());
    FLOAT tempFval = (FLOAT)atof(param.stopBit.c_str());
    if (tempFval == 1) {
        dcb.StopBits = ONESTOPBIT;
    }
    else if (tempFval == 1.5) {
        dcb.StopBits = ONE5STOPBITS;
    }
    else if (tempFval == 2) {
        dcb.StopBits = TWOSTOPBITS;
    }
    if (!SetCommState(serialPortHandle, &dcb))
    {
        CloseHandle(serialPortHandle);
        return false;
    }

    //清空缓冲
    PurgeComm(serialPortHandle, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    //清除错误
    DWORD dwError;
    COMSTAT cs;
    if (!ClearCommError(serialPortHandle, &dwError, &cs))
    {
        CloseHandle(serialPortHandle);
        return false;
    }

    return (isConnected = TRUE);
}

VOID disconnectSerialPort(VOID) {
    if (isConnected) {
        isConnected = FALSE;
        CloseHandle(serialPortHandle);
    }
}

VOID serialPortTransmit(PUINT8 buf, UINT len) {
    WriteFile(serialPortHandle, buf, len, NULL, NULL);
}

VOID serialPortReceive(PUINT8 buf, PUINT8 len, UINT timeout) {
    Sleep(timeout);
    //获取当前通信状态
    DWORD dwErrors;
    COMSTAT Rcs;
    ClearCommError(serialPortHandle, &dwErrors, &Rcs);
    *len = Rcs.cbInQue & 0xFF;
    //读
    ReadFile(serialPortHandle, buf, (*len), NULL, NULL);
}

BOOL isSerialPortConnected(VOID) {
    return isConnected;
}

// https://www.cnblogs.com/flylong0204/articles/5354690.html
std::wstring stringToWstring(std::string str)
{
    if (str.length() == 0)
        return L"";

    std::wstring wstr;
    wstr.assign(str.begin(), str.end());
    return wstr;
}

std::string wstringToString(const std::wstring& wstr)
{
    // https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
    if (wstr.empty())
    {
        return std::string();
    }

    int size = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string ret = std::string(size, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &ret[0], size, NULL, NULL); // CP_UTF8

    return ret;
}

bool enumDetailsSerialPorts(vector<SerialPortInfo>& portInfoList)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdienumdeviceinfo

    bool bRet = false;
    SerialPortInfo m_serialPortInfo;

    std::string strFriendlyName;
    std::string strPortName;

    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

    // Return only devices that are currently present in a system
    // The GUID_DEVINTERFACE_COMPORT device interface class is defined for COM ports. GUID
    // {86E0D1E0-8089-11D0-9CE4-08003E301F73}
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (INVALID_HANDLE_VALUE != hDevInfo)
    {
        SP_DEVINFO_DATA devInfoData;
        // The caller must set DeviceInfoData.cbSize to sizeof(SP_DEVINFO_DATA)
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++)
        {
            // get port name
            TCHAR portName[256];
            HKEY hDevKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (INVALID_HANDLE_VALUE != hDevKey)
            {
                DWORD dwCount = 255; // DEV_NAME_MAX_LEN
                RegQueryValueEx(hDevKey, _T("PortName"), NULL, NULL, (BYTE*)portName, &dwCount);
                RegCloseKey(hDevKey);
            }

            // get friendly name
            TCHAR fname[256];
            SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)fname,
                sizeof(fname), NULL);

#ifdef UNICODE
            strPortName = wstringToString(portName);
            strFriendlyName = wstringToString(fname);
#else
            strPortName = std::string(portName);
            strFriendlyName = std::string(fname);
#endif
            // remove (COMxx)
            strFriendlyName = strFriendlyName.substr(0, strFriendlyName.find(("(COM")));

            m_serialPortInfo.portName = strPortName;
            m_serialPortInfo.description = strFriendlyName;
            portInfoList.push_back(m_serialPortInfo);
        }

        if (ERROR_NO_MORE_ITEMS == GetLastError())
        {
            bRet = true; // no more item
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return bRet;
}
