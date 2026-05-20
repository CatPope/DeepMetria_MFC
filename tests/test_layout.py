"""
레이아웃 테스트 (L-01 ~ L-09)
초기 실행 후 윈도우 크기, 패널 구성, 상태바 텍스트를 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import helpers
from config import APP_START_TIMEOUT


def test_L01_window_size(win) -> bool:
    """L-01: 윈도우 크기가 대략 1280x800 인지 확인한다."""
    print("[L-01] 윈도우 크기 확인")
    try:
        rect = win.rectangle()
        width = rect.width()
        height = rect.height()
        print(f"  실제 크기: {width}x{height}")

        # 허용 오차 ±200px
        ok = (1080 <= width <= 1480) and (600 <= height <= 1000)
        print(f"  {'PASS' if ok else 'FAIL'} — 기대: ~1280x800")
        helpers.capture(win, "L01", "window_size")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_L02_layout_screenshot(win) -> bool:
    """L-02: 3패널 레이아웃 스크린샷을 캡처해 시각적 검증용으로 저장한다."""
    print("[L-02] 레이아웃 스크린샷 캡처")
    try:
        path = helpers.capture(win, "L02", "three_panel_layout")
        ok = os.path.exists(path)
        print(f"  {'PASS' if ok else 'FAIL'} — 저장 경로: {path}")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_L09_statusbar_text(win) -> bool:
    """L-09: 상태바(또는 Static 상태 텍스트)에 '준비' 가 포함되어 있는지 확인한다."""
    print("[L-09] 상태바 '준비' 텍스트 확인")
    try:
        # MFC 상태바: class_name "msctls_statusbar32"
        try:
            statusbar = win.child_window(class_name="msctls_statusbar32")
            text = statusbar.window_text()
        except Exception:
            text = helpers.get_status_text(win)

        print(f"  상태 텍스트: '{text}'")
        ok = "준비" in text
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "L09", "statusbar")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def run_suite(win) -> dict:
    """레이아웃 테스트 스위트를 실행하고 결과 dict 를 반환한다."""
    results = {}
    results["L-01"] = test_L01_window_size(win)
    results["L-02"] = test_L02_layout_screenshot(win)
    results["L-09"] = test_L09_statusbar_text(win)
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=APP_START_TIMEOUT)

    results = run_suite(win)

    print("\n=== 레이아웃 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")

    app.kill()
