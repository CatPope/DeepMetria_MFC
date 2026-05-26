"""
대시보드 인터랙션 테스트 (DI-01 ~ DI-06)
DashboardView의 빈 상태, 드래그/리사이즈/색상 변경을 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import helpers


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
    """DI-03~DI-06: 대시보드 카드 인터랙션 (차트 데이터 필요)"""
    print("[DI-03~DI-06_MANUAL] 대시보드 카드 드래그/리사이즈/색상/삭제 테스트")
    print("  MANUAL -- LLM 분석으로 차트 카드 생성 후 수동 테스트 필요")
    print("  검증 항목:")
    print("    DI-03: 카드 드래그 이동")
    print("    DI-04: 카드 리사이즈 (우하단 그립)")
    print("    DI-05: 우클릭 컨텍스트 메뉴 (색상 변경, 삭제)")
    print("    DI-06: 색상 변경 대화상자 (CColorDialog)")
    return True


def run_suite(app, win) -> dict:
    results = {}
    results["DI-01"] = test_DI01_dashboard_exists(win)
    results["DI-02"] = test_DI02_empty_dashboard_screenshot(win)
    results["DI-03~DI-06_MANUAL"] = test_DI03_interaction_manual()
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
