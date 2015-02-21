#include <Windows.h>
#include "Image.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_LAYER_LIST		0
#define ID_START_BUTTON		1
#define ID_SAVE_BUTTON		2

struct Image{
	unsigned short width;
	unsigned short height;
	PIXEL *data;
};

LPCWSTR lpszClass = L"AokanaCGExtractor";
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HWND FindAokana(LPDWORD lpdwThreadId, LPDWORD lpdwProcessId = NULL);
int saveImage(Image &img, WCHAR *pszFileName);
BOOL saveDialog(HWND hWnd, Image &img);
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
DWORD WINAPI DebugThread(LPVOID arg);

int APIENTRY WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	WNDCLASS wndclass;
	RECT rt;

	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hInstance = hInstance;
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = lpszClass;
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClass(&wndclass);

	SetRect(&rt, 0, 0, 300, 700);
	AdjustWindowRect(&rt, WS_CAPTION | WS_SYSMENU, FALSE);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, rt.right - rt.left, rt.bottom - rt.top, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&msg, 0, 0, 0))
	{
		DispatchMessage(&msg);
		TranslateMessage(&msg);
	}

	return msg.wParam;
}

DWORD dwThreadID;
DWORD dwProcessID;
HWND hAokanaWnd;

HWND hButtonStart;
HWND hButtonSave;
HWND hListLayer;

HFONT hFont;

ImgViewer viewer;

BOOL bStarted;

