"""
JSON 파일 로드 테스트 (J-01 ~ J-04)
JSON 파일 열기, 타이틀바 업데이트, DataSummaryView 반영, 오류 처리를 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
from config import FILE_LOAD_TIMEOUT

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
SAMPLE_DATA = os.path.join(DATA_DIR, "sample_data.json")
SAMPLE_EMPTY = os.path.join(DATA_DIR, "sample_empty.json")
SAMPLE_MALFORMED = os.path.join(DATA_DIR, "sample_malformed.json")


def _dismiss_any_dialog(app) -> str:
    try:
        dlg = app.window(class_name="#32770")
        dlg.wait("visible", timeout=3)
        text = ""
        for idx in range(5):
            try:
                static = dlg.child_window(class_name="Static", found_index=idx)
                t = static.window_text()
                if t:
                    text = t
                    break
            except Exception:
                break
        if not text:
            text = dlg.window_text()
        dlg.type_keys("{ENTER}")
        return text
    except Exception:
        return ""


def test_J01_json_title_bar(app, win) -> bool:
    print("[J-01] JSON 로드 후 타이틀바 파일명 확인")
    try:
        helpers.open_file(app, win, SAMPLE_DATA)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)
        title = win.window_text()
        filename = os.path.basename(SAMPLE_DATA)
        base = os.path.splitext(filename)[0]
        ok = filename in title or base in title
        print(f"  타이틀: '{title}'")
        print(f"  파일명: '{filename}'")
        print(f"  PASS" if ok else "  FAIL")
        helpers.capture(win, "J01", "json_title_bar")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_J02_json_listview_items(win) -> bool:
    print("[J-02] JSON 로드 후 ListView 항목 존재 확인")
    try:
        lv = win.child_window(class_name="SysListView32")
        item_count = lv.item_count()
        ok = item_count > 0
        print(f"  항목 수: {item_count}")
        print(f"  PASS — 기대: > 0" if ok else f"  FAIL — 기대: > 0, 실제: {item_count}")
        helpers.capture(win, "J02", "json_listview_items")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_J03_empty_json(app, win) -> bool:
    print("[J-03] 빈 JSON 파일 로드 후 오류 없음 확인")
    try:
        helpers.open_file(app, win, SAMPLE_EMPTY)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)
        status = helpers.get_status_text(win)
        ok = "오류" not in status and "error" not in status.lower()
        print(f"  상태 텍스트: '{status}'")
        print(f"  PASS — 오류 없이 로드됨" if ok else "  FAIL — 오류 감지됨")
        helpers.capture(win, "J03", "empty_json")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_J04_malformed_json(app, win) -> bool:
    print("[J-04] 손상된 JSON 파일 로드 시 오류 처리 확인")
    try:
        helpers.open_file(app, win, SAMPLE_MALFORMED)
        time.sleep(1)
        msg_text = _dismiss_any_dialog(app)
        status = helpers.get_status_text(win)
        ok = (
            "오류" in msg_text
            or "error" in msg_text.lower()
            or "실패" in msg_text
            or "오류" in status
            or "실패" in status
            or "error" in status.lower()
        )
        print(f"  메시지박스 텍스트: '{msg_text}'")
        print(f"  상태 텍스트: '{status}'")
        print(f"  PASS" if ok else "  FAIL")
        helpers.capture(win, "J04", "malformed_json")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def run_suite(app, win) -> dict:
    results = {}
    results["J-01"] = test_J01_json_title_bar(app, win)
    results["J-02"] = test_J02_json_listview_items(win)
    results["J-03"] = test_J03_empty_json(app, win)
    results["J-04"] = test_J04_malformed_json(app, win)
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)
    results = run_suite(app, win)
    print("\n=== JSON 로드 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")
    app.kill()
