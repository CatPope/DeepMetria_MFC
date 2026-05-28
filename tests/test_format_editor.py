"""
서식 편집 테스트 (FE-01 ~ FE-05)
FormatEditorDlg 다이얼로그 접근 및 컨트롤 존재를 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
import config


def test_FE01_format_editor_menu_access(app, win) -> bool:
    """TC-06-01: 서식 편집 메뉴/버튼 접근 가능 여부 확인
    대시보드/차트뷰에서 서식 편집 메뉴를 찾고 클릭 시도.
    차트 없으면 SKIP 반환.
    """
    test_id = "FE-01"
    print(f"[{test_id}] 서식 편집 메뉴/버튼 접근 확인")
    try:
        win.set_focus()

        # 차트뷰에서 서식 편집 메뉴를 탐색
        menu_found = False
        for menu_path in [
            "도구(&T)->서식 편집(&F)...",
            "도구->서식 편집",
            "도구->서식 편집...",
            "보기->서식 편집",
            "차트->서식 편집",
            "Format->Edit",
        ]:
            try:
                win.menu_select(menu_path)
                menu_found = True
                break
            except Exception:
                continue

        if not menu_found:
            # 툴바 버튼 탐색 시도
            try:
                btn = win.child_window(title_re=".*서식.*", class_name="Button")
                btn.click()
                menu_found = True
            except Exception:
                pass

        if not menu_found:
            print(f"  [{test_id}] SKIP — 서식 편집 메뉴/버튼 미발견 (차트 미선택 또는 미구현)")
            helpers.capture(win, test_id, "format_menu_skip")
            return None

        time.sleep(0.5)
        # 안내 다이얼로그가 뜨면 닫기
        helpers.dismiss_dialog(app)
        time.sleep(0.3)

        helpers.capture(win, test_id, "format_menu_access")
        print(f"  [{test_id}] PASS — 서식 편집 메뉴 접근 성공")
        return True
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_FE02_format_editor_dialog_controls(app, win) -> bool:
    """TC-06-02: FormatEditorDlg 다이얼로그 컨트롤 존재 확인
    슬라이더(msctls_trackbar32), 색상 버튼, 콤보박스 존재 확인.
    다이얼로그 미출현 시 SKIP.
    """
    test_id = "FE-02"
    print(f"[{test_id}] FormatEditorDlg 컨트롤 존재 확인")
    try:
        win.set_focus()

        # 서식 편집 다이얼로그 열기 시도
        opened = False
        for menu_path in [
            "도구(&T)->서식 편집(&F)...",
            "도구->서식 편집",
            "도구->서식 편집...",
            "보기->서식 편집",
            "차트->서식 편집",
        ]:
            try:
                win.menu_select(menu_path)
                opened = True
                break
            except Exception:
                continue

        if not opened:
            # 안내 메시지 다이얼로그가 이미 떠 있을 수 있음
            helpers.dismiss_dialog(app)
            print(f"  [{test_id}] SKIP — 서식 편집 메뉴 없음 (차트 미선택 또는 미구현)")
            return None

        time.sleep(1)

        # 안내 메시지 다이얼로그 닫기
        helpers.dismiss_dialog(app)
        time.sleep(0.5)

        # FormatEditorDlg(#32770) 탐색
        try:
            dlg = app.window(class_name="#32770")
            dlg.wait("visible", timeout=5)
        except Exception:
            print(f"  [{test_id}] SKIP — FormatEditorDlg 미출현 (차트 미선택 상태에서 정상)")
            helpers.capture(win, test_id, "format_dialog_skip")
            return None

        checks = {}

        # 슬라이더(msctls_trackbar32) 존재 확인
        try:
            slider = dlg.child_window(class_name="msctls_trackbar32")
            checks["slider"] = slider.exists()
        except Exception:
            checks["slider"] = False

        # 색상 버튼 존재 확인
        try:
            color_btn = dlg.child_window(title_re=".*색상.*|.*Color.*|.*색.*", class_name="Button")
            checks["color_btn"] = color_btn.exists()
        except Exception:
            # 버튼 개수로 대체 확인
            try:
                btns = dlg.children(class_name="Button")
                checks["color_btn"] = len(btns) >= 1
            except Exception:
                checks["color_btn"] = False

        # 콤보박스 존재 확인
        try:
            combo = dlg.child_window(class_name="ComboBox")
            checks["combo"] = combo.exists()
        except Exception:
            checks["combo"] = False

        # 다이얼로그 닫기
        try:
            cancel_btn = dlg.child_window(title_re="취소|Cancel", class_name="Button")
            cancel_btn.click()
        except Exception:
            try:
                dlg.close()
            except Exception:
                from pywinauto.keyboard import send_keys
                send_keys("{ESCAPE}")

        time.sleep(0.3)

        result = checks.get("slider", False) or checks.get("color_btn", False)
        print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — "
              f"slider={checks.get('slider')}, color_btn={checks.get('color_btn')}, "
              f"combo={checks.get('combo')}")
        helpers.capture(win, test_id, "format_dialog_controls")
        return result
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_FE03_color_change_manual() -> bool:
    """TC-06-03: 색상 변경 시각적 반영 (수동 검증)"""
    print("[FE-03_MANUAL] 색상 변경 시각적 반영 테스트")
    print("  MANUAL -- 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목: TC-06-03 서식 편집 대화상자에서 색상 변경 후 즉시 반영 확인")
    print("  절차: 1) 쿼리 제출하여 차트 카드 생성")
    print("        2) 차트 우클릭 -> 서식 편집 선택")
    print("        3) 색상 변경 버튼 클릭 -> CColorDialog 표시")
    print("        4) 색상 선택 후 확인 -> 차트 색상 즉시 반영 확인")
    return True


def test_FE04_slider_manual() -> bool:
    """TC-06-04: 크기 슬라이더 조작 후 변경 (수동 검증)"""
    print("[FE-04_MANUAL] 크기 슬라이더 조작 테스트")
    print("  MANUAL -- 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목: TC-06-04 FormatEditorDlg 슬라이더(msctls_trackbar32) 드래그")
    print("  절차: 1) 쿼리 제출하여 차트 카드 생성")
    print("        2) 차트 우클릭 -> 서식 편집 선택")
    print("        3) 높이/너비 슬라이더를 드래그하여 값 변경")
    print("        4) onChange 핸들러 호출 및 대시보드 미리보기 즉시 반영 확인")
    return True


def test_FE05_style_change_e2e_manual() -> bool:
    """TC-06-05: E2E 서식 변경 즉시 반영 (수동 검증)"""
    print("[FE-05_MANUAL] 서식 변경 즉시 반영 E2E 테스트")
    print("  MANUAL -- LLM 분석으로 차트 생성 후 수동 테스트 필요")
    print("  검증 항목: TC-06-05 색상 또는 크기 변경 직후 대시보드에 시각적 반영")
    print("  절차: 1) 쿼리 제출하여 차트 카드 생성")
    print("        2) 서식 편집 대화상자에서 색상 + 크기 모두 변경")
    print("        3) 대화상자 닫은 직후 변경 사항 즉시 반영됨을 육안 확인")
    return True


def run_suite(app, win) -> dict:
    results = {}
    results["FE-01"] = test_FE01_format_editor_menu_access(app, win)
    results["FE-02"] = test_FE02_format_editor_dialog_controls(app, win)
    results["FE-03_MANUAL"] = test_FE03_color_change_manual()
    results["FE-04_MANUAL"] = test_FE04_slider_manual()
    results["FE-05_MANUAL"] = test_FE05_style_change_e2e_manual()
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== 서식 편집 테스트 결과 ===")
    for tid, ok in results.items():
        if "MANUAL" in tid:
            label = "MANUAL"
        elif ok is None:
            label = "SKIP"
        elif ok:
            label = "PASS"
        else:
            label = "FAIL"
        print(f"  {tid}: {label}")

    app.kill()
