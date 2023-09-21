// Project.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "Project.h"
#include "serialPort.h"
#include "strsafe.h"
#include <commdlg.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING          100
#define EDIT_HEIGHT_OFFSET      45
#define EDIT_WIDTH_OFFSET       1
#define MAX_STR_SIZE            1024
#define CRC_CAL_TIMEOUT         10      // ms
#define IAP_DOWNLOAD_PACK_SIZE  1024	// bytes

#define EDIT_DISPLAY_APPEND(str) \
    do{\
        SendMessage(hwndDisplayWin, EM_SETSEL, -2, -1); \
        SendMessage(hwndDisplayWin, EM_REPLACESEL, TRUE, (LPARAM)str); \
    }while(0)

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
static HWND hwndStatus;                         // 状态栏
static HMENU hMenu;                             // 主菜单栏
static HWND hwndDisplayWin;                     // 显示框
static HWND hwndEditDevAddr;                    // 设备地址
static HWND hwndEditCmd;                        // 指令
static HWND hwndEditCRC;                        // CRC16
static HWND hwndButtonSend;                     // 发送按钮
static HFONT hFont;                             // 字体
BOOL isStartEdit = FALSE;
TCHAR editCmdStrBuf[MAX_STR_SIZE] = { 0 };
// modbus相关
static INT deviceID = -1;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Communicate(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK       TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);      // 计时器回调函数
BOOL                EditCmdFriendlyUpdate(PTCHAR inBuf, UINT inSize, PTCHAR outBuf, PUINT outSize);
unsigned short getCRC16(volatile uint8_t *ptr, uint16_t len);
VOID translateStrToHex(PTCHAR str, PUINT8 hexBuf, PUINT8 hexBufLen);
VOID translateHexToStr(PTCHAR str, UINT strSiz, LPCTSTR header, PUINT8 hexBuf, UINT8 hexBufLen);
// IAP下载
BOOL packageDataSendAndReceive(UINT16 index, PUINT8 data, UINT16 dataLen);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 设置字体
    hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, \
        0, 0, 0, 0, TEXT("宋体"));

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROJECT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECT));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PROJECT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_PROJECT));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
      (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX),
      CW_USEDEFAULT, CW_USEDEFAULT,
      600, 700, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR tempStrBuf[MAX_STR_SIZE] = { 0 };
    RECT rect;
    INT statusH = 0;
    INT interval = 0, cx = 0, cy = 0;
    INT notifiCode = 0;
    INT bufSize = 0;
    UINT8 dataToSend[100] = { 0 };
    UINT8 dataReceive[100] = { 0 };
    UINT8 dataLen = 0;
    UINT16 crc16 = 0;
    UINT16 u16Buf[4] = { 2, 9, 3, 2 };          // WM_SIZE各子窗口比例

    TCHAR szFile[MAX_STR_SIZE] = { 0 };         // 返回用户选择的文件名的缓冲区
    TCHAR szFileTitle[MAX_STR_SIZE] = { 0 };    // 返回用户所选文件的文件名和扩展名的缓冲区

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter =
        TEXT("二进制文件(*.bin)\0*.bin\0");
    ofn.nFilterIndex = 1;                       // 默认选择第1个过滤器
    ofn.lpstrFile = szFile;                     // 返回用户选择的文件名的缓冲区
    ofn.lpstrFile[0] = NULL;                    // 不需要初始化文件名编辑控件
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrFileTitle = szFileTitle;	        // 返回用户选择的文件的文件名和扩展名的缓冲区
    ofn.nMaxFileTitle = _countof(szFileTitle);
    ofn.lpstrInitialDir = TEXT(".\\");          // 初始目录
    ofn.lpstrTitle = TEXT("请选择要打开的文件");
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    FILE *fp = NULL;
    UINT32 fileSize = 0;
    UINT16 packageNum = 0;
    UINT32 i = 0;
    UINT8 packageData[IAP_DOWNLOAD_PACK_SIZE] = { 0 };
    UINT16 lastPackSize = 0;

    switch (message)
    {
    case WM_CREATE:
        hMenu = GetMenu(hWnd);
        hwndStatus = CreateWindowEx(0, TEXT("msctls_statusbar32"), NULL,
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUSBAR,
            hInst, NULL);
        SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)TEXT("就绪"));
        hwndDisplayWin = CreateWindowEx(0, TEXT("Edit"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            ES_LEFT | ES_AUTOVSCROLL | ES_READONLY | ES_MULTILINE| ES_WANTRETURN,
            0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_DISPLAY, hInst, NULL);
        hwndEditDevAddr = CreateWindowEx(0, TEXT("Edit"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER,
            0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_DEVADDR, hInst, NULL);
        hwndEditCmd = CreateWindowEx(0, TEXT("Edit"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_COMMAND, hInst, NULL);
        hwndEditCRC = CreateWindowEx(0, TEXT("Edit"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_CRC16, hInst, NULL);
        hwndButtonSend = CreateWindowEx(0, TEXT("Button"), NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON | BS_ICON,
            0, 0, 0, 0, hWnd, (HMENU)IDC_BUTTON_SEND, hInst, NULL);
        SendDlgItemMessage(hWnd, IDC_BUTTON_SEND, BM_SETIMAGE, IMAGE_ICON, \
            (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_SEND), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
        SendMessage(hwndEditDevAddr, WM_SETTEXT, NULL, (LPARAM)TEXT("设备地址"));
        SendMessage(hwndEditCmd, WM_SETTEXT, NULL, (LPARAM)TEXT("指令"));
        SendMessage(hwndEditCRC, WM_SETTEXT, NULL, (LPARAM)TEXT("CRC16"));
        // 设置子窗口控件字体
        SendMessage(hwndDisplayWin, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditDevAddr, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditCmd, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditCRC, WM_SETFONT, (WPARAM)hFont, FALSE);
        // 开启定时器
        SetTimer(hWnd, IDT_TIMER_CRC_CAL, CRC_CAL_TIMEOUT, TimerProc);
        break;
    case WM_SIZE:
        GetWindowRect(hWnd, &rect);
        SendMessage(hwndStatus, WM_SIZE, 0, 0);
        SendMessage(hwndStatus, SB_GETRECT, 0, (LPARAM)&rect);
        statusH = rect.bottom - rect.top;
        MoveWindow(hwndDisplayWin, EDIT_WIDTH_OFFSET, 0,
            LOWORD(lParam) - EDIT_WIDTH_OFFSET * 2,
            HIWORD(lParam) - EDIT_HEIGHT_OFFSET, TRUE);
        // 2 : 9 : 3 : 2
        interval = (LOWORD(lParam) - EDIT_WIDTH_OFFSET * 2) / (u16Buf[0] + u16Buf[1] + u16Buf[2] + u16Buf[3]);
        cx = 1, cy = HIWORD(lParam) - EDIT_HEIGHT_OFFSET;
        MoveWindow(hwndEditDevAddr, cx, cy, interval * u16Buf[0], EDIT_HEIGHT_OFFSET - statusH, TRUE);
        cx += interval * u16Buf[0];
        MoveWindow(hwndEditCmd, cx, cy, interval * u16Buf[1], EDIT_HEIGHT_OFFSET - statusH, TRUE);
        cx += interval * u16Buf[1];
        MoveWindow(hwndEditCRC, cx, cy, interval * u16Buf[2], EDIT_HEIGHT_OFFSET - statusH, TRUE);
        cx += interval * u16Buf[2];
        MoveWindow(hwndButtonSend, cx, cy,\
            (LOWORD(lParam) - EDIT_WIDTH_OFFSET * 2 - interval * (u16Buf[0] + u16Buf[1] + u16Buf[2])),\
            EDIT_HEIGHT_OFFSET - statusH, TRUE);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_CONNECT:
                GetMenuString(hMenu, IDM_CONNECT, tempStrBuf, 50, MF_BYCOMMAND);
                if (_tcscmp(TEXT("连接 ..."), tempStrBuf) == 0) {
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_CONNECTBOX), hWnd, Communicate);
                }
                else {
                    disconnectSerialPort();
                    SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)TEXT("串口连接断开"));
                    ModifyMenu(hMenu, IDM_CONNECT, MF_STRING, IDM_CONNECT, TEXT("连接 ..."));
                }
                break;
            case IDC_EDIT_DEVADDR:
                notifiCode = HIWORD(wParam);
                switch (notifiCode) {
                case EN_SETFOCUS:
                    if (!isStartEdit) {
                        SendMessage(hwndEditDevAddr, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                        SendMessage(hwndEditCmd, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                        isStartEdit = TRUE;
                    }
                    break;
                case EN_CHANGE:
                    if (isStartEdit) {
                        deviceID = GetDlgItemInt(hWnd, IDC_EDIT_DEVADDR, NULL, FALSE);
                        if (deviceID > 255) {
                            MessageBox(hWnd, TEXT("设备地址不能超过255"), TEXT("错误提示"),
                                MB_OK | MB_ICONERROR | MB_APPLMODAL);
                            SendMessage(hwndEditDevAddr, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                            deviceID = -1;
                        }
                        break;
                    }
                    else {
                        return DefWindowProc(hWnd, message, wParam, lParam);
                    }
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
                break;
            case IDC_EDIT_COMMAND:
                notifiCode = HIWORD(wParam);
                switch (notifiCode) {
                case EN_SETFOCUS:
                    if (!isStartEdit) {
                        SendMessage(hwndEditDevAddr, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                        SendMessage(hwndEditCmd, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                        isStartEdit = TRUE;
                    }
                    break;
                case EN_CHANGE:
                    if (isStartEdit) {
                        tempStrBuf[0] = MAX_STR_SIZE;
                        bufSize = SendMessage(hwndEditCmd, EM_GETLINE, 0, (LPARAM)tempStrBuf);
                        if ((_tcscmp(editCmdStrBuf, tempStrBuf) == 0) || (bufSize == 0)) {
                            if (0 == bufSize) {
                                ZeroMemory(editCmdStrBuf, sizeof(editCmdStrBuf));
                            }
                            break;
                        }
                        if (!EditCmdFriendlyUpdate(tempStrBuf, bufSize, editCmdStrBuf, (PUINT)&bufSize)) {
                            MessageBox(hWnd, TEXT("指令输入错误"), TEXT("错误提示"),
                                MB_OK | MB_ICONERROR | MB_APPLMODAL);
                            SendMessage(hwndEditCmd, WM_SETTEXT, NULL, (LPARAM)TEXT(""));
                            ZeroMemory(editCmdStrBuf, sizeof(editCmdStrBuf));
                            break;
                        }
                        else {
                            SendMessage(hwndEditCmd, WM_SETTEXT, NULL, (LPARAM)editCmdStrBuf);
                            // 设置光标
                            SendMessage(hwndEditCmd, EM_SETSEL, bufSize - 1, bufSize - 1);
                            break;
                        }
                    }
                    else {
                        return DefWindowProc(hWnd, message, wParam, lParam);
                    }
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
                break;
            case IDC_BUTTON_SEND:
                if (!isSerialPortConnected()) {
                    MessageBox(hWnd, TEXT("请先连接串口"), TEXT("错误提示"),
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    break;
                }
                if ((editCmdStrBuf[0] == 0) || (-1 == deviceID)) { break; }
                dataToSend[0] = deviceID;
                if (editCmdStrBuf[0] == 0) { break; }
                translateStrToHex(editCmdStrBuf, dataToSend + 1, &dataLen);
                dataLen += 1;
                crc16 = getCRC16(dataToSend, dataLen);
                dataToSend[dataLen++] = crc16 & 0xFF;
                dataToSend[dataLen++] = (crc16 >> 8) & 0xFF;
                serialPortTransmit(dataToSend, dataLen);
                translateHexToStr(tempStrBuf, MAX_STR_SIZE, TEXT("[发送] "),dataToSend, dataLen);
                EDIT_DISPLAY_APPEND(tempStrBuf);
                serialPortReceive(dataReceive, &dataLen, 100);
                if (dataLen == 0) {
                    EDIT_DISPLAY_APPEND(TEXT("[接收] None\r\n"));
                }
                else {
                    translateHexToStr(tempStrBuf, MAX_STR_SIZE, TEXT("[接收] "), dataReceive, dataLen);
                    EDIT_DISPLAY_APPEND(tempStrBuf);
                }
                break;
            case ID_CLRWIN:
                SendMessage(hwndDisplayWin, EM_SETSEL, 0, -1);
                SendMessage(hwndDisplayWin, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));
                break;
            case ID_DOWNLOADAPP:
                if (!isSerialPortConnected()) {
                    MessageBox(hWnd, TEXT("请先连接串口"), TEXT("错误提示"),
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    break;
                }
                if (-1 == deviceID) {
                    MessageBox(hWnd, TEXT("设备地址不能为空"), TEXT("错误提示"),
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);
                    break;
                }
                if (GetOpenFileName(&ofn)) {
                    wsprintf(tempStrBuf, TEXT("文件：%s\r\n"), ofn.lpstrFile);
                    EDIT_DISPLAY_APPEND(tempStrBuf);
                    // 获取文件大小
                    fopen_s(&fp, wstringToString(wstring(ofn.lpstrFile)).c_str(), "rb");
                    fseek(fp, 0, SEEK_END);
                    fileSize = ftell(fp);
                    wsprintf(tempStrBuf, TEXT("文件大小为：%d 字节\r\n"), fileSize);
                    EDIT_DISPLAY_APPEND(tempStrBuf);
                    // 要发送的数据包数量
                    packageNum = fileSize / IAP_DOWNLOAD_PACK_SIZE;
                    lastPackSize = fileSize % IAP_DOWNLOAD_PACK_SIZE;
                    if (lastPackSize != 0) {
                        packageNum += 1;
                    }
                    else {
                        lastPackSize = IAP_DOWNLOAD_PACK_SIZE;
                    }
                    // 发送数据包
                    for (i = 0; i < packageNum; i++) {
                        // 设置文件指针
                        fseek(fp, i * IAP_DOWNLOAD_PACK_SIZE, SEEK_SET);
                        // 获取数据
                        fread(packageData, 1,\
                            (i != (packageNum - 1))? IAP_DOWNLOAD_PACK_SIZE : lastPackSize, fp);
                        if (packageDataSendAndReceive((i * 8), packageData, \
                            (i != (packageNum - 1)) ? IAP_DOWNLOAD_PACK_SIZE : lastPackSize)) {
                            wsprintf(tempStrBuf, TEXT("发送数据包：%d/%d\t[成功]\r\n"), (i + 1), packageNum);
                            EDIT_DISPLAY_APPEND(tempStrBuf);
                        }
                        else {
                            wsprintf(tempStrBuf, TEXT("发送数据包：%d/%d\t[失败]\r\n"), (i + 1), packageNum);
                            EDIT_DISPLAY_APPEND(tempStrBuf);
                            break;
                        }
                    }
                }
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        disconnectSerialPort();
        KillTimer(hWnd, IDT_TIMER_CRC_CAL);
		DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#define GET_CURRENT_COMBO_CONTENT(hDlg, index, comboID, stringBuf, stringOut) \
    do{\
        index = SendDlgItemMessage(hDlg, comboID, CB_GETCURSEL, NULL, NULL);\
        SendDlgItemMessage(hDlg, comboID, CB_GETLBTEXT, index, (LPARAM)stringBuf);\
        *stringOut = wstringToString(stringBuf);\
    }while(0)

// “通信”框的消息处理程序。
INT_PTR CALLBACK Communicate(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    LPCTSTR baudrateList[4] = { TEXT("1200"), TEXT("2400"), TEXT("4800"), TEXT("9600") };
    LPCTSTR stopBitList[3] = { TEXT("1"), TEXT("1.5"), TEXT("2") };
    LPCTSTR dataBitList[4] = { TEXT("5"), TEXT("6"), TEXT("7"), TEXT("8") };
    LPCTSTR chksumList[3] = { TEXT("无"), TEXT("奇校验"), TEXT("偶校验") };
    vector<SerialPortInfo> spInfo;
    TCHAR tempStringBuf[100] = { 0 };
    INT curSelIndex = 0;
    SerialPortParam serialPortParam;
    BOOL ret = FALSE;

    switch (message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hDlg, IDC_BUTTON_REFRESH, BM_SETIMAGE, IMAGE_ICON,\
            (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_REFRESH), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
        // load baudrate list
        for (INT i = 0; i < 4; i++) {
            SendDlgItemMessage(hDlg, IDC_COMBO_BAUDRATE, CB_ADDSTRING, NULL, (LPARAM)baudrateList[i]);
        }
        // load stop bit
        for (INT i = 0; i < 3; i++) {
            SendDlgItemMessage(hDlg, IDC_COMBO_STOPBIT, CB_ADDSTRING, NULL, (LPARAM)stopBitList[i]);
        }
        // load data bit
        for (INT i = 0; i < 4; i++) {
            SendDlgItemMessage(hDlg, IDC_COMBO_DATABIT, CB_ADDSTRING, NULL, (LPARAM)dataBitList[i]);
        }
        // load checksum
        for (INT i = 0; i < 3; i++) {
            SendDlgItemMessage(hDlg, IDC_COMBO_CHECKSUM, CB_ADDSTRING, NULL, (LPARAM)chksumList[i]);
        }
        // set default value
        SendDlgItemMessage(hDlg, IDC_COMBO_BAUDRATE, CB_SELECTSTRING, -1, (LPARAM)baudrateList[3]);
        SendDlgItemMessage(hDlg, IDC_COMBO_STOPBIT, CB_SELECTSTRING, -1, (LPARAM)stopBitList[0]);
        SendDlgItemMessage(hDlg, IDC_COMBO_DATABIT, CB_SELECTSTRING, -1, (LPARAM)dataBitList[3]);
        SendDlgItemMessage(hDlg, IDC_COMBO_CHECKSUM, CB_SELECTSTRING, -1, (LPARAM)chksumList[0]);
        // click refresh com port button
        SendDlgItemMessage(hDlg, IDC_BUTTON_REFRESH, BM_CLICK, NULL, NULL);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        INT wmId = LOWORD(wParam);
        switch (wmId) {
        case IDOK:
            GET_CURRENT_COMBO_CONTENT(hDlg, curSelIndex, IDC_COMBO_PORT, \
                tempStringBuf, &serialPortParam.comPort);
            if (serialPortParam.comPort.size() != 0) {
                serialPortParam.comPort = serialPortParam.comPort.substr(0, serialPortParam.comPort.find_first_of(' '));
                GET_CURRENT_COMBO_CONTENT(hDlg, curSelIndex, IDC_COMBO_BAUDRATE, \
                    tempStringBuf, &serialPortParam.baudrate);
                GET_CURRENT_COMBO_CONTENT(hDlg, curSelIndex, IDC_COMBO_STOPBIT, \
                    tempStringBuf, &serialPortParam.stopBit);
                GET_CURRENT_COMBO_CONTENT(hDlg, curSelIndex, IDC_COMBO_DATABIT, \
                    tempStringBuf, &serialPortParam.dataBit);
                GET_CURRENT_COMBO_CONTENT(hDlg, curSelIndex, IDC_COMBO_CHECKSUM, \
                    tempStringBuf, &serialPortParam.parity);
                ret = connectSerialPort(serialPortParam);
                SendMessage(hwndStatus, SB_SETTEXT, 0,
                    (LPARAM)(ret ? TEXT("串口连接成功") : TEXT("串口连接失败")));
                if (ret) {
                    //EnableMenuItem(hMenu, IDM_CONNECT, MF_GRAYED);
                    ModifyMenu(hMenu, IDM_CONNECT, MF_STRING, IDM_CONNECT, TEXT("断开连接"));
                }
            }
            else {
                SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)TEXT("没选择串口"));
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON_REFRESH:
            SendDlgItemMessage(hDlg, IDC_COMBO_PORT, CB_RESETCONTENT, NULL, NULL);
            enumDetailsSerialPorts(spInfo);
            for (UINT i = 0; i < spInfo.size(); i++) {
                wsprintf(tempStringBuf, TEXT("%s %s"),
                    stringToWstring(spInfo[i].portName).c_str(),
                    stringToWstring(spInfo[i].description).c_str());
                SendDlgItemMessage(hDlg, IDC_COMBO_PORT, CB_ADDSTRING, NULL, (LPARAM)tempStringBuf);
            }
            SendDlgItemMessage(hDlg, IDC_COMBO_PORT, CB_SETCURSEL, 0, NULL);
            return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

/**
* @brief  calculate MODBUS CRC16.
* @retval CRC16
*/
unsigned short getCRC16(volatile uint8_t *ptr, uint16_t len)
{
    uint8_t  i;
    unsigned short crc = 0xFFFF;
    if (len == 0)
    {
        len = 1;
    }
    while (len--)
    {
        crc ^= *ptr;
        for (i = 0; i < 8; i++)
        {
            if (crc & 1)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
        ptr++;
    }
    return(crc);
}

VOID translateStrToHex(PTCHAR str, PUINT8 hexBuf, PUINT8 hexBufLen) {
    TCHAR tempChar[2] = { 0 };
    INT i = 0;
    INT strLen = _tcslen(str);
    *hexBufLen = 0;
    while (i <= strLen) {
        tempChar[0] = str[i];
        if (str[i + 1] != TEXT('\0')) {
            tempChar[1] = str[i + 1];
            swscanf_s(tempChar, TEXT("%02X"), (PUINT)(hexBuf + *hexBufLen));
            *hexBufLen += 1;
        }
        else {
            swscanf_s(tempChar, TEXT("%01X"), (PUINT)(hexBuf + *hexBufLen));
            *hexBufLen += 1;
            break;
        }
        i += 3;
    }
}

VOID translateHexToStr(PTCHAR str, UINT strSize, LPCTSTR header, PUINT8 hexBuf, UINT8 hexBufLen) {
    wstring retStr = wstring(header);
    for (INT i = 0; i < hexBufLen; i++) {
        TCHAR tempStr[10] = { 0 };
        wsprintf(tempStr, TEXT("%02X"),hexBuf[i]);
        retStr += wstring(tempStr);
        if (i != hexBufLen) {
            retStr += wstring(TEXT(" "));
        }
    }
    retStr += wstring(TEXT("\r\n"));
    StringCchCopy(str, strSize, retStr.c_str());
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    UINT8 dataBuf[100] = { 0 };
    UINT8 dataLen = 0;
    UINT16 crc16 = 0;
    TCHAR tempStr[MAX_STR_SIZE] = { 0 };
    switch (idEvent) {
    case IDT_TIMER_CRC_CAL:
        if (-1 == deviceID){ break; }
        dataBuf[0] = deviceID;
        translateStrToHex(editCmdStrBuf, dataBuf + 1, &dataLen);
        dataLen += 1;
        crc16 = getCRC16(dataBuf, dataLen);
        wsprintf(tempStr, TEXT("%02X %02X"), crc16 & 0xFF, (crc16 >> 8) & 0xFF);
        SendMessage(hwndEditCRC, WM_SETTEXT, NULL, (LPARAM)tempStr);
        break;
    }
}

BOOL EditCmdFriendlyUpdate(PTCHAR inBuf, UINT inSize, PTCHAR outBuf, PUINT outSize) {
    UINT j = 0;
    for (UINT i = 0; i < inSize; i++) {
        BOOL isValid = ((inBuf[i] >= TEXT('0') && inBuf[i] <= TEXT('9')) || \
                        (inBuf[i] >= TEXT('A') && inBuf[i] <= TEXT('F')) || \
                        (inBuf[i] == TEXT(' ')));
        if (!isValid) {
            return FALSE;
        }
        if (inBuf[i] != TEXT(' ')){
            outBuf[j++] = inBuf[i];
        }
    }
    PTCHAR temp = new TCHAR[j * 2];
    UINT k = 0;
    for (UINT i = 0; i < j; i++) {
        temp[k++] = outBuf[i];
        if (((i + 1) % 2 == 0) && (i < (j - 1))) {
            temp[k++] = TEXT(' ');
        }
    }
    memcpy(outBuf, temp, k * sizeof(TCHAR));
    delete temp;
    outBuf[k++] = TEXT('\0');
    *outSize = k;
    return TRUE;
}

BOOL packageDataSendAndReceive(UINT16 index, PUINT8 data, UINT16 dataLen) {
    UINT8 tempU8buf[1200] = {0};
    UINT16 crc16 = 0;
    UINT16 len = 0;
	UINT8 revLen = 0;
	/* 计算发送的数据长度 */
	if ((dataLen % 128) != 0) {
		len = ((UINT16)((dataLen / 128) + 1)) * 128;
	}
	else {
		len = dataLen;
	}
	UINT16 lenBk = len;
	/* 开始发送 */
    tempU8buf[0] = deviceID;
    tempU8buf[1] = 0x10;
    tempU8buf[2] = (index >> 8) & 0xFF;
    tempU8buf[3] = index & 0xFF;
    tempU8buf[4] = 0;
    tempU8buf[5] = len / 128;
    tempU8buf[6] = len / 64;
	if ((dataLen % 128) != 0) {
		memcpy(tempU8buf + 7, data, dataLen);
		memset(tempU8buf + 7 + dataLen, 0xFF, (len - dataLen));
	}
	else {
		memcpy(tempU8buf + 7, data, len);
	}
	len += 7;
    crc16 = getCRC16(tempU8buf, len);
    tempU8buf[len++] = crc16 & 0xFF;
    tempU8buf[len++] = (crc16 >> 8) & 0xFF;
    serialPortTransmit(tempU8buf, len);
    serialPortReceive(tempU8buf, &revLen, 200);
    /* simple check */
    crc16 = getCRC16(tempU8buf, 6);
    if ( (crc16 == (UINT16)((tempU8buf[7] << 8) | tempU8buf[6]))    &&\
         (tempU8buf[0] == deviceID)                                 &&\
         (tempU8buf[1] == 0x10)                                     &&\
         (index == (UINT16)((tempU8buf[2] << 8) | tempU8buf[3]))    &&\
         ((lenBk / 128) == (UINT16)((tempU8buf[4] << 8) | tempU8buf[5]))
    ){
        return TRUE;
    }
    else {
        return FALSE;
    }
}
