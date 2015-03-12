#include "Image.h"

ImgViewer::ImgViewer()
{
	width = 0;
	height = 0;
	data = NULL;

	bInit = FALSE;
	bData = FALSE;
	hWindow = NULL;
	hThread = NULL;

	hThreadInit = CreateEvent(NULL, TRUE, FALSE, NULL);
	hThreadEnd = CreateEvent(NULL, TRUE, FALSE, NULL);

	hThread = CreateThread(NULL, 0, ImgViewer::callback, this, 0, NULL);
	if (hThread != INVALID_HANDLE_VALUE)
	{
		bInit = TRUE;

		WaitForSingleObject(hThreadInit, INFINITE);
		CloseHandle(hThreadInit);
	}
}

ImgViewer::~ImgViewer()
{
	if (bInit)
	{
		SendMessage(hWindow, WM_APP, 0, 0);
		WaitForSingleObject(hThreadEnd, INFINITE);
		CloseHandle(hThread);
		CloseHandle(hThreadEnd);
	}

	if (bData)
		free(data);
}

void ImgViewer::show()
{
	if (bData)
		ShowWindow(hWindow, SW_SHOWNORMAL);
}

void ImgViewer::hide()
{
	if (bInit)
		ShowWindow(hWindow, SW_HIDE);
}

void ImgViewer::changeCaption(WCHAR *pszCaption)
{
	if (bInit)
		SendMessage(hWindow, WM_SETTEXT, 0, (LPARAM)pszCaption);
}

void ImgViewer::release()
{
	if (bData)
	{
		free(data);
		bData = FALSE;
		width = 0;
		height = 0;
		data = NULL;
	}
}

bool ImgViewer::create(unsigned short width, unsigned short height)
{
	bool bReturn = false;
	RECT rt;

	release();

	if (bInit)
	{
		this->width = width;
		this->height = height;

		SetRect(&rt, 0, 0, width, height);
		AdjustWindowRectEx(&rt, WS_CAPTION, FALSE, WS_EX_TOOLWINDOW);
		SetWindowPos(hWindow, HWND_NOTOPMOST, 0, 0, rt.right - rt.left, rt.bottom - rt.top, SWP_NOACTIVATE | SWP_NOMOVE);

		data = (PIXEL *)calloc(width * height, sizeof(PIXEL));
		if (data)
		{
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
				{
					if ((i / 10) % 2 == (j / 10) % 2)
						data[j * width + i].value = 0xFFCCCCCC;
					else
						data[j * width + i].value = 0xFFFFFFFF;
				}

			bData = TRUE;

			InvalidateRect(hWindow, NULL, FALSE);
		}
		else
			MessageBox(NULL, L"Memory Allocation Failed!!", L"Error", MB_OK | MB_ICONERROR);
	}

	return bReturn;
}

unsigned short ImgViewer::getwidth()
{
	return width;
}

unsigned short ImgViewer::getheight()
{
	return height;
}

void ImgViewer::setdata(PIXEL *data)
{
	if (bData)
	{
		memcpy(this->data, data, width * height * sizeof(PIXEL));

		InvalidateRect(hWindow, NULL, FALSE);
	}
}

void ImgViewer::foreground()
{
	if (bInit)
		SetForegroundWindow(hWindow);
}

void ImgViewer::overlay(PIXEL *data)
{
	if (bData)
	{
		pixeloverlay(width, height, this->data, data, this->data);

		InvalidateRect(hWindow, NULL, FALSE);
	}
}

DWORD WINAPI ImgViewer::callback(LPVOID arg)
{
	ImgViewer *data = (ImgViewer *)arg;
	WNDCLASS wndclass;
	MSG msg;
	DWORD dwExStyle;
	WCHAR lpszClass[64];

	SetProcessDPIAware();

	wsprintf(lpszClass, L"ImgViewer - %08X", rand());

	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hInstance = NULL;
	wndclass.lpfnWndProc = wndproc;
	wndclass.lpszClassName = lpszClass;
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClass(&wndclass);

	data->hWindow = CreateWindowEx(WS_EX_TOOLWINDOW, lpszClass, lpszClass, WS_BORDER, 0, 0, 0, 0, NULL, NULL, NULL, data);

	dwExStyle = GetWindowLongPtr(data->hWindow, GWL_EXSTYLE);
	SetWindowLongPtr(data->hWindow, GWL_EXSTYLE, dwExStyle & (~WS_EX_APPWINDOW));

	SetEvent(data->hThreadInit);

	while (GetMessage(&msg, 0, 0, 0))
	{
		DispatchMessage(&msg);
		TranslateMessage(&msg);
	}

	SetEvent(data->hThreadEnd);

	return msg.wParam;
}

LRESULT CALLBACK ImgViewer::wndproc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	ImgViewer *data;
	HDC hdc;
	PAINTSTRUCT ps;

	switch (iMessage)
	{
		case WM_CREATE:
			data = (ImgViewer *)((LPCREATESTRUCT)lParam)->lpCreateParams;
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)data);
			return 0;
		case WM_PAINT:
			data = (ImgViewer *)GetWindowLongPtr(hWnd, GWL_USERDATA);

			hdc = BeginPaint(hWnd, &ps);

			data->draw(hdc);

			EndPaint(hWnd, &ps);
			return 0;
		case WM_APP:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void ImgViewer::draw(HDC hdc)
{
	BITMAPINFO bmi;

	if (width == 0 || height == 0)
		return;

	memset(&bmi, 0, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 72;
	bmi.bmiHeader.biYPelsPerMeter = 72;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;

	SetDIBitsToDevice(hdc, 0, 0, width, height, 0, 0, 0, height, data, &bmi, DIB_RGB_COLORS);
}

void pixeloverlay(unsigned short width, unsigned short height, PIXEL *under, PIXEL *over, PIXEL *out)
{
	unsigned char alpha;

	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			alpha = over[j * width + i].a;
			out[j * width + i].a = (under[j * width + i].a * (255 - alpha) + over[j * width + i].a * alpha) / 255;
			out[j * width + i].r = (under[j * width + i].r * (255 - alpha) + over[j * width + i].r * alpha) / 255;
			out[j * width + i].g = (under[j * width + i].g * (255 - alpha) + over[j * width + i].g * alpha) / 255;
			out[j * width + i].b = (under[j * width + i].b * (255 - alpha) + over[j * width + i].b * alpha) / 255;
		}
}