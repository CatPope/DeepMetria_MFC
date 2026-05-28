"""
SetForegroundWindow 제한 해제 — UI 자동화 전 실행
pywinauto의 set_focus를 Alt 키 트릭으로 패치한다.
"""
import ctypes
import ctypes.wintypes

def apply_foreground_fix():
    try:
        ctypes.windll.user32.AllowSetForegroundWindow(ctypes.wintypes.DWORD(-1))
    except Exception:
        pass

    try:
        val = ctypes.wintypes.DWORD(0)
        ctypes.windll.user32.SystemParametersInfoW(
            0x2001, 0, ctypes.byref(val), 0)
    except Exception:
        pass

    try:
        import pywinauto.controls.hwndwrapper as hw
        _original = hw.HwndWrapper.set_focus

        def _patched_set_focus(self):
            try:
                VK_MENU = 0x12
                KEYEVENTF_KEYUP = 0x0002
                ctypes.windll.user32.keybd_event(VK_MENU, 0, 0, 0)
                ctypes.windll.user32.keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0)
            except Exception:
                pass
            return _original(self)

        hw.HwndWrapper.set_focus = _patched_set_focus
    except Exception:
        pass

apply_foreground_fix()
