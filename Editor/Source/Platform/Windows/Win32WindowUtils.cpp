/*
 * Corebryo
 * Copyright (c) 2026 Jonathan Den Haerynck
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Win32WindowUtils.h"

#include <Windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")


#define REGISTRY_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"
#define VALUE_NAME L"AppsUseLightTheme"

void Win32WindowUtils::ApplyDarkTitlebar(void* nativeHandle)
{
    if (!nativeHandle)
    {
        return;
    }

    HWND Hwnd = static_cast<HWND>(nativeHandle);


    HKEY hKey;


    DWORD value = 1;
    DWORD valueSize = sizeof(value);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, VALUE_NAME, nullptr, nullptr, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS)
        {
        }
        RegCloseKey(hKey);
    }


    if (value == 0)
    {

        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(Hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
    else
    {

        BOOL darkMode = FALSE;
        DwmSetWindowAttribute(Hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
}

void Win32WindowUtils::BringWindowToFront(void* nativeHandle)
{
    if (!nativeHandle)
    {
        return;
    }

    HWND Hwnd = static_cast<HWND>(nativeHandle);


    if (IsIconic(Hwnd))
    {
        ShowWindow(Hwnd, SW_RESTORE);
    }


    DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    DWORD currentThread = GetCurrentThreadId();
    
    if (foregroundThread != currentThread)
    {
        AttachThreadInput(foregroundThread, currentThread, TRUE);
        SetForegroundWindow(Hwnd);
        SetFocus(Hwnd);
        AttachThreadInput(foregroundThread, currentThread, FALSE);
    }
    else
    {
        SetForegroundWindow(Hwnd);
        SetFocus(Hwnd);
    }


    SetWindowPos(
        Hwnd,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
    );
    
    SetWindowPos(
        Hwnd,
        HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
    );


    ShowWindow(Hwnd, SW_SHOW);
    BringWindowToTop(Hwnd);
}
