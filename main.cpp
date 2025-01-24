#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

// Enable Visual Styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Constants
#define ID_BUTTON 1
#define ID_CHECKBOX 2
#define IDI_ICON1 101
#define DWMWCP_ROUND 2
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define REGISTRY_PATH L"Software\\ScreenOff"
#define REGISTRY_VALUE L"AutoOff"

int GetDpiForWindow(HWND hwnd) {
    // Get the DPI for the window
    const HMODULE hUser32 = GetModuleHandle(TEXT("user32"));
    typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
    GetDpiForWindowFunc getDpiForWindow = (GetDpiForWindowFunc)GetProcAddress(hUser32, "GetDpiForWindow");
    
    if (getDpiForWindow != nullptr) {
        return getDpiForWindow(hwnd);
    }
    
    // Fallback for older Windows versions
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);
    return dpi;
}

int ScaleForDpi(int value, int dpi) {
    return MulDiv(value, dpi, 96); 
}

HFONT CreateSystemFont(HWND hwnd) {
    int dpi = GetDpiForWindow(hwnd);
    NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };

    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
    
    LOGFONTW messageFont = ncm.lfMessageFont;
    messageFont.lfHeight = ScaleForDpi(-12, dpi);
    wcscpy_s(messageFont.lfFaceName, L"Segoe UI Variable");
    
    return CreateFontIndirectW(&messageFont);
}

// Registry functions
bool LoadAutoOffState() {
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(DWORD);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, REGISTRY_VALUE, NULL, NULL, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value != 0;
}

void SaveAutoOffState(bool state) {
    HKEY hKey;
    DWORD value = state ? 1 : 0;
    
    RegCreateKeyExW(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExW(hKey, REGISTRY_VALUE, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
    RegCloseKey(hKey);
}

// UI Helper functions
HFONT CreateModernFont(HWND hwnd) {
    return CreateSystemFont(hwnd);
}

void StyleButton(HWND hButton) {
    SetWindowTheme(hButton, L"Explorer", NULL);
    LONG_PTR style = GetWindowLongPtr(hButton, GWL_STYLE);
    style |= BS_PUSHBUTTON | BS_NOTIFY;
    SetWindowLongPtr(hButton, GWL_STYLE, style);
    SetWindowPos(hButton, NULL, 0, 0, 260, 32, SWP_NOMOVE | SWP_NOZORDER);
}

void TurnOffScreen(HWND hwnd) {
    EnableWindow(hwnd, FALSE);
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
    SetTimer(hwnd, 2, 1000, NULL);
}

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont = NULL;
    static HBRUSH hBrush = NULL;
    static HWND hCheckbox = NULL;

    switch (uMsg) {
        case WM_CREATE: {
            // Enable rounded corners
            DWORD value = DWMWCP_ROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));

            // Initialize controls
            INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
            InitCommonControlsEx(&icex);

            hFont = CreateModernFont(hwnd);;
            hBrush = CreateSolidBrush(RGB(255, 255, 255));

            // Create main button
            HWND hButton = CreateWindowW(
                L"BUTTON", L"Turn Off Screen",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 20, 260, 32,
                hwnd, (HMENU)ID_BUTTON, NULL, NULL
            );
            StyleButton(hButton);
            SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetWindowTheme(hButton, L"Explorer", NULL);

            // Create checkbox
            hCheckbox = CreateWindowW(
                L"BUTTON", L"Turn off screen on app startup",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                20, 70, 260, 30,
                hwnd, (HMENU)ID_CHECKBOX, NULL, NULL
            );
            
            bool autoOff = LoadAutoOffState();
            SendMessage(hCheckbox, BM_SETCHECK, autoOff ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(hCheckbox, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetWindowTheme(hCheckbox, L"Explorer", NULL);

            if (autoOff) {
                SetTimer(hwnd, 1, 500, NULL);
            }
            break;
        }
		
		case WM_DPICHANGED: {
			// Update font when DPI changes
			if (hFont) DeleteObject(hFont);
			hFont = CreateModernFont(hwnd);
			
			// Update font for all controls
			EnumChildWindows(hwnd, [](HWND hChild, LPARAM lParam) -> BOOL {
				SendMessage(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
				return TRUE;
			}, (LPARAM)hFont);
			
			// Update window size and position
			RECT* prc = (RECT*)lParam;
			SetWindowPos(hwnd, NULL, 
				prc->left, prc->top, 
				prc->right - prc->left, prc->bottom - prc->top, 
				SWP_NOZORDER | SWP_NOACTIVATE);
			
			return 0;
		}

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON && HIWORD(wParam) == BN_CLICKED) {
                SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
            }
            if (LOWORD(wParam) == ID_CHECKBOX && HIWORD(wParam) == BN_CLICKED) {
                SaveAutoOffState(SendMessage(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
            break;

        case WM_TIMER:
            if (wParam == 1) {
                KillTimer(hwnd, 1);
                TurnOffScreen(hwnd);
            }
            else if (wParam == 2) {
                KillTimer(hwnd, 2);
                EnableWindow(hwnd, TRUE);
            }
            break;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)hBrush;
        }

        case WM_DESTROY:
            if (hFont) DeleteObject(hFont);
            if (hBrush) DeleteObject(hBrush);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenOff";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(
        L"ScreenOff", L"Screen Off",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 140,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;
    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}