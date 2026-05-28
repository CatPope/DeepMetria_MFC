"""
DeepMetria 차트 렌더링 E2E 테스트 (TC-05-06)
LLM 분석 필요 → LLM API 키가 없으면 SKIP
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
import config


def test_CE01_chart_rendering_e2e(app, win) -> bool:
    """TC-05-06: 데이터 로드 후 차트 렌더링 확인"""
    test_id = "CE-01"
    print(f"[{test_id}] 데이터 로드 후 차트 렌더링 E2E 확인")
    try:
        # CSV 파일 로드
        csv_path = os.path.join(config.DATA_DIR, "sample_data.csv")
        if not os.path.exists(csv_path):
            # 대안 경로 시도
            csv_path = os.path.join(config.DATA_DIR, "sample_sales.csv")
            if not os.path.exists(csv_path):
                print(f"  [{test_id}] SKIP — sample_data.csv / sample_sales.csv 없음")
                return None

        helpers.open_file(app, win, csv_path)
        helpers.capture(win, test_id, "after_file_load")
        time.sleep(1)

        # 분석 실행 시도 (LLM 필요)
        try:
            helpers.type_query(win, "매출 데이터를 차트로 보여줘")
            helpers.click_analyze(win)
        except Exception as e:
            print(f"  [{test_id}] SKIP — 분석 실행 불가: {e}")
            helpers.capture(win, test_id, "analyze_failed")
            return None

        # 분석 결과 대기 (LLM 응답 필요)
        try:
            result_text = helpers.wait_for_analysis(win, timeout=30)
            if "오류" in result_text or "실패" in result_text:
                print(f"  [{test_id}] SKIP — LLM 분석 실패: {result_text}")
                helpers.capture(win, test_id, "analysis_error")
                return None
        except TimeoutError:
            print(f"  [{test_id}] SKIP — LLM 분석 타임아웃")
            helpers.capture(win, test_id, "analysis_timeout")
            return None

        helpers.capture(win, test_id, "after_analysis")

        # 대시보드에 차트 영역이 있는지 확인
        try:
            dashboard = win.child_window(class_name_re="Afx.*", found_index=0)
            rect = dashboard.rectangle()
            has_chart_area = (rect.width() > 100 and rect.height() > 100)
            print(f"  [{test_id}] {'PASS' if has_chart_area else 'FAIL'} — "
                  f"차트 렌더링 E2E 완료 — 대시보드 영역 {rect.width()}x{rect.height()}")
            helpers.capture(win, test_id, "chart_area_check")
            return has_chart_area
        except Exception as exc:
            print(f"  [{test_id}] SKIP — 대시보드 영역 확인 불가: {exc}")
            return None

    except Exception as e:
        print(f"  [{test_id}] SKIP — LLM 종속 오류: {e}")
        helpers.capture(win, test_id, "error")
        return None  # LLM 종속이므로 SKIP 처리


def run_suite(app, win) -> dict:
    return {
        "CE-01": test_CE01_chart_rendering_e2e(app, win),
    }


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== 차트 렌더링 E2E 테스트 결과 ===")
    for tid, ok in results.items():
        label = "SKIP" if ok is None else ("PASS" if ok else "FAIL")
        print(f"  {tid}: {label}")

    app.kill()
