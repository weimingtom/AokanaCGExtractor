#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <Windows.h>

typedef union _PIXEL{
	struct {
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char a;
	};
	DWORD value;
} PIXEL;

void pixeloverlay(unsigned short width, unsigned short height, PIXEL *under, PIXEL *over, PIXEL *out);

class ImgViewer {
private:
	unsigned short width;
	unsigned short height;
	PIXEL *data;

	BOOL bInit;
	BOOL bData;
	HWND hWindow;
	HANDLE hThread;

	HANDLE hThreadInit;
	HANDLE hThreadEnd;

	void draw(HDC hdc);
public:
	ImgViewer();
	~ImgViewer();
	bool create(unsigned short width, unsigned short height);
	void release();
	void changeCaption(WCHAR *pszCaption);
	void show();
	void hide();
	void foreground();
	void setdata(PIXEL *data);
	void overlay(PIXEL *data);
	unsigned short getwidth();
	unsigned short getheight();

	static DWORD WINAPI callback(LPVOID arg);
	static LRESULT CALLBACK wndproc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
};

#endif