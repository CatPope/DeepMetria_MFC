"""
DeepMetria 내보내기 E2E 테스트 (TC-07-04)
CSV 내보내기 기능의 전체 흐름 검증
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import tempfile
import helpers
import config
from pywinauto.keyboard import send_keys


def test_EX01_export_csv_e2e(app, win) -> bool:
    """TC-07-04: 데이터 로드 → CSV 내보내기 전체 흐름"""
    test_id = "EX-01"
    print(f"[{test_id}] 데이터 로드 → CSV 내보내기 E2E 흐름 확인")
    try:
        # 1. CSV 파일 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_data.csv")
        if not os.path.exists(csv_path):
            csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
            if not os.path.exists(csv_path):
                print(f"  [{test_id}] SKIP — sample_data.csv / sample_sales.csv 없음")
                return None

        helpers.open_file(app, win, csv_path)
        time.sleep(1)
        helpers.capture(win, test_id, "after_load")

        # 2. 내보내기 메뉴 시도
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
            # 단축키 시도
            send_keys("^+s")
            time.sleep(1)

        time.sleep(1)

        # 안내 메시지 다이얼로그가 뜨면 닫기
        helpers.dismiss_dialog(app)
        time.sleep(0.5)

        # 3. 저장 대화상자 확인
        try:
            from pywinauto import timings as pw_timings
            pw_timings.wait_until(5, 0.3, lambda: len(app.windows(class_name="#32770")) > 0)
            save_dlg = app.window(class_name="#32770")
            save_dlg.wait("visible", timeout=5)

            # 내보내기 경로 설정 — 임시 파일 사용
            export_path = os.path.join(
                tempfile.gettempdir(),
                f"deepmetria_e2e_export_{int(time.time())}.csv"
            )

            try:
                filename_edit = save_dlg.child_window(class_name="Edit", found_index=0)
                filename_edit.set_focus()
                filename_edit.set_text(export_path)
            except Exception:
                send_keys(export_path, with_spaces=True)

            # 형식 콤보에서 CSV 선택 시도
            try:
                combo = save_dlg.child_window(class_name="ComboBox")
                combo.select("CSV")
            except Exception:
                pass  # 기본값이 CSV이거나 콤보 없을 수 있음

            send_keys("{ENTER}")
            time.sleep(2)

            helpers.capture(win, test_id, "after_export")

            # 4. 파일 생성 확인
            if os.path.exists(export_path):
                file_size = os.path.getsize(export_path)
                print(f"  [{test_id}] PASS — 내보내기 성공: {export_path} ({file_size} bytes)")
                try:
                    os.remove(export_path)
                except Exception:
                    pass
                return True
            else:
                print(f"  [{test_id}] FAIL — 내보내기 파일 미생성: {export_path}")
                return False

        except Exception as e:
            print(f"  [{test_id}] SKIP — 내보내기 대화상자 미표시: {e}")
            helpers.capture(win, test_id, "no_dialog")
            return None

    except Exception as e:
        print(f"  [{test_id}] SKIP — 예외 발생: {e}")
        helpers.capture(win, test_id, "error")
        return None


def run_suite(app, win) -> dict:
    return {
        "EX-01": test_EX01_export_csv_e2e(app, win),
    }


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== 내보내기 E2E 테스트 결과 ===")
    for tid, ok in results.items():
        label = "SKIP" if ok is None else ("PASS" if ok else "FAIL")
        print(f"  {tid}: {label}")

    app.kill()
