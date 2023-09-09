// Project.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "Project.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;

#define MAX_LOADSTRING 100
/* window size self-adjusting */
#define WIN_OFFSET_X                4
#define WIN_OFFSET_Y                4
#define WIN_GROUPBOX_FILE_HEIGHT    90
#define WIN_STATIC_IAP_OFFSET_X     10
#define WIN_STATIC_IAP_WIDTH        60
#define WIN_STATIC_HEIGHT           20
#define WIN_GROUPBOX_CTL_HEIGHT     50
#define WIN_STATIC_DEV_WIDTH        40

// 烧录用结构体
typedef struct tagDownloadSettingStruct
{
    string deviceName;
    string version;
    string iapFilePath;
    string appFilePath;
    string appAddrS;
    string downloadToolSet;
}downloadSetStruct;

// 烧录工具枚举
enum DonwloadToolEnum
{
    JLINK = 1,
};

// JLink烧录设置结构体
typedef struct tagJLinkSettingStruct
{
    string toolPath;
    string device;
    string interface;
    string speed;
}JLinkSetStruct;

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
/* 子窗口控件 */
static HFONT hFont;
static HWND hwndGroupBoxFile;
static HWND hwndStaticIAP;
static HWND hwndEditIAP;
static HWND hwndButtonIAP;
static HWND hwndStaticAPP;
static HWND hwndEditAPP;
static HWND hwndButtonAPP;
static HWND hwndGroupBoxControl;
static HWND hwndStaticDevice;
static HWND hwndComboBoxDevice;
static HWND hwndStaticVer;
static HWND hwndComboBoxVer;
static HWND hwndStaticAppAddr;
static HWND hwndEditAppAddr;
static HWND hwndButtonDownload;
static HWND hwndEditDisplay;
/* 烧录设置 */
vector<downloadSetStruct> downloadSetList;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT                 enumDownloadSettings(vector<downloadSetStruct> & setlist);
VOID                loadComboBoxDevice(VOID);
VOID                loadComboBoxVersion(VOID);
VOID                loadDownloadEditInfo(VOID);
VOID                downloadProc(VOID);

// 错误提示
#define ERROR_HINT(hwnd, str) \
    MessageBox(hwnd, TEXT(str), TEXT("错误提示"), \
               MB_OK | MB_ICONERROR | MB_APPLMODAL);

// 清空编辑控件文本
#define CLEAR_EDIT_TEXT(hwnd) \
    do{\
        SendMessage(hwnd, EM_SETSEL, 0, -1);\
        SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)TEXT(""));\
    }while(0)

// 获取ComboBox当前选择的文本
#define GET_COMBOBOX_CURTEXT(hwnd, curText) \
    do{\
        INT curIndex = SendMessage(hwnd, CB_GETCURSEL, NULL, NULL);\
        SendMessage(hwnd, CB_GETLBTEXT, curIndex, (LPARAM)(curText.c_str()));\
    }while(0)

