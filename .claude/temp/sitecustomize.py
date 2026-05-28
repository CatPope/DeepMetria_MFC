"""
SetForegroundWindow 제한 해제 — UI 자동화 전 자동 로드
AttachThreadInput 기반 패치
"""
import ctypes
import ctypes.wintypes

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32


def _force_foreground(hwnd):
    """AttachThreadInput 트릭으로 SetForegroundWindow를 강제한다."""
    fg = user32.GetForegroundWindow()
    if fg == hwnd:
        return True

    fg_tid = user32.GetWindowThreadProcessId(fg, None)
    cur_tid = kernel32.GetCurrentThreadId()

    attached = False
    if fg_tid != cur_tid:
        attached = bool(user32.AttachThreadInput(cur_tid, fg_tid, True))

    user32.BringWindowToTop(hwnd)
    result = user32.SetForegroundWindow(hwnd)

    if attached:
        user32.AttachThreadInput(cur_tid, fg_tid, False)

    return bool(result)


def _apply():
    try:
        user32.AllowSetForegroundWindow(ctypes.wintypes.DWORD(-1))
    except Exception:
        pass

    try:
        import pywinauto.controls.hwndwrapper as hw
        _original = hw.HwndWrapper.set_focus

        def _patched(self):
            try:
                hwnd = self.handle
                if hwnd and _force_foreground(hwnd):
                    return self
            except Exception:
                pass
            return _original(self)

        hw.HwndWrapper.set_focus = _patched
    except ImportError:
        pass


_apply()