HANDLE hAttachSucceeded;
HANDLE hDebugInit;
HANDLE hDebugEnd;
HANDLE hDebugThread;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	int nItemCount;
	int nSelected;
	int *nIndexes;
	Image *imgCurrent;
	Image out;

	switch (iMessage)
	{
		case WM_CREATE:
			HANDLE hToken;

			viewer.changeCaption(L"Preview");

			//Create Controls
			hListLayer = CreateWindow(L"ListBox", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_EXTENDEDSEL | LBS_HASSTRINGS | LBS_NOTIFY, 0, 25, 300, 675, hWnd, (HMENU)ID_LAYER_LIST, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
			hButtonStart = CreateWindow(L"Button", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 25, hWnd, (HMENU)ID_START_BUTTON, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
			hButtonSave = CreateWindow(L"Button", L"Save Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 100, 0, 200, 25, hWnd, (HMENU)ID_SAVE_BUTTON, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			hFont = CreateFont(20, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, FF_ROMAN, L"Segoe UI");

			SendMessage(hListLayer, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hButtonStart, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hButtonSave, WM_SETFONT, (WPARAM)hFont, TRUE);

			//Create Events
			hAttachSucceeded = CreateEvent(NULL, TRUE, FALSE, NULL);
			hDebugEnd = CreateEvent(NULL, TRUE, FALSE, NULL);
			hDebugInit = CreateEvent(NULL, TRUE, FALSE, NULL);

			//Adjust Privileges
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			{
				if (SetPrivilege(hToken, SE_DEBUG_NAME, TRUE))
					return 0;
				else
					MessageBox(hWnd, L"Fail to get debug privilege!!", L"Error", MB_OK | MB_ICONERROR);
			}
			else
				MessageBox(hWnd, L"Fail to get process token!!", L"Error", MB_OK | MB_ICONERROR);

			SendMessage(hWnd, WM_DESTROY, 0, 0);

			return 0;
		case WM_ACTIVATE:
			if (wParam == WA_CLICKACTIVE)
			{
				viewer.foreground();
				SetForegroundWindow(hWnd);
				SetFocus(hListLayer);
			}
			return 0;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_START_BUTTON:
					if (!bStarted)
					{
						hAokanaWnd = FindAokana(&dwThreadID, &dwProcessID);

						if (dwThreadID != 0)
						{
							hDebugThread = CreateThread(NULL, 0, DebugThread, NULL, 0, NULL);

							WaitForSingleObject(hDebugInit, INFINITE);
							if (WaitForSingleObject(hAttachSucceeded, 0) != WAIT_OBJECT_0)
							{
								SetEvent(hDebugEnd);
								MessageBox(hWnd, L"Fail to attach process!!", L"Error", MB_OK | MB_ICONERROR);

								break;
							}

							SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Stop");
							bStarted = TRUE;
						}
					}
					else
					{
						SetEvent(hDebugEnd);
						WaitForSingleObject(hDebugThread, INFINITE);

						ResetEvent(hDebugEnd);
						ResetEvent(hDebugInit);
						ResetEvent(hAttachSucceeded);
						CloseHandle(hDebugThread);

						SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Start");
						bStarted = FALSE;
					}
					break;
				case ID_SAVE_BUTTON:
					nItemCount = SendMessage(hListLayer, LB_GETCOUNT, 0, 0);
					nSelected = SendMessage(hListLayer, LB_GETSELCOUNT, 0, 0);

					if (nSelected > 0)
					{
						nIndexes = (int *)calloc(nSelected, sizeof(int));

						SendMessage(hListLayer, LB_GETSELITEMS, nSelected, (LPARAM)nIndexes);

						for (int i = nSelected - 1; i >= 0; i--)
						{
							imgCurrent = (Image *)SendMessage(hListLayer, LB_GETITEMDATA, nIndexes[i], 0);

							if (imgCurrent == NULL)
								continue;

							if (i == nSelected - 1)
							{
								out.data = (PIXEL *)calloc(imgCurrent->width * imgCurrent->height, sizeof(PIXEL));
								out.width = imgCurrent->width;
								out.height = imgCurrent->height;
							}

							if (imgCurrent->width == out.width && imgCurrent->height == out.height)
								pixeloverlay(out.width, out.height, out.data, imgCurrent->data, out.data);
						}

						saveDialog(hWnd, out);

						free(nIndexes);
					}
					break;
				case ID_LAYER_LIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						nItemCount = SendMessage(hListLayer, LB_GETCOUNT, 0, 0);
						nSelected = SendMessage(hListLayer, LB_GETSELCOUNT, 0, 0);

						if (nSelected > 0)
						{
							nIndexes = (int *)calloc(nSelected, sizeof(int));

							SendMessage(hListLayer, LB_GETSELITEMS, nSelected, (LPARAM)nIndexes);

							for (int i = nSelected - 1; i >= 0; i--)
							{
								imgCurrent = (Image *)SendMessage(hListLayer, LB_GETITEMDATA, nIndexes[i], 0);

								if (imgCurrent == NULL)
									continue;

								if (i == nSelected - 1)
									viewer.create(imgCurrent->width, imgCurrent->height);

								if (imgCurrent->width == viewer.getwidth() && imgCurrent->height == viewer.getheight())
									viewer.overlay(imgCurrent->data);
							}

							viewer.show();

							free(nIndexes);
						}
						else
							viewer.hide();
					}
					break;
			}
			return 0;
		case WM_DESTROY:
			if (bStarted)
				SendMessage(hWnd, WM_COMMAND, ID_START_BUTTON, 0);

			nItemCount = SendMessage(hListLayer, LB_GETCOUNT, 0, 0);
			for (int i = 0; i < nItemCount; i++)
			{
				imgCurrent = (Image *)SendMessage(hListLayer, LB_GETITEMDATA, i, 0);
				free(imgCurrent->data);
				free(imgCurrent);
			}

			CloseHandle(hAttachSucceeded);
			CloseHandle(hDebugEnd);

			DeleteObject(hFont);

			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

HWND FindAokana(LPDWORD lpdwThreadId, LPDWORD lpdwProcessId)
{
	HWND hWnd;

	hWnd = FindWindow(L"BGI - Main window", L"蒼の彼方のフォーリズム");

	if (hWnd != NULL && lpdwThreadId != NULL)
		*lpdwThreadId = GetWindowThreadProcessId(hWnd, lpdwProcessId);

	return hWnd;
}

BOOL saveDialog(HWND hWnd, Image &img)
{
	WCHAR *pszFileName;
	BOOL bReturn = FALSE;
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };

	pszFileName = (WCHAR *)calloc(4096, sizeof(WCHAR));

	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Portable Network Graphic (*.PNG)\0*.png\0";
	ofn.lpstrFile = pszFileName;
	ofn.lpstrDefExt = L"png";
	ofn.nMaxFile = 4096;
	ofn.lpstrTitle = L"Save Image";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	if (GetSaveFileName(&ofn))
	{
		if ((ofn.Flags & OFN_EXTENSIONDIFFERENT) == OFN_EXTENSIONDIFFERENT)
			wcscat_s(pszFileName, 4096, L".png");

		saveImage(img, pszFileName);

		bReturn = TRUE;
	}

	free(pszFileName);

	return bReturn;
}

BYTE SetSoftwareBreakpoint(HANDLE hProcess, DWORD dwAddr)
{
	BYTE ret;
	BYTE int3 = 0xCC;
	DWORD dwRead;

	ReadProcessMemory(hProcess, (LPVOID)dwAddr, &ret, 1, &dwRead);
	WriteProcessMemory(hProcess, (LPVOID)dwAddr, &int3, 1, &dwRead);
	FlushInstructionCache(hProcess, (LPVOID)dwAddr, 1);

	return ret;
}

void ResetSoftwareBreakpoint(HANDLE hProcess, DWORD dwAddr, BYTE original)
{
	DWORD dwRead;

	WriteProcessMemory(hProcess, (LPVOID)dwAddr, &original, 1, &dwRead);
	FlushInstructionCache(hProcess, (LPVOID)dwAddr, 1);
}

void MoveEIPBack(HANDLE hThread)
{
	CONTEXT lcContext;

	lcContext.ContextFlags = CONTEXT_ALL;
	GetThreadContext(hThread, &lcContext);

	lcContext.Eip--;
	SetThreadContext(hThread, &lcContext);
}

DWORD WINAPI DebugThread(LPVOID arg)
{
	DEBUG_EVENT evt;
	DWORD dwContinue;
	DWORD dwBPAddress = 0x44D471;
	DWORD dwBPEnd = 0x44D47C;
	DWORD dwDataRead = 0x443848;
	DWORD dwDataSucceed = 0x44384D;
	BYTE bBPAddr = 0;
	BYTE bBPEnd = 0;
	BYTE bDataRead = 0;
	BYTE bDataSucceed = 0;
	DWORD dwRead;

	DWORD dwDataLength = 0;	//[ESP + 0x10] at 0x443848
	char szName[16];		//[ESP + 0x0C] at 0x44384D
	char szHeader[8];		//[ESP + 0x08] at 0x44384D This value should "BSE 1.0\0"

	BOOL bFlag = FALSE;
	BOOL bFlag2 = FALSE;
	BOOL bAlpha = FALSE;

	CONTEXT lcContext;
	HANDLE hThread = NULL;
	HANDLE hProcess = NULL;

	LPVOID ptr, ptr2;
	BYTE *pBMPData;
	DWORD dwLength;
	BYTE *pData;

	Image *result;
//	SYSTEMTIME time;
	WCHAR str[64];

	unsigned short width;
	unsigned short height;

	szName[0] = 0;

	if (DebugActiveProcess(dwProcessID))
		SetEvent(hAttachSucceeded);

	SetEvent(hDebugInit);

	DebugSetProcessKillOnExit(FALSE);

	while (TRUE)
	{
		if (WaitForSingleObject(hDebugEnd, 0) == WAIT_OBJECT_0)
			break;

		dwContinue = DBG_CONTINUE;

		if (WaitForDebugEvent(&evt, 10))
		{
			switch (evt.dwDebugEventCode)
			{
				case CREATE_PROCESS_DEBUG_EVENT:
					bDataRead = SetSoftwareBreakpoint(evt.u.CreateProcessInfo.hProcess, dwDataRead);

					hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, evt.dwProcessId);

					CloseHandle(evt.u.CreateProcessInfo.hFile);
					break;
				case CREATE_THREAD_DEBUG_EVENT:
					break;
				case LOAD_DLL_DEBUG_EVENT:
					CloseHandle(evt.u.LoadDll.hFile);
					break;
				case EXCEPTION_DEBUG_EVENT:
					if (evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
					{
						if (evt.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID)dwBPAddress)
						{
						//	if (bFlag)
						//	{
							//	MessageBeep(0);
								bAlpha = FALSE;

								hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, evt.dwThreadId);

								lcContext.ContextFlags = CONTEXT_ALL;
								GetThreadContext(hThread, &lcContext);

								ptr = (LPVOID)(lcContext.Eax + 0x08);
								ReadProcessMemory(hProcess, ptr, &pBMPData, 4, &dwRead);

								ReadProcessMemory(hProcess, (LPVOID)pBMPData, &width, 2, &dwRead);
								ReadProcessMemory(hProcess, (LPVOID)(pBMPData + 2), &height, 2, &dwRead);
								dwLength = width * height * 4;

								pData = (BYTE *)calloc(dwLength, sizeof(BYTE));
								if (pData)
								{
									wsprintf(str, L"[%dx%d] %S", width, height, szName);
									
									if (SendMessage(hListLayer, LB_FINDSTRINGEXACT, -1, (LPARAM)str) == LB_ERR)
									{
										SendMessage(hListLayer, LB_INSERTSTRING, 0, (LPARAM)str);

										ReadProcessMemory(hProcess, (LPVOID)(pBMPData + 0x10), pData, dwLength, &dwRead);
										result = (Image *)calloc(1, sizeof(Image));

										result->width = width;
										result->height = height;
										result->data = (PIXEL *)pData;

										//Is this bitmap has no alpha channel (all alpha channel == 0)
										for (int i = 0; i < width * height; i++)
											if (result->data[i].a != 0x00)
											{
												bAlpha = TRUE;
												break;
											}

										//Set alpha channel
										if (!bAlpha)
											for (int i = 0; i < width * height; i++)
												result->data[i].a = 0xFF;

										//Insert Decoded Image to List
										SendMessage(hListLayer, LB_SETITEMDATA, 0, (LPARAM)result);
									}
								}

								MoveEIPBack(hThread);
								bBPEnd = SetSoftwareBreakpoint(hProcess, dwBPEnd);
								ResetSoftwareBreakpoint(hProcess, dwBPAddress, bBPAddr);

								CloseHandle(hThread);
						//	}
						//	else
								bFlag = TRUE;
						}
						else if (evt.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID)dwBPEnd)
						{
						//	if (bFlag2)
						//	{
								hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, evt.dwThreadId);

								MoveEIPBack(hThread);
								bDataRead = SetSoftwareBreakpoint(hProcess, dwDataRead);
								ResetSoftwareBreakpoint(hProcess, dwBPEnd, bBPEnd);

								CloseHandle(hThread);
						//	}
						//	else
								bFlag2 = TRUE;
						}
						else if (evt.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID)dwDataRead)
						{
							hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, evt.dwThreadId);

							lcContext.ContextFlags = CONTEXT_ALL;
							GetThreadContext(hThread, &lcContext);

							ptr = (LPVOID)(lcContext.Esp + 0x10);
							ReadProcessMemory(hProcess, ptr, &dwDataLength, 4, &dwRead);

							MoveEIPBack(hThread);
							bDataSucceed = SetSoftwareBreakpoint(hProcess, dwDataSucceed);
							ResetSoftwareBreakpoint(hProcess, dwDataRead, bDataRead);

							CloseHandle(hThread);
						}
						else if (evt.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID)dwDataSucceed)
						{
							hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, evt.dwThreadId);

							MoveEIPBack(hThread);

							if (dwDataLength)
							{
								lcContext.ContextFlags = CONTEXT_ALL;
								GetThreadContext(hThread, &lcContext);

								if (lcContext.Eax == dwDataLength)
								{
									ptr = (LPVOID)(lcContext.Esp + 0x08);
									ReadProcessMemory(hProcess, ptr, &ptr2, 4, &dwRead);
									ReadProcessMemory(hProcess, ptr2, &szHeader, 8, &dwRead);

									if (strncmp(szHeader, "BSE 1.0\0", 8) == 0)
									{
										//Read Succeed
										ptr = (LPVOID)(lcContext.Esp + 0x0C);
										ReadProcessMemory(hProcess, ptr, &ptr2, 4, &dwRead);
										ReadProcessMemory(hProcess, ptr2, &szName, 16, &dwRead);

										bBPAddr = SetSoftwareBreakpoint(hProcess, dwBPAddress);
									}
									else
										bDataRead = SetSoftwareBreakpoint(hProcess, dwDataRead);
								}
								else
									bDataRead = SetSoftwareBreakpoint(hProcess, dwDataRead);
							}
							else
								bDataRead = SetSoftwareBreakpoint(hProcess, dwDataRead);

							ResetSoftwareBreakpoint(hProcess, dwDataSucceed, bDataSucceed);

							CloseHandle(hThread);
						}
					}
					else if (evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP)
					{
					}
					else
						dwContinue = DBG_EXCEPTION_NOT_HANDLED;
					break;
				case EXIT_PROCESS_DEBUG_EVENT:
					SetEvent(hDebugEnd);
					break;
				default:
					dwContinue = DBG_EXCEPTION_NOT_HANDLED;
					break;
			}

			ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, dwContinue);
		}
	}
	if (bDataRead)
		ResetSoftwareBreakpoint(hProcess, dwDataRead, bDataRead);
	if (bDataSucceed)
		ResetSoftwareBreakpoint(hProcess, dwDataSucceed, bDataSucceed);
	if (bBPAddr)
		ResetSoftwareBreakpoint(hProcess, dwBPAddress, bBPAddr);
	if (bBPEnd)
		ResetSoftwareBreakpoint(hProcess, dwBPEnd, bBPEnd);

	if (hProcess)
		CloseHandle(hProcess);

	DebugActiveProcessStop(dwProcessID);

	return 0;
}

