#include "project.h"
#include "resources/embed.rc"

#define _WIN32_WINNT 0x0601 /* Windows 7 */
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>

NOTIFYICONDATA tray = {
	.cbSize = sizeof(NOTIFYICONDATA),
	.uCallbackMessage = WM_APP,
	.uFlags = NIF_ICON | NIF_MESSAGE
};

struct {
	HKEY key;
	LPTSTR path;
	LPTSTR label;
	DWORD state;
	DWORD size;
} reg;

struct {
	HICON empty;
	HICON full;
} icon;

struct {
	UINT create;
} msg;

LRESULT CALLBACK WndProc(HWND wnd, UINT id, WPARAM wp, LPARAM lp)
{
	if (id == WM_TIMER) {
		if (reg.state) {
			/* This is where the magic happens */
			SetThreadExecutionState(ES_SYSTEM_REQUIRED);
		}

		/* 30 second repeat */
		SetTimer(tray.hWnd, 1, 30 * 1000, NULL);
	}

	/* Only handle left-clicks */
	else if (id == tray.uCallbackMessage && lp == WM_LBUTTONUP) {
		if (reg.state) {
			reg.state = 0;
			tray.hIcon = icon.empty;
		}
		else {
			reg.state = 1;
			tray.hIcon = icon.full;
		}

		/* Clear any notifications */
		tray.uFlags &= ~NIF_INFO;
		Shell_NotifyIcon(NIM_MODIFY, &tray);

		/* Save state to the registry */
		RegSetValueEx(reg.key, reg.label, 0, REG_DWORD, (BYTE*) &reg.state, reg.size);
	}

	else if (id == msg.create) {
		Shell_NotifyIcon(NIM_ADD, &tray);
	}

	else if (id == WM_DESTROY) {
		Shell_NotifyIcon(NIM_DELETE, &tray);
		PostQuitMessage(0);
	}

	else {
		return DefWindowProc(wnd, id, wp, lp);
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR args, int show)
{
	/* Only allow a single instance */
	CreateMutex(NULL, FALSE, TEXT(PROG_NAME));
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}

	/* Check the saved state */
	reg.path = TEXT("Software\\" PROG_NAME);
	reg.label = TEXT("Enabled");
	reg.size = sizeof(DWORD);
	if (RegOpenKey(HKEY_CURRENT_USER, reg.path, &reg.key)) {
		RegCreateKey(HKEY_CURRENT_USER, reg.path, &reg.key);
	}
	RegGetValue(reg.key, NULL, reg.label, RRF_ZEROONFAILURE, NULL, &reg.state, &reg.size);

	/* Make sure we can save it */
	if (RegSetValueEx(reg.key, reg.label, 0, REG_DWORD, (BYTE*) &reg.state, reg.size)) {
		StrCpy(tray.szInfoTitle, TEXT("Mi Scusi"));
		StrCpy(tray.szInfo, TEXT("Can't save state to the registry!"));
		tray.dwInfoFlags = NIIF_WARNING;
		tray.uFlags |= NIF_INFO;
	}

	/* Load the tray icons */
	LoadIconMetric(inst, MAKEINTRESOURCE(ICON_EMPTY), LIM_SMALL, &icon.empty);
	LoadIconMetric(inst, MAKEINTRESOURCE(ICON_FULL), LIM_SMALL, &icon.full);
	tray.hIcon = reg.state? icon.full : icon.empty;

	/* Hidden window connects the icon to WndProc() */
	WNDCLASS class = {
		.hInstance = inst,
		.lpfnWndProc = WndProc,
		.lpszClassName = TEXT(PROG_NAME)
	};
	tray.hWnd = CreateWindow(
		MAKEINTATOM(RegisterClass(&class)), /* lpClassName */
		NULL, /* lpWindowName */
		WS_POPUP, /* dwStyle */
		0, /* x */
		0, /* y */
		0, /* nWidth */
		0, /* nHeight */
		NULL, /* hWndParent */
		NULL, /* hMenu */
		inst, /* hInstance */
		NULL /* lpParam */
	);

	/* Create the icon and survive Explorer restarts */
	msg.create = RegisterWindowMessage(TEXT("TaskbarCreated"));
	PostMessage(tray.hWnd, msg.create, 0, 0);

	/* Start the timer loop */
	PostMessage(tray.hWnd, WM_TIMER, 0, 0);

	/* Dispatch messages to WndProc() */
	MSG queue;
	while (GetMessage(&queue, NULL, 0, 0) > 0) {
		DispatchMessage(&queue);
	}

	return (int) queue.wParam;
}
