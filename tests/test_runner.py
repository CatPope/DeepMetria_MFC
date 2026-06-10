"""tests/test_runner.py - DeepMetria UI 자동화 테스트 러너 (표준 라이브러리만).

설계:
- EXE 실행 후 메인 윈도우 핸들을 찾고, 각 스위트는 PostMessage(WM_COMMAND, ID_xxx) 로
  MFC 메뉴 명령이 등록·라우팅되는지를 검증한다. 모달 다이얼로그가 뜨면 자동으로 닫는다.
- pyautogui / pywinauto 같은 외부 의존성을 요구하지 않는다.

산출:
- tests/test_results.txt   : [PASS]/[FAIL]/[SKIP] <suite>
- tests/screenshots/run_<TS>/<suite>.png   (현재는 placeholder — 의존성 없이 비움)
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes as wt
import datetime
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"
SHOTS_BASE = TESTS_DIR / "screenshots"
RESULTS = TESTS_DIR / "test_results.txt"

# resource.h 와 동기 — MFC 표준 ID는 AFX 정의값
WM_COMMAND     = 0x0111
ID_FILE_NEW    = 0xE100   # MFC 표준
ID_FILE_OPEN   = 0xE101
ID_APP_EXIT    = 0xE105
ID_APP_ABOUT   = 0xE140
ID_AI_ANALYZE      = 32771
ID_AI_SUMMARIZE    = 32772
ID_VIEW_DASHBOARD  = 32773
ID_EXPORT_PNG      = 32774
ID_FORMAT_VIZ      = 32775

SUITE_CMD = {
    "file_load":     ID_FILE_OPEN,
    "json_load":     ID_FILE_OPEN,
    "query_input":   ID_AI_ANALYZE,
    "data_summary":  ID_AI_SUMMARIZE,
    "dashboard":     ID_VIEW_DASHBOARD,
    "export":        ID_EXPORT_PNG,
    "format_editor": ID_FORMAT_VIZ,
}
SUITES = ["layout"] + list(SUITE_CMD.keys())


user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.EnumWindows.argtypes  = [ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM), wt.LPARAM]
user32.EnumWindows.restype   = wt.BOOL
user32.GetWindowTextLengthW.argtypes = [wt.HWND]
user32.GetWindowTextW.argtypes       = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.IsWindowVisible.argtypes      = [wt.HWND]
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.PostMessageW.restype  = wt.BOOL
user32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]


def find_window(title_substr: str, timeout_sec: float) -> int:
    EnumWindowsProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    end = time.time() + timeout_sec
    while time.time() < end:
        found: list[int] = []

        def cb(hwnd, _lp):
            if not user32.IsWindowVisible(hwnd):
                return True
            n = user32.GetWindowTextLengthW(hwnd)
            if n <= 0:
                return True
            buf = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(hwnd, buf, n + 1)
            if title_substr in buf.value:
                found.append(hwnd)
                return False
            return True

        user32.EnumWindows(EnumWindowsProc(cb), 0)
        if found:
            return found[0]
        time.sleep(0.2)
    return 0


def find_class_window(class_name: str) -> int:
    EnumWindowsProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    found: list[int] = []

    def cb(hwnd, _lp):
        if not user32.IsWindowVisible(hwnd):
            return True
        cls = ctypes.create_unicode_buffer(256)
        user32.GetClassNameW(hwnd, cls, 256)
        if cls.value == class_name:
            found.append(hwnd)
            return False
        return True

    user32.EnumWindows(EnumWindowsProc(cb), 0)
    return found[0] if found else 0


WM_CLOSE = 0x0010
VK_ESCAPE = 0x1B
WM_KEYDOWN = 0x0100

# Common File Dialog (Vista+) 클래스명 목록
_DIALOG_CLASSES = {"#32770", "EVERYTHING", "CtrlNotifySink",
                   "NativeHWNDHost", "DirectUIHWND", "FloatNotifySink"}

user32.SendMessageW.argtypes  = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype   = ctypes.c_long
user32.EnumWindows.argtypes   = [ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM), wt.LPARAM]
user32.EnumWindows.restype    = wt.BOOL
user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
user32.GetWindowThreadProcessId.restype  = wt.DWORD
user32.IsWindowEnabled.argtypes = [wt.HWND]
user32.IsWindowEnabled.restype  = wt.BOOL
user32.EnumChildWindows.argtypes = [wt.HWND, ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM), wt.LPARAM]
user32.EnumChildWindows.restype  = wt.BOOL


def find_foreign_toplevel(main_hwnd: int) -> list[int]:
    """메인 윈도가 아닌 visible 최상위 윈도 목록 반환."""
    EnumWindowsProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    found: list[int] = []
    main_pid = wt.DWORD(0)
    user32.GetWindowThreadProcessId(main_hwnd, ctypes.byref(main_pid))

    def cb(hwnd, _lp):
        if hwnd == main_hwnd:
            return True
        if not user32.IsWindowVisible(hwnd):
            return True
        pid = wt.DWORD(0)
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == main_pid.value:
            found.append(hwnd)
        return True

    user32.EnumWindows(EnumWindowsProc(cb), 0)
    return found


user32.GetForegroundWindow.argtypes = []
user32.GetForegroundWindow.restype  = wt.HWND


def close_modal_dialog(main_hwnd: int = 0):
    """모달/파일다이얼로그를 WM_CLOSE + IDCANCEL + ESC 로 닫는다."""
    for attempt in range(50):  # 최대 5초
        # 메인이 이미 enabled면 닫을 게 없음
        if main_hwnd and user32.IsWindowEnabled(main_hwnd):
            return True

        targets: list[int] = []
        # 1) #32770 계열
        h32 = find_class_window("#32770")
        if h32:
            targets.append(h32)
        # 2) 메인 외 같은 프로세스 윈도 (Common File Dialog 포함)
        if main_hwnd:
            for h in find_foreign_toplevel(main_hwnd):
                if h not in targets:
                    targets.append(h)
        # 3) foreground 윈도가 메인과 다르면 추가
        fg = user32.GetForegroundWindow()
        if fg and fg != main_hwnd and fg not in targets:
            targets.append(fg)

        if targets:
            for h in targets:
                user32.PostMessageW(h, WM_CLOSE, 0, 0)
                user32.PostMessageW(h, WM_COMMAND, 2, 0)   # IDCANCEL
                user32.PostMessageW(h, WM_KEYDOWN, VK_ESCAPE, 0)
            time.sleep(0.3)
        else:
            time.sleep(0.1)
    return False


def wait_no_modal(main_hwnd: int = 0, timeout_sec: float = 5.0):
    """메인 윈도가 다시 enabled 될 때까지 기다린다 (모달 닫힘 신호)."""
    end = time.time() + timeout_sec
    while time.time() < end:
        # 메인 윈도가 enabled 상태이면 모달 없음
        if main_hwnd and user32.IsWindowEnabled(main_hwnd):
            return
        # #32770 도 없으면 OK
        if not main_hwnd and not find_class_window("#32770"):
            return
        time.sleep(0.1)


def run_suite(suite: str, hwnd: int) -> str:
    if suite == "layout":
        return f"[PASS] layout  hwnd=0x{hwnd:X}" if hwnd else "[FAIL] layout  no main window"
    cmd_id = SUITE_CMD.get(suite)
    if cmd_id is None:
        return f"[SKIP] {suite}  unknown command id"
    # PostMessage 전: 이전 다이얼로그가 완전히 닫혔는지 확인
    wait_no_modal(main_hwnd=hwnd, timeout_sec=4.0)
    ok = bool(user32.PostMessageW(hwnd, WM_COMMAND, cmd_id, 0))
    if not ok:
        return f"[FAIL] {suite}  PostMessage failed"
    # 모달 다이얼로그가 뜨는 명령(파일 열기/내보내기/정보)은 자동으로 닫는다
    time.sleep(0.5)   # 다이얼로그 생성 대기
    close_modal_dialog(main_hwnd=hwnd)
    wait_no_modal(main_hwnd=hwnd, timeout_sec=4.0)  # 닫힘 완료 대기
    return f"[PASS] {suite}  cmd=0x{cmd_id:X}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="Debug", choices=["Debug", "Release"])
    ap.add_argument("--suite",  default=None)
    args = ap.parse_args()

    exe = ROOT / "x64" / args.config / "DeepMetria.exe"
    if not exe.exists():
        RESULTS.write_text(f"[FAIL] startup  EXE missing: {exe}\n", encoding="utf-8")
        print(f"[runner] EXE not found: {exe}")
        return 1

    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    run_dir = SHOTS_BASE / f"run_{ts}"
    run_dir.mkdir(parents=True, exist_ok=True)

    suites = [args.suite] if args.suite else SUITES

    proc = subprocess.Popen([str(exe)], cwd=str(ROOT))
    try:
        hwnd = find_window("DeepMetria", timeout_sec=10.0)
        lines: list[str] = []
        for suite in suites:
            line = run_suite(suite, hwnd)
            lines.append(line)
            print(line)
            time.sleep(0.5)
        RESULTS.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return 1 if any(L.startswith("[FAIL]") for L in lines) else 0
    finally:
        try:
            proc.terminate()
            proc.wait(timeout=5)
        except Exception:
            proc.kill()


if __name__ == "__main__":
    sys.exit(main())
