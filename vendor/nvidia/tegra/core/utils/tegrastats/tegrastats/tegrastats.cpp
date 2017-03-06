/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "stdafx.h"
#include "resourceppc.h"
#include "nvrm_power.h"
#include "nvrm_memmgr.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE			g_hInst;			// current instance
NvRmDeviceHandle    g_rm = 0;

// Forward declarations of functions included in this code module:
ATOM			    MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			    InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

static const TCHAR* NvFormatStringToTChar(int& len, const char* fmt, ...)
{
    static TCHAR NvStringToTCharBuf[1024];
    int i;
    char str[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(str, fmt, args);
    va_end(args);

    i = 0;
    while (str[i] && i < 1024 - 1)
    {
        NvStringToTCharBuf[i] = str[i];
        i++;
    }
    len = i;
    NvStringToTCharBuf[i] = 0;
    return NvStringToTCharBuf;
}

//
void CALLBACK TimerProc(HWND hWnd, UINT uMsg,UINT idEvent, DWORD dwTime) 
{
    MEMORYSTATUS mem;
    memset(&mem, 0, sizeof(MEMORYSTATUS));
    mem.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&mem);

    int totalCarveout = 0;
    NvRmMemGetStat(NvRmMemStat_TotalCarveout, &totalCarveout);
    int usedCarveout = 0;
    NvRmMemGetStat(NvRmMemStat_UsedCarveout, &usedCarveout);
    int largestFreeCarveoutBlock = 0;
    NvRmMemGetStat(NvRmMemStat_LargestFreeCarveoutBlock, &largestFreeCarveoutBlock);
    int totalGart = 0;
    NvRmMemGetStat(NvRmMemStat_TotalGart, &totalGart);
    int usedGart = 0;
    NvRmMemGetStat(NvRmMemStat_UsedGart, &usedGart);
    int largestFreeGartBlock = 0;
    NvRmMemGetStat(NvRmMemStat_LargestFreeGartBlock, &largestFreeGartBlock);

	HDC mdc = GetDC(hWnd);
    HFONT hFont = (HFONT)GetStockObject(SYSTEM_FONT); 
    SelectObject(mdc, hFont);
    int len;

    /* Clear the client rect */
   	RECT target;
    GetClientRect(hWnd, &target);
    HBRUSH solidBrush = CreateSolidBrush(0xffffff);
    FillRect(mdc, &target, solidBrush);
    DeleteObject(solidBrush);

    //memory
    const TCHAR* tmsg = NvFormatStringToTChar(len, 
        "sysmem[%7d/%7dkB] carveout[%6d/%6dkB, lfb %6dkB] GART[%5d/%5dkB, lfb %5dkB]", 
        (mem.dwTotalPhys-mem.dwAvailPhys)>>10, mem.dwTotalPhys>>10, 
        usedCarveout>>10, totalCarveout>>10, largestFreeCarveoutBlock>>10,
        usedGart>>10, totalGart>>10, largestFreeGartBlock>>10);
    ExtTextOut(mdc, 1, 0, 0, NULL, tmsg, len, NULL); 

    //clocks
    if(g_rm)
    {
        NvRmDfsClockUsage ClocksUsage[NvRmDfsClockId_Num];
        memset(ClocksUsage, 0, sizeof(NvRmDfsClockUsage)*NvRmDfsClockId_Num);
        for(int i=0;i<NvRmDfsClockId_Num;i++)
            NvRmDfsGetClockUtilization(g_rm, (NvRmDfsClockId)i, &ClocksUsage[i]);
        const TCHAR* tmsg2 = NvFormatStringToTChar(len, 
            "CPU[%3d%%/%4dMHz] AVP[%3d%%/%3dMHz] EMC[%3d%%/%3dMHz]", 
            ClocksUsage[NvRmDfsClockId_Cpu].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Cpu].CurrentKHz, ClocksUsage[NvRmDfsClockId_Cpu].CurrentKHz/1000, 
            ClocksUsage[NvRmDfsClockId_Avp].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Avp].CurrentKHz, ClocksUsage[NvRmDfsClockId_Avp].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_Emc].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Emc].CurrentKHz, ClocksUsage[NvRmDfsClockId_Emc].CurrentKHz/1000
            );
    /*    const TCHAR* tmsg2 = NvFormatStringToTChar(len, 
            "CPU[%3d%%/%4d] AVP[%3d%%/%3d] Sysbus[%3d] AHB[%3d%%/%3d] APB[%3d%%/%3d] VPipe[%3d%%/%3d] EMC[%3d%%/%3d]", 
            ClocksUsage[NvRmDfsClockId_Cpu].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Cpu].CurrentKHz, ClocksUsage[NvRmDfsClockId_Cpu].CurrentKHz/1000, 
            ClocksUsage[NvRmDfsClockId_Avp].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Avp].CurrentKHz, ClocksUsage[NvRmDfsClockId_Avp].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_System].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_Ahb].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Ahb].CurrentKHz, ClocksUsage[NvRmDfsClockId_Ahb].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_Apb].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Apb].CurrentKHz, ClocksUsage[NvRmDfsClockId_Apb].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_Vpipe].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Vpipe].CurrentKHz, ClocksUsage[NvRmDfsClockId_Vpipe].CurrentKHz/1000,
            ClocksUsage[NvRmDfsClockId_Emc].AverageKHz*100/ClocksUsage[NvRmDfsClockId_Emc].CurrentKHz, ClocksUsage[NvRmDfsClockId_Emc].CurrentKHz/1000
            );
    */
        ExtTextOut(mdc, 1, 14, 0, NULL, tmsg2, len, NULL); 
    }

    ReleaseDC(hWnd, mdc);
}


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
	MSG msg;

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	HACCEL hAccelTable;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEGRASTATS));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    NvRmClose( g_rm );

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEGRASTATS));
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[MAX_LOADSTRING];		// title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];	// main window class name

    g_hInst = hInstance; // Store instance handle in our global variable

    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the device specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING); 
    LoadString(hInstance, IDC_TEGRASTATS, szWindowClass, MAX_LOADSTRING);

    //If it is already running, then focus on the window, and exit
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
        // set focus to foremost child window
        // The "| 0x00000001" is used to bring any owned windows to the foreground and
        // activate them.
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        return 0;
    } 

    if (!MyRegisterClass(hInstance, szWindowClass))
    {
    	return FALSE;
    }

    NvRmOpen( &g_rm, 0 );

    hWnd = CreateWindowEx(WS_EX_TOPMOST, szWindowClass, szTitle, WS_POPUP|WS_CAPTION|WS_SYSMENU, 0, 0, 600, 53, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	if (!SetTimer(hWnd, 2, 500, TimerProc)) 
	{
		return FALSE;
	}

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    static SHACTIVATEINFO s_sai;
	
    switch (message) 
    {
        case WM_COMMAND:
            break;
        case WM_CREATE:
            // Initialize the shell activate info structure
            memset(&s_sai, 0, sizeof (s_sai));
            s_sai.cbSize = sizeof (s_sai);
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            TimerProc(hWnd, 0, 0, 0); 
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_ACTIVATE:
            // Notify shell of our activate message
            SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
            break;
        case WM_SETTINGCHANGE:
            SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
