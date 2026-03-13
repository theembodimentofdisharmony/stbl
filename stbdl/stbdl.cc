#include <windows.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <cmath>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup")

using namespace Gdiplus;

GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR g_gdiplusToken = 0;
Image* g_pImage = nullptr;

HWND g_hOverlay = nullptr;
HWND g_hTargetWnd = nullptr;
UINT_PTR g_followTimer = 0;
UINT_PTR g_animTimer = 0;

int g_alpha = 255;
int g_alphaDirection = -5;
float g_angle = 0.0f;
float g_scale = 1.0f;
float g_scaleDirection = 0.01f;
int g_hueShift = 0;
int g_animMode = 0;

int g_colorPhase = 0;
int g_colorChangeCounter = 0;
const int COLOR_CHANGE_DELAY = 10;

const float COLOR_RED[3]   = { 0xF6/255.0f, 0x53/255.0f, 0x14/255.0f }; // #F65314
const float COLOR_GREEN[3] = { 0x7C/255.0f, 0xBB/255.0f, 0x00/255.0f }; // #7CBB00
const float COLOR_BLUE[3]  = { 0x00/255.0f, 0xA1/255.0f, 0xF1/255.0f }; // #00A1F1
const float COLOR_YELLOW[3]= { 0xFF/255.0f, 0xBB/255.0f, 0x00/255.0f }; // #FFBB00

HWND FindTaskbarSwitcher() {
    HWND hTaskbar = FindWindow("Shell_TrayWnd", NULL);
    if (!hTaskbar) hTaskbar = FindWindow("Shell_SecondaryTrayWnd", NULL);
    if (!hTaskbar) return NULL;

    HWND hRebar = FindWindowEx(hTaskbar, NULL, "ReBarWindow32", NULL);
    if (hRebar) {
        HWND hSwitcher = FindWindowEx(hRebar, NULL, "MSTaskSwWClass", NULL);
        if (hSwitcher) return hSwitcher;
    }

    HWND hWorkerW = FindWindowEx(hTaskbar, NULL, "MSTaskListWClass", NULL);
    if (hWorkerW) return hWorkerW;

    return NULL;
}

LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (hdc && g_pImage) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                int w = rc.right - rc.left;
                int h = rc.bottom - rc.top;

                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
                HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

                FillRect(memDC, &rc, (HBRUSH)GetStockObject(NULL_BRUSH));

                Graphics graphics(memDC);
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

                graphics.TranslateTransform(w / 2.0f, h / 2.0f);
                if (g_animMode == 1) graphics.RotateTransform(g_angle);
                else if (g_animMode == 2) graphics.ScaleTransform(g_scale, g_scale);
                graphics.TranslateTransform(-w / 2.0f, -h / 2.0f);

                ColorMatrix colorMatrix = {
                    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, g_alpha / 255.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
                };

                if (g_animMode == 3) {
                    const float* col = nullptr;
                    switch (g_colorPhase % 4) {
                        case 0: col = COLOR_RED; break;
                        case 1: col = COLOR_GREEN; break;
                        case 2: col = COLOR_BLUE; break;
                        case 3: col = COLOR_YELLOW; break;
                    }

                    colorMatrix.m[0][0] = col[0]; // R
                    colorMatrix.m[1][1] = col[1]; // G
                    colorMatrix.m[2][2] = col[2]; // B
                }
                else if (g_animMode == 4) {
                    float r = (sin(g_hueShift * 0.1f) + 1) / 2;
                    float g = (sin(g_hueShift * 0.1f + 2.0f) + 1) / 2;
                    float b = (sin(g_hueShift * 0.1f + 4.0f) + 1) / 2;
                    colorMatrix.m[0][0] = r;
                    colorMatrix.m[1][1] = g;
                    colorMatrix.m[2][2] = b;
                }

                ImageAttributes imgAttr;
                imgAttr.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

                Rect destRect(0, 0, w, h);
                graphics.DrawImage(g_pImage, destRect, 0, 0, g_pImage->GetWidth(), g_pImage->GetHeight(), UnitPixel, &imgAttr);

                BLENDFUNCTION blend = {0};
                blend.BlendOp = AC_SRC_OVER;
                blend.SourceConstantAlpha = 255;
                blend.AlphaFormat = AC_SRC_ALPHA;
                AlphaBlend(hdc, 0, 0, w, h, memDC, 0, 0, w, h, blend);

                SelectObject(memDC, oldBM);
                DeleteObject(memBM);
                DeleteDC(memDC);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                switch (g_animMode) {
                    case 0:
                        g_alpha += g_alphaDirection;
                        if (g_alpha <= 100) { g_alpha = 100; g_alphaDirection = 5; }
                        else if (g_alpha >= 255) { g_alpha = 255; g_alphaDirection = -5; }
                        break;
                    case 1:
                        g_angle += 2.0f;
                        if (g_angle >= 360) g_angle -= 360;
                        break;
                    case 2:
                        g_scale += g_scaleDirection;
                        if (g_scale >= 1.5f || g_scale <= 0.8f) g_scaleDirection = -g_scaleDirection;
                        break;
                    case 3:
                        g_colorChangeCounter++;
                        if (g_colorChangeCounter >= COLOR_CHANGE_DELAY) {
                            g_colorPhase++;
                            g_colorChangeCounter = 0;
                        }
                        break;
                    case 4:
                        g_hueShift++;
                        break;
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else if (wParam == 2) {
                if (!g_hTargetWnd || !IsWindow(g_hTargetWnd)) {
                    g_hTargetWnd = FindTaskbarSwitcher();
                    if (!g_hTargetWnd) {
                        ShowWindow(g_hOverlay, SW_HIDE);
                        return 0;
                    }
                }

                RECT rcTarget;
                if (GetWindowRect(g_hTargetWnd, &rcTarget)) {
                    RECT rcOverlay;
                    GetWindowRect(g_hOverlay, &rcOverlay);
                    if (rcOverlay.left != rcTarget.left || rcOverlay.top != rcTarget.top ||
                        rcOverlay.right - rcOverlay.left != rcTarget.right - rcTarget.left ||
                        rcOverlay.bottom - rcOverlay.top != rcTarget.bottom - rcTarget.top) {
                        SetWindowPos(g_hOverlay, HWND_TOPMOST,
                                     rcTarget.left, rcTarget.top,
                                     rcTarget.right - rcTarget.left,
                                     rcTarget.bottom - rcTarget.top,
                                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
                    }
                    BOOL targetVisible = IsWindowVisible(g_hTargetWnd);
                    ShowWindow(g_hOverlay, targetVisible ? SW_SHOW : SW_HIDE);
                }
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (event == EVENT_OBJECT_CREATE && idObject == OBJID_WINDOW) {
        WCHAR className[256];
        GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"MSTaskSwWClass") == 0 || wcscmp(className, L"MSTaskListWClass") == 0) {
            g_hTargetWnd = hwnd;
        }
    }
    else if (event == EVENT_OBJECT_DESTROY && hwnd == g_hTargetWnd) {
        g_hTargetWnd = NULL;
    }
}

void Cleanup() {
    if (g_animTimer) KillTimer(g_hOverlay, 1);
    if (g_followTimer) KillTimer(g_hOverlay, 2);
    if (g_hOverlay) DestroyWindow(g_hOverlay);
    GdiplusShutdown(g_gdiplusToken);
    if (g_pImage) delete g_pImage;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	auto ClassName = L"stblWindow";

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 3) { return 0; }
    const wchar_t* imagePath = argv[1];
    int mode = _wtoi(argv[2]);
    if (mode < 0 || mode > 4) mode = 0;
    g_animMode = mode;

    GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);
    g_pImage = Image::FromFile(imagePath);
    if (!g_pImage || g_pImage->GetLastStatus() != Ok) {
        MessageBox(NULL, "Failed to load image.", "Error", MB_ICONERROR);
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    g_hTargetWnd = FindTaskbarSwitcher();
    if (!g_hTargetWnd) {
        MessageBox(NULL, "Taskbar switcher window not found.", "Error", MB_ICONERROR);
        delete g_pImage;
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = ClassName;
    RegisterClassExW(&wc);

    RECT rcTarget;
    GetWindowRect(g_hTargetWnd, &rcTarget);

    g_hOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        ClassName, NULL,
        WS_POPUP,
        rcTarget.left, rcTarget.top,
        rcTarget.right - rcTarget.left,
        rcTarget.bottom - rcTarget.top,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hOverlay) {
        MessageBox(NULL, "Failed to create overlay window.", "Error", MB_ICONERROR);
        delete g_pImage;
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    SetLayeredWindowAttributes(g_hOverlay, RGB(0,0,0), 255, LWA_ALPHA);

    g_animTimer = SetTimer(g_hOverlay, 1, 50, NULL);
    g_followTimer = SetTimer(g_hOverlay, 2, 50, NULL);

    ShowWindow(g_hOverlay, SW_SHOW);

    HWINEVENTHOOK g_hWinEventHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Cleanup();
    return 0;
}