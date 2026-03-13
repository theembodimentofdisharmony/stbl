# Copyright (c) 2026 enstarep <enstarep@rncyk.org> and other authors of stbdl
# License: https://github.com/theembodimentofdisharmony/stbdl/blob/main/LICENSE

import os
import subprocess
import ctypes
from ctypes import wintypes


__version__ = "0.5.1"


def window(path: str = None, mode: int = 3):
    """
    The background window setting function. If no value is provided, it defaults to 3. Also, the background window needs to be clicked on a blank area of the taskbar to bring the taskbar above it.
    :param path: The parameter `path` is the path to the background image, **must be an absolute path**. If no value is provided, it defaults to loading `stbdl\\white.png`.
    :param mode: The parameter `mode` is the dynamic style, with the following meanings: mode=0: Fade in/out; mode=1: Rotation; mode=2: Zoom; mode=3: Four-color cycle (red, green, blue, yellow); mode=4: Color cycle
    :return: None
    """
    if path is not None and not isinstance(path, str):
        raise TypeError("The parameter 'path' must be 'str'")
    if not isinstance(mode, int):
        raise TypeError("The parameter 'mode' must be 'int'")

    current_dir = os.path.dirname(os.path.abspath(__file__))
    pro = os.path.join(current_dir, "stbdl.exe")
    file = path if path is not None else os.path.join(current_dir, "white.png")
    subprocess.Popen([pro, file, str(mode)])


def expansion():
    """
    Enables the stbdl taskbar extension. The actual effect depends on system support. It prioritizes the mica effect (requires Windows 11), then the acrylic effect (requires Windows 10), and finally the blur effect.
    :return: None
    """
    class DWM_BLURBEHIND(ctypes.Structure):
        _fields_ = [
            ("dwFlags", wintypes.DWORD),
            ("fEnable", wintypes.BOOL),
            ("hRgnBlur", wintypes.HANDLE),
            ("fTransitionOnMaximized", wintypes.BOOL)
        ]

    class ACCENT_POLICY(ctypes.Structure):
        _fields_ = [
            ("AccentState", wintypes.DWORD),
            ("AccentFlags", wintypes.DWORD),
            ("GradientColor", wintypes.DWORD),
            ("AnimationId", wintypes.DWORD)
        ]

    class WINDOWCOMPOSITIONATTRIBDATA(ctypes.Structure):
        _fields_ = [
            ("Attribute", wintypes.DWORD),
            ("Data", ctypes.POINTER(ACCENT_POLICY)),
            ("SizeOfData", ctypes.c_size_t)
        ]

    WS_EX_LAYERED = 0x00080000
    GWL_EXSTYLE = -20
    LWA_ALPHA = 0x00000002
    DWMWA_MICA = 1029
    DWM_BB_ENABLE = 0x00000001
    WCA_ACCENT_POLICY = 19
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 3

    def _enable_blur_behind(hwnd):
        dwmapi.DwmEnableBlurBehindWindow.argtypes = [
            wintypes.HWND,
            ctypes.POINTER(DWM_BLURBEHIND)
        ]
        dwmapi.DwmEnableBlurBehindWindow.restype = wintypes.LONG
        blur = DWM_BLURBEHIND()
        blur.dwFlags = DWM_BB_ENABLE
        blur.fEnable = True
        blur.hRgnBlur = None
        blur.fTransitionOnMaximized = False
        dwmapi.DwmEnableBlurBehindWindow(hwnd, ctypes.byref(blur))

    def _try_acrylic(hwnd):
        try:
            SetWindowCompositionAttribute = ctypes.windll.user32.SetWindowCompositionAttribute
        except AttributeError:
            return False

        SetWindowCompositionAttribute.argtypes = [
            wintypes.HWND,
            ctypes.POINTER(WINDOWCOMPOSITIONATTRIBDATA)
        ]
        SetWindowCompositionAttribute.restype = wintypes.BOOL

        accent = ACCENT_POLICY()
        accent.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND
        accent.GradientColor = 0x99FFFFFF
        attr_data = WINDOWCOMPOSITIONATTRIBDATA()
        attr_data.Attribute = WCA_ACCENT_POLICY
        attr_data.Data = ctypes.pointer(accent)
        attr_data.SizeOfData = ctypes.sizeof(accent)

        return bool(SetWindowCompositionAttribute(hwnd, ctypes.byref(attr_data)))

    user32 = ctypes.windll.user32
    dwmapi = ctypes.windll.dwmapi
    kernel32 = ctypes.windll.kernel32

    user32.FindWindowW.argtypes = [wintypes.LPCWSTR, wintypes.LPCWSTR]
    user32.FindWindowW.restype = wintypes.HWND
    hTaskbar = user32.FindWindowW("Shell_TrayWnd", None)
    if not hTaskbar:
        raise RuntimeError("Cannot find taskbar window")

    user32.GetWindowLongW.argtypes = [wintypes.HWND, ctypes.c_int]
    user32.GetWindowLongW.restype = wintypes.LONG
    user32.SetWindowLongW.argtypes = [wintypes.HWND, ctypes.c_int, wintypes.LONG]
    user32.SetWindowLongW.restype = wintypes.LONG

    ex_style = user32.GetWindowLongW(hTaskbar, GWL_EXSTYLE)
    ex_style |= WS_EX_LAYERED
    user32.SetWindowLongW(hTaskbar, GWL_EXSTYLE, ex_style)

    dwmapi.DwmSetWindowAttribute.argtypes = [
        wintypes.HWND,
        wintypes.DWORD,
        ctypes.c_void_p,
        wintypes.DWORD
    ]
    dwmapi.DwmSetWindowAttribute.restype = wintypes.LONG
    mica = wintypes.BOOL(True)
    hr = dwmapi.DwmSetWindowAttribute(
        hTaskbar, DWMWA_MICA,
        ctypes.byref(mica), ctypes.sizeof(wintypes.BOOL)
    )

    if hr != 0:
        if not _try_acrylic(hTaskbar):
            _enable_blur_behind(hTaskbar)

    user32.SetLayeredWindowAttributes.argtypes = [
        wintypes.HWND,
        wintypes.COLORREF,
        wintypes.BYTE,
        wintypes.DWORD
    ]
    user32.SetLayeredWindowAttributes.restype = wintypes.BOOL
    user32.SetLayeredWindowAttributes(hTaskbar, 0, 220, LWA_ALPHA)

    return


