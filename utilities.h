// This is free and unencumbered software released into the public domain.

#pragma once
#include <windows.h>
#include <strsafe.h>

static BOOL IsAlreadyRunning(const WCHAR* id) {
	CreateMutex(NULL, FALSE, id);
	return GetLastError() == ERROR_ALREADY_EXISTS;
}

static BOOL CreateTrayIcon(NOTIFYICONDATA* handle) {
	return Shell_NotifyIcon(NIM_ADD, handle);
}

static BOOL DestroyTrayIcon(NOTIFYICONDATA* handle) {
	return Shell_NotifyIcon(NIM_DELETE, handle);
}

static BOOL SetTrayIcon(NOTIFYICONDATA* handle, HICON icon) {
	handle->hIcon = icon;
	return Shell_NotifyIcon(NIM_MODIFY, handle);
}

static HICON LoadTrayIcon(DWORD id) {
	return LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_ICON,
		SM_CXSMICON, SM_CYSMICON, 0);
}

static UINT StartTimer(UINT milliseconds, TIMERPROC callback) {
	return SetTimer(NULL, 0, milliseconds, callback);
}

static BOOL StopTimer(UINT id) {
	return KillTimer(NULL, id);
}

static WCHAR* GetErrorMessage(DWORD code)
{
	WCHAR* buffer;
	DWORD language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
	            | FORMAT_MESSAGE_FROM_SYSTEM
	            | FORMAT_MESSAGE_IGNORE_INSERTS;

	DWORD chars = FormatMessage(flags, NULL, code, language,
		(WCHAR*) &buffer, 0, NULL);
	if (! chars) return NULL;

	return buffer;
}

static WCHAR* FormatString(const WCHAR* format, ...)
{
	WCHAR* buffer;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
	            | FORMAT_MESSAGE_FROM_STRING;

	va_list args;
	va_start(args, format);
		DWORD chars = FormatMessage( flags, format, 0, 0,
			(WCHAR*) &buffer, 0, &args);
	va_end(args);

	if (! chars) return NULL;
	return buffer;
}

static WCHAR* ExplainError(const WCHAR* message)
{
	DWORD error = GetLastError();
	WCHAR* explanation = GetErrorMessage(error);

	WCHAR* combined = FormatString(L"%1\n\nError %2!X!: %3",
		message, error, explanation);

	LocalFree(explanation);
	return combined;
}

static BOOL ShowTrayWarning(NOTIFYICONDATA* handle, const WCHAR* message)
{
	handle->dwInfoFlags = NIIF_WARNING;
	WCHAR* full_message = ExplainError(message);

	// These are fixed length strings.
	StringCchCopy(handle->szInfoTitle, sizeof(handle->szInfoTitle), L"Warning");
	StringCchCopy(handle->szInfo, sizeof(handle->szInfo), full_message);

	LocalFree(full_message);

	handle->uFlags |= NIF_INFO;
	BOOL status = Shell_NotifyIcon(NIM_MODIFY, handle);
	handle->uFlags &= ~NIF_INFO;

	return status;
}

static VOID ShowErrorDialog(const WCHAR* title, const WCHAR* message)
{
	WCHAR* full_message = ExplainError(message);
	MessageBox(NULL, full_message, title, MB_ICONERROR | MB_OK);
	LocalFree(full_message);
}

static HWND RegisterMessageHandler(WNDPROC handler) {
	static UINT64 count = 0;

	WCHAR* class_name = FormatString(L"MESSAGE_HANDLER_%u", ++count);
	if (class_name == NULL) return NULL;

	WNDCLASS flags = {
		.hInstance = GetModuleHandle(NULL),
		.lpszClassName = class_name,
		.lpfnWndProc = handler,
	};

	// We need to create a hidden window.
	HWND handle = CreateWindow(MAKEINTATOM(RegisterClass(&flags)), NULL, WS_POPUP,
		0, 0, 0, 0, NULL, NULL, flags.hInstance, NULL);

	LocalFree(class_name);
	return handle;
}

static HKEY GetRegistryKey(const WCHAR* key, BOOL createIfMissing)
{
	HKEY result;
	switch (RegOpenKey(HKEY_CURRENT_USER, key, &result))
	{
		case ERROR_SUCCESS: return result;
		case ERROR_FILE_NOT_FOUND: if (createIfMissing)
		{
			if (RegCreateKey(HKEY_CURRENT_USER, key, &result) == ERROR_SUCCESS) {
				return result;
			}
		}
	}
	return NULL;
}

static DWORD GetRegistryDword(HKEY key, const WCHAR* label)
{
	DWORD result, size;
	RegGetValue(key, NULL, label, RRF_ZEROONFAILURE,
		NULL, &result, &size);
	return result;
}

static BOOL SetRegistryDword(HKEY key, const WCHAR* label, DWORD value)
{
	return RegSetValueEx(key, label, 0, REG_DWORD,
		(BYTE*) &value, sizeof(DWORD)) == ERROR_SUCCESS;
}
