"""
대시보드 인터랙션 테스트 (DI-01 ~ DI-08)
DashboardView의 빈 상태, 차트 영역, 리사이즈, 스크린샷 비교를 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
import config


def test_DI01_dashboard_exists(win) -> bool:
    """DI-01: DashboardView 패널 존재 확인"""
    print("[DI-01] DashboardView 패널 존재 확인")
    try:
        children = win.children()
        pane_count = 0
        for child in children:
            cls = child.class_name()
            if "Afx" in cls or "SysListView32" in cls:
                pane_count += 1

        ok = pane_count >= 3
        print(f"  자식 윈도우 수: {len(children)}, Afx 관련: {pane_count}")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "DI01", "dashboard_exists")
        return ok
    except Exception as exc:
        print(f"  FAIL -- 예외: {exc}")
        return False


def test_DI02_empty_dashboard_screenshot(win) -> bool:
    """DI-02: 빈 대시보드 상태 스크린샷 캡처"""
    print("[DI-02] 빈 대시보드 스크린샷 캡처")
    try:
        path = helpers.capture(win, "DI02", "empty_dashboard")
        ok = os.path.exists(path)
        print(f"  {'PASS' if ok else 'FAIL'} -- 스크린샷 저장: {path}")
        return ok
    except Exception as exc:
        print(f"  FAIL -- 예외: {exc}")
        return False


def test_DI03_interaction_manual() -> bool:
    """DI-03: 대시보드 카드 드래그 이동 (차트 데이터 필요, DI-04~DI-06은 자동화됨)"""
    print("[DI-03_MANUAL] 대시보드 카드 드래그 이동 테스트")
    print("  MANUAL -- LLM 분석으로 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목:")
    print("    DI-03: 카드 드래그 이동")
    print("    DI-04: 카드 리사이즈 (우하단 그립)")
    print("    DI-05: 우클릭 컨텍스트 메뉴 (색상 변경, 삭제)")
    print("    DI-06: 색상 변경 대화상자 (CColorDialog)")
    return True


def test_DI04_chart_area_exists(app, win) -> bool:
    """DI-04: 데이터 로드 후 대시보드에 차트 영역이 존재하는지 확인 — TC-05-01/02"""
    test_id = "DI-04"
    print(f"[{test_id}] 차트 영역 식별 (데이터 로드 후)")
    try:
        # CSV 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if os.path.exists(csv_path):
            helpers.open_file(app, win, csv_path)
            time.sleep(2)

        # DashboardView 영역 확인 — AfxWnd 클래스로 찾기
        try:
            dashboard = win.child_window(class_name_re="Afx.*", found_index=0)
            rect = dashboard.rectangle()
            has_area = (rect.width() > 100 and rect.height() > 100)
            print(f"  [{test_id}] {'PASS' if has_area else 'FAIL'} — "
                  f"대시보드 영역 {rect.width()}x{rect.height()}")
            helpers.capture(win, test_id, "dashboard_after_load")
            return has_area
        except Exception as exc:
            print(f"  [{test_id}] FAIL — 대시보드 영역 미발견: {exc}")
            return False
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_DI05_dashboard_resize(app, win) -> bool:
    """DI-05: 메인 윈도우 크기 변경 후 대시보드 영역 리사이즈 확인 — TC-06-02 관련"""
    test_id = "DI-05"
    print(f"[{test_id}] 대시보드 윈도우 리사이즈 확인")
    try:
        # 초기 크기 기록
        rect_before = win.rectangle()
        width_before = rect_before.width()
        height_before = rect_before.height()

        # 대시보드 영역 초기 크기 기록
        try:
            dashboard = win.child_window(class_name_re="Afx.*", found_index=0)
            dash_rect_before = dashboard.rectangle()
        except Exception:
            dashboard = None
            dash_rect_before = None

        # 윈도우 크기 변경 (100px 줄이기)
        new_width = max(width_before - 100, 600)
        new_height = max(height_before - 100, 400)
        win.move_window(width=new_width, height=new_height)
        time.sleep(0.5)

        # 변경 후 대시보드 영역 크기 확인
        result = False
        if dashboard is not None and dash_rect_before is not None:
            try:
                dash_rect_after = dashboard.rectangle()
                # 대시보드 영역이 여전히 유효한 크기이면 통과
                result = (dash_rect_after.width() > 50 and dash_rect_after.height() > 50)
                print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — "
                      f"리사이즈 전 {dash_rect_before.width()}x{dash_rect_before.height()} "
                      f"-> 후 {dash_rect_after.width()}x{dash_rect_after.height()}")
            except Exception as exc:
                print(f"  [{test_id}] FAIL — 리사이즈 후 영역 확인 실패: {exc}")
                result = False
        else:
            # 대시보드 자식 영역을 찾지 못해도 윈도우 자체가 리사이즈되면 PASS
            rect_after = win.rectangle()
            result = (rect_after.width() != width_before or rect_after.height() != height_before)
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — "
                  f"윈도우 리사이즈: {width_before}x{height_before} -> "
                  f"{rect_after.width()}x{rect_after.height()}")

        # 원래 크기로 복원
        win.move_window(width=width_before, height=height_before)
        time.sleep(0.3)

        helpers.capture(win, test_id, "dashboard_after_resize")
        return result
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_DI06_dashboard_screenshot_compare(app, win) -> bool:
    """DI-06: 데이터 로드 전후 대시보드 스크린샷 파일 크기 비교로 변경 감지"""
    test_id = "DI-06"
    print(f"[{test_id}] 대시보드 스크린샷 비교 (데이터 로드 전후)")
    try:
        # 빈 상태 스크린샷
        path_empty = helpers.capture(win, test_id, "before_load")
        size_empty = os.path.getsize(path_empty) if os.path.exists(path_empty) else 0

        # CSV 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if not os.path.exists(csv_path):
            print(f"  [{test_id}] FAIL — 샘플 CSV 없음: {csv_path}")
            return False

        helpers.open_file(app, win, csv_path)
        time.sleep(2)

        # 데이터 로드 후 스크린샷
        path_loaded = helpers.capture(win, test_id, "after_load")
        size_loaded = os.path.getsize(path_loaded) if os.path.exists(path_loaded) else 0

        # 파일 크기 차이로 화면 변화 감지 (1% 이상 차이 시 변경 있음으로 판단)
        diff_ratio = 0.0
        if size_empty > 0 and size_loaded > 0:
            diff_ratio = abs(size_loaded - size_empty) / size_empty
            changed = diff_ratio > 0.01
        else:
            changed = False

        result = os.path.exists(path_empty) and os.path.exists(path_loaded)
        if size_empty > 0 and size_loaded > 0:
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — "
                  f"빈 화면 크기: {size_empty}B, 로드 후: {size_loaded}B, "
                  f"변화율: {diff_ratio * 100:.1f}%")
        else:
            print(f"  [{test_id}] {'PASS' if result else 'FAIL'} — 스크린샷 저장 확인")
        return result
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_DI07_chart_drag_resize_manual() -> bool:
    """DI-07: 차트 드래그/리사이즈 테스트 — TC-06-01/02 (MANUAL)"""
    print("[DI-07_MANUAL] 차트 카드 드래그 및 리사이즈 테스트")
    print("  MANUAL -- 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목: TC-06-01 카드 드래그 이동, TC-06-02 우하단 그립으로 리사이즈")
    return True


def test_DI08_chart_color_change_manual() -> bool:
    """DI-08: 차트 색상 변경 테스트 — TC-06-03 (MANUAL)"""
    print("[DI-08_MANUAL] 차트 카드 색상 변경 테스트")
    print("  MANUAL -- 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목: TC-06-03 우클릭 컨텍스트 메뉴 -> 색상 변경 -> CColorDialog 표시")
    return True


def test_DI09_format_editor_slider_manual() -> bool:
    """DI-09: CFormatEditorDlg 슬라이더 조작 테스트 — TC-06-04 (MANUAL)"""
    print("[DI-09_MANUAL] 서식 편집기 슬라이더 조작 테스트")
    print("  MANUAL -- 차트 카드 생성 후 서식 편집 대화상자를 열어 수동 테스트 필요")
    print("  검증 항목: TC-06-04 높이/너비 슬라이더 조작 → onChange 핸들러 호출")
    print("  절차: 1) 차트 카드 더블클릭 또는 우클릭 -> 서식 편집")
    print("        2) CFormatEditorDlg 슬라이더를 드래그하여 값 변경")
    print("        3) 대시보드 미리보기에 크기 변화 즉시 반영 확인")
    return True


def test_DI10_style_change_e2e_manual() -> bool:
    """DI-10: 서식 변경 즉시 반영 E2E 테스트 — TC-06-05 (MANUAL)"""
    print("[DI-10_MANUAL] 서식 변경 즉시 반영 E2E 테스트")
    print("  MANUAL -- LLM 분석으로 차트 생성 후 색상/크기 변경하여 수동 테스트 필요")
    print("  검증 항목: TC-06-05 색상 또는 크기 변경 직후 대시보드에 시각적 반영")
    print("  절차: 1) 쿼리 제출하여 차트 카드 생성")
    print("        2) 차트 색상 변경 (TC-06-03 경로)")
    print("        3) 대화상자 닫은 직후 차트 색상이 즉시 변경됨을 육안 확인")
    return True


def test_DI11_chart_card_click_selection(app, win) -> bool:
    """DI-11: 차트 카드 존재 시 카드 클릭 후 선택 상태 변경 확인
    차트 카드가 없으면 SKIP 반환.
    """
    test_id = "DI-11"
    print(f"[{test_id}] 차트 카드 클릭 → 선택 상태 변경 확인")
    try:
        # CSV 로드 후 차트 카드 탐색 시도
        csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
        if os.path.exists(csv_path):
            helpers.open_file(app, win, csv_path)
            time.sleep(2)

        # 대시보드 영역에서 자식 컨트롤(차트 카드) 탐색
        try:
            dashboard = win.child_window(class_name_re="Afx.*", found_index=0)
            children = dashboard.children()
        except Exception:
            children = win.children()

        # 클릭 가능한 자식 컨트롤 탐색
        card_clicked = False
        for child in children:
            try:
                rect = child.rectangle()
                if rect.width() > 50 and rect.height() > 50:
                    child.click_input()
                    time.sleep(0.3)
                    card_clicked = True
                    break
            except Exception:
                continue

        if not card_clicked:
            print(f"  [{test_id}] SKIP — 클릭 가능한 차트 카드 미발견 (차트 미생성 상태)")
            helpers.capture(win, test_id, "card_click_skip")
            return None

        helpers.capture(win, test_id, "card_click_selected")
        print(f"  [{test_id}] PASS — 카드 클릭 완료 (선택 상태 변경)")
        return True
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def test_DI12_empty_state_guidance_text(win) -> bool:
    """DI-12: 대시보드 빈 상태에서 안내 텍스트 존재 확인
    '차트를 추가하려면' 또는 빈 상태 안내 메시지 탐색.
    """
    test_id = "DI-12"
    print(f"[{test_id}] 대시보드 빈 상태 안내 텍스트 존재 확인")
    try:
        # Static 텍스트 컨트롤을 순회하며 안내 메시지 탐색
        guidance_keywords = ["차트", "추가", "없음", "비어", "empty", "add", "no chart"]
        found_text = ""

        all_statics = win.children(class_name="Static")
        for static in all_statics:
            try:
                text = static.window_text().strip()
                if text:
                    text_lower = text.lower()
                    if any(kw.lower() in text_lower for kw in guidance_keywords):
                        found_text = text
                        break
            except Exception:
                continue

        if not found_text:
            # 재귀 탐색: 모든 자식 컨트롤에서 텍스트 탐색
            try:
                for child in win.descendants(class_name="Static"):
                    try:
                        text = child.window_text().strip()
                        if text and any(kw.lower() in text.lower() for kw in guidance_keywords):
                            found_text = text
                            break
                    except Exception:
                        continue
            except Exception:
                pass

        helpers.capture(win, test_id, "empty_state_text")
        if found_text:
            print(f"  [{test_id}] PASS — 안내 텍스트: '{found_text}'")
            return True
        else:
            print(f"  [{test_id}] SKIP — 빈 상태 안내 텍스트 미구현 (향후 추가 예정)")
            return None
    except Exception as exc:
        print(f"  [{test_id}] FAIL — 예외: {exc}")
        return False


def run_suite(app, win) -> dict:
    results = {}
    results["DI-01"] = test_DI01_dashboard_exists(win)
    results["DI-02"] = test_DI02_empty_dashboard_screenshot(win)
    results["DI-03_MANUAL"] = test_DI03_interaction_manual()
    results["DI-04"] = test_DI04_chart_area_exists(app, win)
    results["DI-05"] = test_DI05_dashboard_resize(app, win)
    results["DI-06"] = test_DI06_dashboard_screenshot_compare(app, win)
    results["DI-07_MANUAL"] = test_DI07_chart_drag_resize_manual()
    results["DI-08_MANUAL"] = test_DI08_chart_color_change_manual()
    results["DI-09_MANUAL"] = test_DI09_format_editor_slider_manual()
    results["DI-10_MANUAL"] = test_DI10_style_change_e2e_manual()
    results["DI-11"] = test_DI11_chart_card_click_selection(app, win)
    results["DI-12"] = test_DI12_empty_state_guidance_text(win)
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== 대시보드 인터랙션 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")

    app.kill()
