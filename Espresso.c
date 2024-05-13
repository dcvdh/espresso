// This is free and unencumbered software released into the public domain.

#include "resources.rc"
#include "utilities.h"
#include <windows.h>

#define ESPRESSO        L"Espresso"
#define WAKEUP_INTERVAL 30 * 1000//ms

HICON   empty_icon;
HICON   full_icon;

LPCWSTR reg_path  = L"SOFTWARE\\" ESPRESSO;
LPCWSTR reg_label = L"Enabled";
HKEY    reg_key;

UINT    taskbar_created_msg;
UINT    wakeup_timer;
BOOL    enabled;

NOTIFYICONDATA tray = {
	.cbSize = sizeof(NOTIFYICONDATA),
	.uCallbackMessage = WM_APP,
	.uFlags = NIF_ICON | NIF_MESSAGE
};

VOID CALLBACK Caffeinate() {
	// This is where the magic happens.
	if (SetThreadExecutionState(ES_SYSTEM_REQUIRED) == 0) {
		ShowErrorDialog(ESPRESSO,L"Failed to prevent Windows from sleeping!");
	}

	wakeup_timer = StartTimer(WAKEUP_INTERVAL, Caffeinate);
	if (!wakeup_timer) {
		ShowErrorDialog(ESPRESSO, L"Failed to start the wakeup timer!");
	}
}

BOOL ReflectState(BOOL enabled) {
	SetTrayIcon(&tray, enabled? full_icon : empty_icon);
	return SetRegistryDword(reg_key, reg_label, enabled);
}

LRESULT CALLBACK HandleMessage(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == tray.uCallbackMessage && lp == WM_LBUTTONUP) {
		ReflectState(enabled = !enabled);
		if (enabled) Caffeinate();
		else StopTimer(wakeup_timer);
	}

	else if (msg == taskbar_created_msg) {
		CreateTrayIcon(&tray);
	}

	else if (msg == WM_CLOSE) {
		DestroyTrayIcon(&tray);
		DestroyWindow(window);
		PostQuitMessage(0);
	}

	else {
		return DefWindowProc(window, msg, wp, lp);
	}

	return 0;
}

static VOID SwitchToMessageLoop() {
	MSG message; BOOL status;

	while ((status = GetMessage(&message, NULL, 0, 0)) > 0) {
		DispatchMessage(&message);
	}

	if (status < 0) {
		ShowErrorDialog(ESPRESSO, L"Unknown error in message loop!");
		DestroyTrayIcon(&tray);
		ExitProcess(1);
	}
}

VOID Start() {
	if (IsAlreadyRunning(ESPRESSO)) {
		ExitProcess(0);
	}

	reg_key = GetRegistryKey(reg_path, TRUE);
	enabled = GetRegistryDword(reg_key, reg_label);

	empty_icon = LoadTrayIcon(EMPTY_ICON_ID);
	full_icon = LoadTrayIcon(FULL_ICON_ID);
	if (empty_icon == NULL || full_icon == NULL) {
		ShowErrorDialog(ESPRESSO, L"Failed to load embedded icons!");
		ExitProcess(1);
	}

	// It's necessary to handle this to survive explorer restarts.
	taskbar_created_msg = RegisterWindowMessage(L"TaskbarCreated");

	tray.hWnd = RegisterMessageHandler(HandleMessage);
	if (tray.hWnd == NULL) {
		ShowErrorDialog(ESPRESSO, L"Failed to register message handler!");
		ExitProcess(1);
	}

	if (!CreateTrayIcon(&tray)) {
		ShowErrorDialog(ESPRESSO, L"Failed to create tray icon!");
		ExitProcess(1);
	}

	// Check if we can write to the registry upfront to prevent spamming.
	if (!ReflectState(enabled)) {
		ShowTrayWarning(&tray, L"Can't save state to the registry!");
	}

	if (enabled) {
		Caffeinate();
	}

	SwitchToMessageLoop();
	DestroyTrayIcon(&tray);

	ExitProcess(0);
}
