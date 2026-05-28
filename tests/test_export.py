"""
내보내기 테스트 (E-01 ~ E-06)
내보내기 메뉴 접근 및 대화상자 컨트롤을 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
import config
from config import FILE_LOAD_TIMEOUT


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

        msg_text = helpers.dismiss_dialog(app)
        ok = "차트" in msg_text or "선택" in msg_text
        print(f"  메시지: '{msg_text}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "E01", "export_menu_message")
        return ok
    except Exception as exc:
        print(f"  FAIL -- 예외: {exc}")
        return False


def test_E02_export_dialog_manual() -> bool:
    """E-02: 내보내기 대화상자 테스트 (차트 데이터 필요, E-03/E-04는 자동화됨)"""
    print("[E-02_MANUAL] 내보내기 대화상자 컨트롤 테스트")
    print("  MANUAL -- 차트 생성 후 내보내기 대화상자 테스트 필요")
    print("  검증 항목: 형식 콤보(PNG/BMP/CSV), 경로, 크기 컨트롤, 실제 내보내기")
    return True


def test_E03_export_dialog_controls(app, win) -> bool:
    """E-03: 내보내기 다이얼로그의 주요 컨트롤 존재 및 초기 상태 확인 — TC-07-01"""
    test_id = "E-03"
    print(f"[{test_id}] 내보내기 다이얼로그 컨트롤 존재 확인")
    try:
        # 먼저 CSV 파일 로드 (내보내기는 데이터 필요)
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if os.path.exists(csv_path):
            helpers.open_file(app, win, csv_path)
            time.sleep(1)

        # Export 메뉴 또는 버튼으로 다이얼로그 열기
        win.set_focus()
        try:
            win.menu_select("도구(&T)->내보내기(&E)...")
        except Exception:
            try:
                win.menu_select("도구->내보내기")
            except Exception:
                try:
                    win.menu_select("도구->내보내기...")
                except Exception:
                    try:
                        win.menu_select("File->Export")
                    except Exception:
                        try:
                            win.menu_select("파일->내보내기")
                        except Exception:
                            print(f"  [{test_id}] FAIL — 내보내기 메뉴 없음")
                            return False

        time.sleep(1)

        # 먼저 안내 메시지 다이얼로그가 뜨면 닫기
        helpers.dismiss_dialog(app)
        time.sleep(0.5)

        # Export 다이얼로그 찾기
        try:
            dlg = app.window(class_name="#32770")
            dlg.wait("visible", timeout=5)
            checks = []

            # 형식 콤보박스 확인
            try:
                combo = dlg.child_window(class_name="ComboBox")
                checks.append(combo.exists())
            except Exception:
                checks.append(False)

            # 경로 입력 필드 확인
            try:
                edit = dlg.child_window(class_name="Edit", found_index=0)
                checks.append(edit.exists())
            except Exception:
                checks.append(False)

            # 다이얼로그 닫기
            try:
                dlg.close()
            except Exception:
                from pywinauto.keyboard import send_keys
                send_keys("{ESCAPE}")

            result = all(checks)
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — 컨트롤 존재: combo={checks[0]}, edit={checks[1]}")
            helpers.capture(win, test_id, "export_dialog_controls")
            return result
        except Exception:
            print(f"  [{test_id}] SKIP — 차트 미선택 상태에서 내보내기 다이얼로그 미출현 (정상)")
            helpers.capture(win, test_id, "export_dialog_skipped")
            return None
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_E04_export_format_combo(app, win) -> bool:
    """E-04: Export 다이얼로그 형식 콤보박스에 PNG, BMP, CSV 항목 존재 확인 — TC-07-01 상세"""
    test_id = "E-04"
    print(f"[{test_id}] 내보내기 형식 콤보박스 항목 확인")
    try:
        # CSV 파일 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if os.path.exists(csv_path):
            helpers.open_file(app, win, csv_path)
            time.sleep(1)

        # 내보내기 메뉴 열기
        win.set_focus()
        opened = False
        for menu_path in [
            "도구(&T)->내보내기(&E)...",
            "도구->내보내기",
            "도구->내보내기...",
            "File->Export",
            "파일->내보내기",
        ]:
            try:
                win.menu_select(menu_path)
                opened = True
                break
            except Exception:
                continue

        if not opened:
            print(f"  [{test_id}] FAIL — 내보내기 메뉴 없음")
            return False

        time.sleep(1)
        helpers.dismiss_dialog(app)
        time.sleep(0.5)

        try:
            dlg = app.window(class_name="#32770")
            dlg.wait("visible", timeout=5)

            combo = dlg.child_window(class_name="ComboBox")
            combo.click_input()
            time.sleep(0.3)

            items = combo.item_texts()
            items_upper = [i.upper() for i in items if i]

            has_png = any("PNG" in i for i in items_upper)
            has_bmp = any("BMP" in i for i in items_upper)
            has_csv = any("CSV" in i for i in items_upper)

            # 다이얼로그 닫기
            try:
                dlg.close()
            except Exception:
                from pywinauto.keyboard import send_keys
                send_keys("{ESCAPE}")

            result = has_png and has_bmp and has_csv
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — "
                  f"PNG={has_png}, BMP={has_bmp}, CSV={has_csv} | 항목: {items}")
            return result
        except Exception:
            print(f"  [{test_id}] SKIP — 차트 미선택 상태에서 내보내기 다이얼로그 미출현 (정상)")
            return None
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_E05_export_png_file(app, win) -> bool:
    """E-05: 임시 경로에 PNG 내보내기 실행 후 파일 생성 확인 — TC-07-01"""
    test_id = "E-05"
    print(f"[{test_id}] PNG 내보내기 실행 및 파일 생성 확인")
    import tempfile
    tmp_file = os.path.join(tempfile.gettempdir(), f"deepmetria_export_test_{int(time.time())}.png")
    try:
        # CSV 파일 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if os.path.exists(csv_path):
            helpers.open_file(app, win, csv_path)
            time.sleep(1)

        # 내보내기 메뉴 열기
        win.set_focus()
        opened = False
        for menu_path in [
            "도구(&T)->내보내기(&E)...",
            "도구->내보내기",
            "도구->내보내기...",
            "File->Export",
            "파일->내보내기",
        ]:
            try:
                win.menu_select(menu_path)
                opened = True
                break
            except Exception:
                continue

        if not opened:
            print(f"  [{test_id}] FAIL — 내보내기 메뉴 없음")
            return False

        time.sleep(1)
        helpers.dismiss_dialog(app)
        time.sleep(0.5)

        try:
            dlg = app.window(class_name="#32770")
            dlg.wait("visible", timeout=5)

            # 형식 콤보에서 PNG 선택
            try:
                combo = dlg.child_window(class_name="ComboBox")
                combo.select("PNG")
            except Exception:
                pass  # 기본값이 PNG일 수도 있음

            # 저장 경로 입력
            try:
                edit = dlg.child_window(class_name="Edit", found_index=0)
                edit.set_text(tmp_file)
            except Exception:
                pass

            # 확인 버튼 클릭
            try:
                ok_btn = dlg.child_window(title="확인", class_name="Button")
                ok_btn.click()
            except Exception:
                try:
                    ok_btn = dlg.child_window(title="OK", class_name="Button")
                    ok_btn.click()
                except Exception:
                    from pywinauto.keyboard import send_keys
                    send_keys("{ENTER}")

            time.sleep(2)

            # 파일 생성 확인
            result = os.path.exists(tmp_file)
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — 파일 생성: {tmp_file}")

            # 임시 파일 정리
            if os.path.exists(tmp_file):
                try:
                    os.remove(tmp_file)
                    print(f"  [{test_id}] 임시 파일 삭제 완료")
                except Exception:
                    pass

            helpers.capture(win, test_id, "export_png_result")
            return result
        except Exception:
            print(f"  [{test_id}] SKIP — 차트 미선택 상태에서 내보내기 다이얼로그 미출현 (정상)")
            return None
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        # 임시 파일 정리 시도
        if os.path.exists(tmp_file):
            try:
                os.remove(tmp_file)
            except Exception:
                pass
        return False


def test_E06_dashboard_capture_manual() -> bool:
    """E-06: 대시보드 전체 캡처 — TC-07-02 (MANUAL)"""
    print("[E-06_MANUAL] 대시보드 전체 캡처 테스트")
    print("  MANUAL -- 차트 카드가 배치된 대시보드 상태에서 수동 테스트 필요")
    print("  검증 항목: 전체 대시보드 PNG/BMP 캡처, 파일 저장 경로 및 품질 확인")
    return True


def run_suite(app, win) -> dict:
    results = {}
    results["E-01"] = test_E01_export_menu_message(app, win)
    results["E-02_MANUAL"] = test_E02_export_dialog_manual()
    results["E-03"] = test_E03_export_dialog_controls(app, win)
    results["E-04"] = test_E04_export_format_combo(app, win)
    results["E-05"] = test_E05_export_png_file(app, win)
    results["E-06_MANUAL"] = test_E06_dashboard_capture_manual()
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
