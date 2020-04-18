#define _WIN32_WINNT 0x0601 // Windows 7
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>

#include "resources/embed.rc"

TCHAR state[MAX_PATH];

NOTIFYICONDATA tray = {
    .cbSize = sizeof(NOTIFYICONDATA),
    .uCallbackMessage = WM_APP,
    .uFlags = NIF_ICON | NIF_MESSAGE
};

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
        if (tray.hIcon == icon.full) {
            // This is where the magic happens
            SetThreadExecutionState(ES_SYSTEM_REQUIRED);
        }

        // 30 second repeat
        SetTimer(tray.hWnd, 1, 30 * 1000, NULL);
    }

    // Only handle left-clicks
    else if (id == tray.uCallbackMessage && lp == WM_LBUTTONUP) {
        if (tray.hIcon == icon.full) {
            tray.hIcon = icon.empty;
            DeleteFile(state);
        }
        else {
            tray.hIcon = icon.full;
            CloseHandle(CreateFile(state, 0, 0, NULL, CREATE_NEW, 0, NULL));
        }

        Shell_NotifyIcon(NIM_MODIFY, &tray);
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
    // Only allow a single instance
    CreateMutex(NULL, FALSE, TEXT(R_UUID));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // Load the tray icons
    LoadIconMetric(inst, MAKEINTRESOURCE(R_ICON_EMPTY), LIM_SMALL,
                   &icon.empty);
    LoadIconMetric(inst, MAKEINTRESOURCE(R_ICON_FULL), LIM_SMALL,
                   &icon.full);

    // Find the exe's parent directory
    GetModuleFileName(inst, state, MAX_PATH);
    PathRemoveFileSpec(state);

    // Check the saved state
    PathAppend(state, TEXT("Enabled"));
    if (PathFileExists(state)) {
        tray.hIcon = icon.full;
    }
    else {
        tray.hIcon = icon.empty;
    }

    // Create a hidden window to connect the icon to WndProc()
    WNDCLASS class = {
        .hInstance = inst,
        .lpfnWndProc = WndProc,
        .lpszClassName = TEXT(R_UUID)
    };
    tray.hWnd = CreateWindow(
        MAKEINTATOM(RegisterClass(&class)), // lpClassName
        NULL, // lpWindowName
        WS_POPUP, // dwStyle
        0, // x
        0, // y
        0, // nWidth
        0, // nHeight
        NULL, // hWndParent
        NULL, // hMenu
        inst, // hInstance
        NULL // lpParam
    );

    // Create the icon and make it persist through Explorer restarts
    msg.create = RegisterWindowMessage(TEXT("TaskbarCreated"));
    PostMessage(tray.hWnd, msg.create, 0, 0);

    // Start the timer loop
    PostMessage(tray.hWnd, WM_TIMER, 0, 0);

    // Dispatch messages to WndProc()
    MSG queue;
    while (GetMessage(&queue, NULL, 0, 0) > 0) {
        DispatchMessage(&queue);
    }

    return (int)queue.wParam;
}