#include "png.h"

#ifdef _DEBUG
#pragma comment(lib, "libpng16d")
#pragma comment(lib, "zlibd")
#else
#pragma comment(lib, "libpng16")
#pragma comment(lib, "zlib")
#endif

#define PNGSETJMP 	if(setjmp(png_jmpbuf(png_ptr))) \
	{ \
		png_destroy_write_struct(&png_ptr, &info_ptr); \
		return 0; \
	}

int saveImage(Image &img, WCHAR *pszFileName)
{
	int x, y;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep * row_pointers;
	FILE * fp;

	_wfopen_s(&fp, pszFileName, L"wb");

	if (fp == NULL)
		return 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	PNGSETJMP

	png_init_io(png_ptr, fp);

	PNGSETJMP

	png_set_IHDR(png_ptr, info_ptr, img.width, img.height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	/* write bytes */
	PNGSETJMP

	/* convert uint8_t[] to pngbyte[][] */
	row_pointers = (png_bytep *)malloc(sizeof(*row_pointers) * img.height);
	for (y = 0; y < img.height; y++)
	{
		row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
		for (x = 0; x < img.width; x ++)
		{
			row_pointers[y][x * 4] = img.data[y * img.width + x].r;
			row_pointers[y][x * 4 + 1] = img.data[y * img.width + x].g;
			row_pointers[y][x * 4 + 2] = img.data[y * img.width + x].b;
			row_pointers[y][x * 4 + 3] = img.data[y * img.width + x].a;
		}
	}

	png_write_image(png_ptr, row_pointers);

	/* end write */
	PNGSETJMP

	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	for (y = 0; y < img.height; y++)
	{
		free(row_pointers[y]);
	}
	free(row_pointers);

	fclose(fp);

	return 1;
}

BOOL SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
	)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
	//	printf("LookupPrivilegeValue error: %u\n", GetLastError());
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
	//	printf("AdjustTokenPrivileges error: %u\n", GetLastError());
		return FALSE;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
	//	printf("The token does not have the specified privilege. \n");
		return FALSE;
	}

	return TRUE;
}