// 新增文本至多行编辑控件尾部
#define APPEND_EDIT_DISPLAY(hwnd, str) \
    do{\
        SendMessage(hwnd, EM_SETSEL, -2, -1); \
        SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)str); \
    }while(0)

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 设置字体
    hFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,\
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 600, 700, NULL, NULL, hInstance, NULL);

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
    /* WM_SIZE */
    WORD curWinWidth = 0;
    WORD curWinHeight = 0;
    RECT rect;
    POINT point;
    WORD ratio[10] = { 0 };
    LONG tempU32;
    INT ret = 0;

    switch (message)
    {
    case WM_CTLCOLORSTATIC:
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        break;
    case WM_CREATE:
        hwndGroupBoxFile = CreateWindowEx(0, TEXT("Button"), TEXT("下载文件"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_GROUPBOX_FILE), hInst, NULL);
        hwndStaticIAP = CreateWindowEx(0, TEXT("Static"), TEXT("IAP 文件："),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_STATIC_IAP), hInst, NULL);
        hwndEditIAP = CreateWindowEx(0, TEXT("Edit"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)(IDC_EDIT_IAP), hInst, NULL);
        hwndButtonIAP = CreateWindowEx(0, TEXT("Button"), TEXT("..."),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_BUTTON_IAP), hInst, NULL);
        hwndStaticAPP = CreateWindowEx(0, TEXT("Static"), TEXT("APP 文件："),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_STATIC_APP), hInst, NULL);
        hwndEditAPP = CreateWindowEx(0, TEXT("Edit"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)(IDC_EDIT_APP), hInst, NULL);
        hwndButtonAPP = CreateWindowEx(0, TEXT("Button"), TEXT("..."),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_BUTTON_APP), hInst, NULL);
        hwndGroupBoxControl = CreateWindowEx(0, TEXT("Button"), TEXT("控制"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_GROUPBOX_FILE), hInst, NULL);
        hwndStaticDevice = CreateWindowEx(0, TEXT("Static"), TEXT("设备："),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_STATIC_DEVICE), hInst, NULL);
        hwndComboBoxDevice = CreateWindowEx(0, TEXT("ComboBox"), NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_COMBOBOX_DEVICE), hInst, NULL);
        hwndStaticVer = CreateWindowEx(0, TEXT("Static"), TEXT("版本："),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_STATIC_VERSION), hInst, NULL);
        hwndComboBoxVer = CreateWindowEx(0, TEXT("ComboBox"), NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_COMBOBOX_VERSION), hInst, NULL);
        hwndStaticAppAddr = CreateWindowEx(0, TEXT("Static"), TEXT("APP起始地址："),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_STATIC_APP_ADDR), hInst, NULL);
        hwndEditAppAddr = CreateWindowEx(0, TEXT("Edit"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT | ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)(IDC_EDIT_APP_ADDR), hInst, NULL);
        hwndButtonDownload = CreateWindowEx(0, TEXT("Button"), TEXT("烧录"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd,
            (HMENU)(IDC_BUTTON_DOWNLOAD), hInst, NULL);
        hwndEditDisplay = CreateWindowEx(0, TEXT("Edit"), TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_AUTOVSCROLL |
            ES_LEFT | ES_READONLY | ES_MULTILINE,
            0, 0, 0, 0, hWnd, (HMENU)(IDC_EDIT_DISPLAY), hInst, NULL);
        // 设置子窗口控件字体
        SendMessage(hwndGroupBoxFile, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndStaticIAP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditIAP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndButtonIAP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndStaticAPP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditAPP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndButtonAPP, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndGroupBoxControl, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndStaticDevice, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndComboBoxDevice, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndStaticVer, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndComboBoxVer, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndStaticAppAddr, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditAppAddr, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndButtonDownload, WM_SETFONT, (WPARAM)hFont, FALSE);
        SendMessage(hwndEditDisplay, WM_SETFONT, (WPARAM)hFont, FALSE);
        // 获取烧录设置
        ret = enumDownloadSettings(downloadSetList);
        if (-1 == ret) {
            ERROR_HINT(hWnd, "settings文件不存在！");
        }
        else if (-2 == ret) {
            ERROR_HINT(hWnd, "settings文件内容为空！");
        }
        else {
            loadComboBoxDevice();
            loadComboBoxVersion();
            loadDownloadEditInfo();
        }
        // 屏蔽文件选择按钮
        EnableWindow(hwndButtonIAP, FALSE);
        EnableWindow(hwndButtonAPP, FALSE);
        break;
    case WM_SIZE:
        curWinWidth = LOWORD(lParam);
        curWinHeight = HIWORD(lParam);
        // GroupBoxFile
        SetWindowPos(hwndGroupBoxFile, NULL, WIN_OFFSET_X, WIN_OFFSET_Y,
            (curWinWidth - 2 * WIN_OFFSET_X), WIN_GROUPBOX_FILE_HEIGHT, SWP_NOZORDER);
        // 确定GroupBoxFile的坐标
        GetWindowRect(hwndGroupBoxFile, &rect);
        point.x = rect.left;
        point.y = rect.top;
        ScreenToClient(hWnd, &point);
        rect.left = point.x;
        rect.top = point.y;
        point.x = rect.right;
        point.y = rect.bottom;
        ScreenToClient(hWnd, &point);
        rect.right = point.x;
        rect.bottom = point.y;
        // 设置GroupBoxFile中的控件位置
        SetWindowPos(hwndStaticIAP, NULL, rect.left + WIN_STATIC_IAP_OFFSET_X, 
            rect.top + (rect.bottom - rect.top-2* WIN_STATIC_HEIGHT)/3,
            WIN_STATIC_IAP_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        ratio[0] = 14; ratio[1] = 1;
        tempU32 = (rect.right - rect.left - WIN_STATIC_IAP_WIDTH - \
            WIN_STATIC_IAP_OFFSET_X * 2);
        SetWindowPos(hwndEditIAP, NULL,
            rect.left + WIN_STATIC_IAP_OFFSET_X + WIN_STATIC_IAP_WIDTH,
            rect.top + (rect.bottom - rect.top - 2 * WIN_STATIC_HEIGHT) / 3,
            tempU32 / (ratio[0] + ratio[1]) * ratio[0], WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndButtonIAP, NULL,
            rect.right - WIN_STATIC_IAP_OFFSET_X - tempU32 + \
            tempU32 / (ratio[0] + ratio[1]) * ratio[0],
            rect.top + (rect.bottom - rect.top - 2 * WIN_STATIC_HEIGHT) / 3,
            tempU32 - tempU32 / (ratio[0] + ratio[1]) * ratio[0],
            WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndStaticAPP, NULL, rect.left + WIN_STATIC_IAP_OFFSET_X,
            rect.bottom - (rect.bottom - rect.top - 2 * WIN_STATIC_HEIGHT) / 3 - \
            WIN_STATIC_HEIGHT, WIN_STATIC_IAP_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndEditAPP, NULL,
            rect.left + WIN_STATIC_IAP_OFFSET_X + WIN_STATIC_IAP_WIDTH,
            rect.bottom - (rect.bottom - rect.top - 2 * WIN_STATIC_HEIGHT) / 3 - \
            WIN_STATIC_HEIGHT, tempU32 / (ratio[0] + ratio[1]) * ratio[0],\
            WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndButtonAPP, NULL,
            rect.right - WIN_STATIC_IAP_OFFSET_X - tempU32 + \
            tempU32 / (ratio[0] + ratio[1]) * ratio[0],
            rect.bottom - (rect.bottom - rect.top - 2 * WIN_STATIC_HEIGHT) / 3 - \
            WIN_STATIC_HEIGHT, tempU32 - tempU32 / (ratio[0] + ratio[1]) * ratio[0],
            WIN_STATIC_HEIGHT, SWP_NOZORDER);
        // GroupBoxControl
        SetWindowPos(hwndGroupBoxControl, NULL, WIN_OFFSET_X,
            WIN_OFFSET_Y * 2 + WIN_GROUPBOX_FILE_HEIGHT,
            (curWinWidth - 2 * WIN_OFFSET_X), WIN_GROUPBOX_CTL_HEIGHT, SWP_NOZORDER);
        // 确定GroupBoxControl的坐标
        GetWindowRect(hwndGroupBoxControl, &rect);
        point.x = rect.left;
        point.y = rect.top;
        ScreenToClient(hWnd, &point);
        rect.left = point.x;
        rect.top = point.y;
        point.x = rect.right;
        point.y = rect.bottom;
        ScreenToClient(hWnd, &point);
        rect.right = point.x;
        rect.bottom = point.y;
        // 设置GroupBoxControl中的控件位置
        SetWindowPos(hwndStaticDevice, NULL, rect.left + WIN_STATIC_IAP_OFFSET_X,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            WIN_STATIC_DEV_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        tempU32 = rect.right - rect.left - 2 * WIN_STATIC_IAP_OFFSET_X - 10 * WIN_STATIC_DEV_WIDTH;
        SetWindowPos(hwndComboBoxDevice, NULL,
            rect.left + WIN_STATIC_IAP_OFFSET_X + WIN_STATIC_DEV_WIDTH,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            tempU32, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndStaticVer, NULL,
            rect.left + 2 * WIN_STATIC_IAP_OFFSET_X + WIN_STATIC_DEV_WIDTH + tempU32,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            WIN_STATIC_DEV_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndComboBoxVer, NULL,
            rect.left + 2 * WIN_STATIC_IAP_OFFSET_X + 2 * WIN_STATIC_DEV_WIDTH + tempU32,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            2 * WIN_STATIC_DEV_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndStaticAppAddr, NULL,
            rect.left + 3 * WIN_STATIC_IAP_OFFSET_X + 4 * WIN_STATIC_DEV_WIDTH + tempU32,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            2 * WIN_STATIC_DEV_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndEditAppAddr, NULL,
            rect.left + 3 * WIN_STATIC_IAP_OFFSET_X + 6 * WIN_STATIC_DEV_WIDTH + tempU32,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            2 * WIN_STATIC_DEV_WIDTH, WIN_STATIC_HEIGHT, SWP_NOZORDER);
        SetWindowPos(hwndButtonDownload, NULL,
            rect.left + 4 * WIN_STATIC_IAP_OFFSET_X + 8 * WIN_STATIC_DEV_WIDTH + tempU32,
            rect.top + (rect.bottom - rect.top - WIN_STATIC_HEIGHT) / 2,
            rect.right - rect.left - 5 * WIN_STATIC_IAP_OFFSET_X - 8 * WIN_STATIC_DEV_WIDTH - tempU32,
            WIN_STATIC_HEIGHT, SWP_NOZORDER);
        // edit display
        tempU32 = WIN_OFFSET_Y * 3 + WIN_GROUPBOX_FILE_HEIGHT + WIN_GROUPBOX_CTL_HEIGHT;
        SetWindowPos(hwndEditDisplay, NULL, WIN_OFFSET_X, tempU32,
            (curWinWidth - 2 * WIN_OFFSET_X),
            curWinHeight - tempU32 - WIN_OFFSET_Y, SWP_NOZORDER);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            INT notifiId = HIWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_COMBOBOX_DEVICE:
                switch (notifiId) {
                case CBN_SELCHANGE:
                    loadComboBoxVersion();
                    loadDownloadEditInfo();
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
                break;
            case IDC_COMBOBOX_VERSION:
                switch (notifiId) {
                case CBN_SELCHANGE:
                    loadDownloadEditInfo();
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
                break;
            case IDC_BUTTON_DOWNLOAD:
                EnableWindow(hwndButtonDownload, FALSE);
                downloadProc();
                EnableWindow(hwndButtonDownload, TRUE);
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
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
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

/*
ret:    0,      success
        -1,     setting file not exist
        -2,     setting file empty
*/
INT enumDownloadSettings(vector<downloadSetStruct> & setlist) {
    char strBuf[500];
    ifstream infile;
    infile.open("./settings.csv", ios::in | ios::_Nocreate);
    if (!infile) { return -1; }

    infile.getline(strBuf, 500);
    while (!infile.eof()) {
        downloadSetStruct settingItem;
        infile.getline(strBuf, 500);
        string tempStr(strBuf);
        if (tempStr.size() == 0) {
            break;
        }
        INT i = 0, pos = 0;
        while (tempStr.find(',') != string::npos) {
            pos = tempStr.find(',');
            switch (i) {
            case 0:
                settingItem.deviceName = tempStr.substr(0, pos);
                break;
            case 1:
                settingItem.version = tempStr.substr(0, pos);
                break;
            case 2:
                settingItem.iapFilePath = tempStr.substr(0, pos);
                break;
            case 3:
                settingItem.appFilePath = tempStr.substr(0, pos);
                break;
            case 4:
                settingItem.appAddrS = tempStr.substr(0, pos);
                break;
            }
            tempStr = tempStr.substr(pos + 1, tempStr.size() - pos);
            i++;
        }
        settingItem.downloadToolSet = tempStr;
        setlist.push_back(settingItem);
    }

    infile.close();

    if (setlist.size() == 0) {
        return -2;
    }
    return 0;
}

static wstring stringToWstring(string str)
{
    LPCSTR pszSrc = str.c_str();
    INT nLen = MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, NULL, 0);
    if (nLen == 0) {
        return wstring(L"");
    }

    wchar_t* pwszDst = new wchar_t[nLen];
    if (!pwszDst) {
        return wstring(L"");
    }

    MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, pwszDst, nLen);
    wstring wstr(pwszDst);
    delete[] pwszDst;
    pwszDst = NULL;

    return wstr;
}

VOID loadComboBoxDevice(VOID) {
    // 清空列表项
    SendMessage(hwndComboBoxDevice, CB_RESETCONTENT, NULL, NULL);
    // 导入列表项
    wstring tempWstrBuf;
    for (UINT i = 0; i < downloadSetList.size(); i++) {
        tempWstrBuf = stringToWstring(downloadSetList[i].deviceName);
        if (-1 == SendMessage(hwndComboBoxDevice, CB_FINDSTRINGEXACT, -1,\
            (LPARAM)(tempWstrBuf.c_str()))) {
            SendMessage(hwndComboBoxDevice, CB_ADDSTRING, NULL,\
                (LPARAM)(tempWstrBuf.c_str()));
        }
    }
    // 选中第一项
    SendMessage(hwndComboBoxDevice, CB_SETCURSEL, 0, NULL);
}

VOID loadComboBoxVersion(VOID) {
    // 清空列表项
    SendMessage(hwndComboBoxVer, CB_RESETCONTENT, NULL, NULL);
    // 获取ComboBoxDevice当前选择的文本
    wstring curDevice;
    GET_COMBOBOX_CURTEXT(hwndComboBoxDevice, curDevice);
    // 导入列表项
    wstring tempWstrBuf;
    for (UINT i = 0; i < downloadSetList.size(); i++) {
        tempWstrBuf = stringToWstring(downloadSetList[i].deviceName);
        if (0 == _tcscmp(curDevice.c_str(), tempWstrBuf.c_str())) {
            tempWstrBuf = stringToWstring(downloadSetList[i].version);
            if (-1 == SendMessage(hwndComboBoxVer, CB_FINDSTRINGEXACT, -1,\
                (LPARAM)(tempWstrBuf.c_str()))) {
                SendMessage(hwndComboBoxVer, CB_ADDSTRING, NULL, \
                    (LPARAM)(tempWstrBuf.c_str()));
            }
        }
    }
    // 选中第一项
    SendMessage(hwndComboBoxVer, CB_SETCURSEL, 0, NULL);
}

VOID loadDownloadEditInfo(VOID) {
    // 清空文本
    CLEAR_EDIT_TEXT(hwndEditIAP);
    CLEAR_EDIT_TEXT(hwndEditAPP);
    CLEAR_EDIT_TEXT(hwndEditAppAddr);
    // 获取ComboBox当前选择的文本
    wstring curDevice, curVer;
    GET_COMBOBOX_CURTEXT(hwndComboBoxDevice, curDevice);
    GET_COMBOBOX_CURTEXT(hwndComboBoxVer, curVer);
    // 填充文本
    wstring tempWstrBuf;
    UINT i = 0;
    for (i = 0; i < downloadSetList.size(); i++) {
        tempWstrBuf = stringToWstring(downloadSetList[i].deviceName);
        if (0 == _tcscmp(curDevice.c_str(), tempWstrBuf.c_str())) {
            tempWstrBuf = stringToWstring(downloadSetList[i].version);
            if (0 == _tcscmp(curVer.c_str(), tempWstrBuf.c_str())) {
                break;
            }
        }
    }
    SendMessage(hwndEditIAP, WM_SETTEXT, NULL, \
        (LPARAM)(stringToWstring(downloadSetList[i].iapFilePath).c_str()));
    SendMessage(hwndEditAPP, WM_SETTEXT, NULL, \
        (LPARAM)(stringToWstring(downloadSetList[i].appFilePath).c_str()));
    SendMessage(hwndEditAppAddr, WM_SETTEXT, NULL, \
        (LPARAM)(stringToWstring(downloadSetList[i].appAddrS).c_str()));
}

/*
ret:    0,      success
        -1,     file1 not exist
        -2,     file2 not exist
*/
static INT binFileMerge(string file1, string file2, string mergeFile, string file2Addr) {
    // 复制file1
    FILE *fp1 = NULL;
    fopen_s(&fp1, file1.c_str(), "rb");
    if (fp1 == NULL) {
        return -1;
    }
    FILE *mfp = NULL;
    fopen_s(&mfp, mergeFile.c_str(), "wb");
    INT count = 0;
    while (!feof(fp1)) {
        fputc(fgetc(fp1), mfp);
        count++;
    }
    // 地址转化
    INT offset = 0;
    sscanf_s(file2Addr.c_str(), "0x%X", &offset);
    offset -= 0x08000000;
    // 填充0xFF
    for (INT i = 0; i < (offset - count); i++) {
        fputc(0xFF, mfp);
    }
    // 复制file2
    FILE *fp2 = NULL;
    fopen_s(&fp2, file2.c_str(), "rb");
    if (fp2 == NULL) {
        return -2;
    }
    while (!feof(fp2)) {
        fputc(fgetc(fp2), mfp);
    }
    // 关闭文件
    fclose(fp1);
    fclose(fp2);
    fclose(mfp);
    return 0;
}

/*
ret:    0,      success
        -1,     setting file not exist
        -2,     setting file empty
*/
static INT getJLinkSet(string setFile, JLinkSetStruct & jlinkSet) {
    char strBuf[500];
    ifstream infile;
    infile.open(setFile.c_str(), ios::in | ios::_Nocreate);
    if (!infile) { return -1; }

    infile.getline(strBuf, 500);
    infile.getline(strBuf, 500);
    string tempStr(strBuf);
    if (tempStr.size() == 0) {
        return -2;
    }

    INT i = 0, pos = 0;
    while (tempStr.find(',') != string::npos) {
        pos = tempStr.find(',');
        switch (i) {
        case 0:
            jlinkSet.toolPath = tempStr.substr(0, pos);
            break;
        case 1:
            jlinkSet.device = tempStr.substr(0, pos);
            break;
        case 2:
            jlinkSet.interface = tempStr.substr(0, pos);
            break;
        }
        tempStr = tempStr.substr(pos + 1, tempStr.size() - pos);
        i++;
    }
    jlinkSet.speed = tempStr;

    infile.close();
    return 0;
}

VOID downloadProc(VOID) {
    // 清空编辑框文本
    CLEAR_EDIT_TEXT(hwndEditDisplay);
    // 建立缓存文件夹cache
    string path = ".\\cache\\";
    if (_access(path.c_str(), 0) == -1) {
        _mkdir(path.c_str());
    }
    // 获取app、iap文件地址和app烧录地址信息
    wstring curDevice, curVer;
    GET_COMBOBOX_CURTEXT(hwndComboBoxDevice, curDevice);
    GET_COMBOBOX_CURTEXT(hwndComboBoxVer, curVer);
    wstring tempWstrBuf;
    UINT i = 0;
    for (i = 0; i < downloadSetList.size(); i++) {
        tempWstrBuf = stringToWstring(downloadSetList[i].deviceName);
        if (0 == _tcscmp(curDevice.c_str(), tempWstrBuf.c_str())) {
            tempWstrBuf = stringToWstring(downloadSetList[i].version);
            if (0 == _tcscmp(curVer.c_str(), tempWstrBuf.c_str())) {
                break;
            }
        }
    }

    TCHAR displayText[200] = { 0 };

    // 合并iap和app文件
    wsprintf(displayText, TEXT("IAP: %s\r\n"),\
        stringToWstring(downloadSetList[i].iapFilePath).c_str());
    APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
    wsprintf(displayText, TEXT("APP: %s\r\n"), \
        stringToWstring(downloadSetList[i].appFilePath).c_str());
    APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
    APPEND_EDIT_DISPLAY(hwndEditDisplay, TEXT("合并中 ...\r\n"));
    INT ret = binFileMerge(downloadSetList[i].iapFilePath,\
                           downloadSetList[i].appFilePath,\
                           ".\\cache\\mergeFile.bin",\
                           downloadSetList[i].appAddrS);
    if (ret == -1) {
        wsprintf(displayText, TEXT("[错误]文件： %s 不存在\r\n"), \
            stringToWstring(downloadSetList[i].iapFilePath).c_str());
        APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
    }
    else if (ret == -2) {
        wsprintf(displayText, TEXT("[错误]文件： %s 不存在\r\n"), \
            stringToWstring(downloadSetList[i].appFilePath).c_str());
        APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
    }
    else {
        APPEND_EDIT_DISPLAY(hwndEditDisplay,\
            TEXT("[成功]合并为文件： .\\cache\\mergeFile.bin\r\n"));
    }

    // 确认烧录工具设置
    UINT curDownloadTool = 0;
    string curDownloadToolPath;
    if (downloadSetList[i].downloadToolSet.find("JLink") != string::npos) {
        curDownloadTool = JLINK;
        // 获取JLink配置
        APPEND_EDIT_DISPLAY(hwndEditDisplay, TEXT("获取JLink配置 ...\r\n"));
        JLinkSetStruct jlinkSetting;
        INT ret = getJLinkSet(downloadSetList[i].downloadToolSet, jlinkSetting);
        if (-1 == ret) {
            wsprintf(displayText, TEXT("[错误]文件： %s 不存在\r\n"), \
                stringToWstring(downloadSetList[i].downloadToolSet).c_str());
            APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
            return;
        }
        else if (-2 == ret) {
            wsprintf(displayText, TEXT("[错误]文件： %s 内容为空\r\n"), \
                stringToWstring(downloadSetList[i].downloadToolSet).c_str());
            APPEND_EDIT_DISPLAY(hwndEditDisplay, displayText);
            return;
        }
        else {
            curDownloadToolPath = jlinkSetting.toolPath;
            // 新建jlink配置文件
            // ref: https://www.cnblogs.com/bh4lm/p/6733232.html
            // ref: https://wiki.segger.com/index.php?title=J-Link_Commander#Batch_processing
            ofstream outfile;
            outfile.open(".\\cache\\CommandFile.jlink", ios::out | ios::trunc);
            outfile << "SelectInterface " << jlinkSetting.interface.c_str() << endl;
            outfile << "Speed " << jlinkSetting.speed.c_str() << endl;
            outfile << "Device " << jlinkSetting.device.c_str() << endl;
            outfile << "R" << endl;
            outfile << "H" << endl;
            outfile << "Erase" << endl;
            outfile << "LoadFile .\\cache\\mergeFile.bin 0x08000000" << endl;
            outfile << "VerifyBin .\\cache\\mergeFile.bin 0x08000000" << endl;
            outfile << "G" << endl;
            outfile << "Exit" << endl;
            outfile.close();
            APPEND_EDIT_DISPLAY(hwndEditDisplay,\
                TEXT("[成功]JLink配置文件建立： .\\cache\\CommandFile.jlink\r\n"));
        }
    }

    // 执行烧录
    if (JLINK == curDownloadTool) {
#if 1
        wstring exeFile = stringToWstring(curDownloadToolPath);
        SHELLEXECUTEINFO shExecInfo = { 0 };
        shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
        shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExecInfo.hwnd = hwndEditDisplay;
        shExecInfo.lpVerb = TEXT("open");
        shExecInfo.lpFile = exeFile.c_str();
        shExecInfo.lpParameters = TEXT("-CommandFile .\\cache\\CommandFile.jlink");
        shExecInfo.lpDirectory = NULL;
        shExecInfo.nShow = SW_SHOW;
        shExecInfo.hInstApp = NULL;
        ShellExecuteEx(&shExecInfo);
        WaitForSingleObject(shExecInfo.hProcess, INFINITE);
#else
        ShellExecute(hwndEditDisplay, TEXT("open"),\
            stringToWstring(curDownloadToolPath).c_str(),\
            TEXT("-CommandFile .\\cache\\CommandFile.jlink"),\
            NULL, SW_SHOW);
#endif
    }

    APPEND_EDIT_DISPLAY(hwndEditDisplay, TEXT("[成功]烧录完成\r\n"));
    // 删除文件
    DeleteFile(TEXT(".\\cache\\mergeFile.bin"));
    DeleteFile(TEXT(".\\cache\\CommandFile.jlink"));
}
