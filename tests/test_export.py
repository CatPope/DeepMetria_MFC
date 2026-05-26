"""
내보내기 테스트 (E-01 ~ E-04)
내보내기 메뉴 접근 및 대화상자 컨트롤을 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
from config import FILE_LOAD_TIMEOUT


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


def test_E01_export_menu_message(app, win) -> bool:
    """E-01: 내보내기 메뉴 선택 시 안내 메시지 확인"""
    print("[E-01] 내보내기 메뉴 안내 메시지 확인")
    try:
        win.set_focus()
        try:
            win.menu_select("도구(&T)->내보내기(&E)...")
        except Exception:
            try:
                win.menu_select("도구->내보내기")
            except Exception:
                win.menu_select("도구->내보내기...")
        time.sleep(0.5)

        msg_text = _dismiss_any_dialog(app)
        ok = "차트" in msg_text or "선택" in msg_text
        print(f"  메시지: '{msg_text}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "E01", "export_menu_message")
        return ok
    except Exception as exc:
        print(f"  FAIL -- 예외: {exc}")
        return False


def test_E02_export_dialog_manual() -> bool:
    """E-02~E-04: 내보내기 대화상자 테스트 (차트 데이터 필요)"""
    print("[E-02~E-04_MANUAL] 내보내기 대화상자 컨트롤 테스트")
    print("  MANUAL -- 차트 생성 후 내보내기 대화상자 테스트 필요")
    print("  검증 항목: 형식 콤보(PNG/BMP/CSV), 경로, 크기 컨트롤, 실제 내보내기")
    return True


def run_suite(app, win) -> dict:
    results = {}
    results["E-01"] = test_E01_export_menu_message(app, win)
    results["E-02~E-04_MANUAL"] = test_E02_export_dialog_manual()
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== 내보내기 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")

    app.kill()