__user32 = ctypes.windll.user32

HWND = wintypes.HWND
LPCWSTR = wintypes.LPCWSTR
BOOL = wintypes.BOOL
DWORD = wintypes.DWORD
LONG_PTR = ctypes.c_ssize_t

GWL_STYLE = -16
WS_BORDER = 0x00800000

__user32.FindWindowW.argtypes = [LPCWSTR, LPCWSTR]
__user32.FindWindowW.restype = HWND

__user32.FindWindowExW.argtypes = [HWND, HWND, LPCWSTR, LPCWSTR]
__user32.FindWindowExW.restype = HWND

try:
    __user32.GetWindowLongPtrW.argtypes = [HWND, ctypes.c_int]
    __user32.GetWindowLongPtrW.restype = LONG_PTR
    get_style = lambda h: __user32.GetWindowLongPtrW(h, GWL_STYLE)
except AttributeError:
    __user32.GetWindowLongW.argtypes = [HWND, ctypes.c_int]
    __user32.GetWindowLongW.restype = LONG_PTR
    get_style = lambda h: __user32.GetWindowLongW(h, GWL_STYLE)

try:
    __user32.SetWindowLongPtrW.argtypes = [HWND, ctypes.c_int, LONG_PTR]
    __user32.SetWindowLongPtrW.restype = LONG_PTR
    set_style = lambda h, style: __user32.SetWindowLongPtrW(h, GWL_STYLE, style)
except AttributeError:
    __user32.SetWindowLongW.argtypes = [HWND, ctypes.c_int, LONG_PTR]
    __user32.SetWindowLongW.restype = LONG_PTR
    set_style = lambda h, style: __user32.SetWindowLongW(h, GWL_STYLE, style)


def taskbar_switcher():
    """
    Gets the handle of the task switcher window (class name `MSTaskListWClass`). Returns None if not found.
    :return: wintypes.HWND
    """
    hTaskbar = __user32.FindWindowW("Shell_TrayWnd", None)
    if not hTaskbar:
        hTaskbar = __user32.FindWindowW("Shell_SecondaryTrayWnd", None)
    if not hTaskbar:
        return None

    hRebar = __user32.FindWindowExW(hTaskbar, None, "ReBarWindow32", None)
    if hRebar:
        hSwitcher = __user32.FindWindowExW(hRebar, None, "MSTaskSwWClass", None)
        if hSwitcher:
            return hSwitcher

    hWorkerW = __user32.FindWindowExW(hTaskbar, None, "MSTaskListWClass", None)
    if hWorkerW:
        return hWorkerW

    return None


def border(hwnd):
    """
    Used to add a border to a specified window
    :param hwnd: The parameter 'hwnd' is the handle of specified window
    :return: None
    """
    if not hwnd:
        return

    current_style = get_style(hwnd)
    new_style = current_style | WS_BORDER
    set_style(hwnd, new_style)

    SWP_NOSIZE = 0x0001
    SWP_NOMOVE = 0x0002
    SWP_FRAMECHANGED = 0x0020
    flags = SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED

    __user32.SetWindowPos(hwnd, None, 0, 0, 0, 0, flags)
    return
