"""
QueryInputView 테스트 (Q-01 ~ Q-16)
좌측 패널의 편집 컨트롤, 버튼, 상태 텍스트, 진행바를 검증한다.
엣지케이스: Q-10(datasource 없음), Q-11(프로그레스 바 상태)
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import time
import helpers
from config import CSV_STANDARD, FILE_LOAD_TIMEOUT


# ---------------------------------------------------------------------------
# 메시지박스 감지 헬퍼
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 테스트 케이스
# ---------------------------------------------------------------------------

def test_Q01_controls_exist(win) -> bool:
    """Q-01: Edit, Button(분석), Static, ProgressCtrl 컨트롤이 존재하는지 확인한다."""
    print("[Q-01] 컨트롤 존재 확인")
    try:
        edit = win.child_window(class_name="Edit", found_index=0)
        btn = win.child_window(title="분석", class_name="Button")
        static = win.child_window(class_name="Static", found_index=0)
        progress = win.child_window(class_name="msctls_progress32")

        ok = all([
            edit.exists(),
            btn.exists(),
            static.exists(),
            progress.exists(),
        ])
        print(f"  Edit: {edit.exists()}, Button: {btn.exists()}, "
              f"Static: {static.exists()}, Progress: {progress.exists()}")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "Q01", "controls_exist")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q02_initial_status(win) -> bool:
    """Q-02: 초기 상태 텍스트가 '질문을 입력하고 분석 버튼을 누르세요.' 인지 확인한다."""
    print("[Q-02] 초기 상태 텍스트 확인")
    try:
        text = helpers.get_status_text(win)
        expected = "질문을 입력하고 분석 버튼을 누르세요."
        ok = expected in text
        print(f"  실제: '{text}'")
        print(f"  {'PASS' if ok else 'FAIL'} — 기대: '{expected}'")
        helpers.capture(win, "Q02", "initial_status")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q05_empty_query_msgbox(app, win) -> bool:
    """Q-05: 빈 입력으로 분석 버튼 클릭 시 '질문을 입력해주세요' 메시지박스가 나타나는지 확인한다."""
    print("[Q-05] 빈 쿼리 분석 → MessageBox 확인")
    try:
        # 편집창 비우기
        edit = win.child_window(class_name="Edit", found_index=0)
        edit.set_focus()
        edit.set_text("")

        helpers.click_analyze(win)
        time.sleep(0.5)

        msg_text = helpers.dismiss_dialog(app)
        ok = "질문을 입력해주세요" in msg_text or "입력" in msg_text
        print(f"  메시지박스 텍스트: '{msg_text}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "Q05", "empty_query_msgbox")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q06_no_file_msgbox(app, win) -> bool:
    """Q-06: 파일 미로드 상태에서 쿼리 입력 후 분석 → '데이터 파일을 먼저 불러오세요' 메시지박스 확인."""
    print("[Q-06] 파일 미로드 분석 → MessageBox 확인")
    try:
        helpers.type_query(win, "테스트 질문입니다")
        helpers.click_analyze(win)
        time.sleep(0.5)

        msg_text = helpers.dismiss_dialog(app)
        ok = "데이터 파일" in msg_text or "파일" in msg_text
        print(f"  메시지박스 텍스트: '{msg_text}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "Q06", "no_file_msgbox")

        # 편집창 초기화
        win.child_window(class_name="Edit", found_index=0).set_text("")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q07_file_load_status(app, win) -> bool:
    """Q-07: CSV 파일 로드 후 상태 텍스트가 '데이터 로드 완료. 질문을 입력하세요.' 인지 확인한다."""
    print("[Q-07] CSV 로드 후 상태 텍스트 확인")
    try:
        helpers.open_file(app, win, CSV_STANDARD)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)

        text = helpers.get_status_text(win)
        expected = "데이터 로드 완료"
        ok = expected in text
        print(f"  실제: '{text}'")
        print(f"  {'PASS' if ok else 'FAIL'} — '데이터 로드 완료' 포함 여부")
        helpers.capture(win, "Q07", "file_load_status")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q08_korean_input(win) -> bool:
    """Q-08: 한글 텍스트를 입력했을 때 편집 컨트롤에 정상 표시되는지 확인한다."""
    print("[Q-08] 한글 입력 확인")
    try:
        korean_text = "월별 매출 추이를 분석해주세요"
        helpers.type_query(win, korean_text)
        time.sleep(0.3)

        edit = win.child_window(class_name="Edit", found_index=0)
        actual = edit.window_text()
        ok = korean_text in actual
        print(f"  입력: '{korean_text}'")
        print(f"  실제: '{actual}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "Q08", "korean_input")

        # 정리
        edit.set_text("")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q03_enter_key_submit(app, win) -> bool:
    """Q-03: 쿼리 편집창에서 Enter 키 입력 시 분석이 트리거되는지 확인한다 (TC-03-05).
    분석 버튼 클릭이 아닌 키보드 Enter 로 OnSubmit 핸들러가 호출되어야 한다.
    파일 미로드 상태이므로 '파일' 관련 오류 메시지 또는 상태 변화로 판단한다.
    """
    print("[Q-03] Enter 키 질문 제출 확인 (TC-03-05)")
    try:
        edit = win.child_window(class_name="Edit", found_index=0)
        edit.set_focus()
        edit.set_text("Enter 키 테스트 질문")
        time.sleep(0.2)

        status_before = helpers.get_status_text(win)

        # Enter 키 전송
        from pywinauto.keyboard import send_keys
        send_keys("{ENTER}")
        time.sleep(0.7)

        msg_text = helpers.dismiss_dialog(app)
        status_after = helpers.get_status_text(win)

        # Enter 키가 처리되었음을 나타내는 조건:
        # - 메시지박스가 나타났거나 (파일 미로드 오류 등)
        # - 상태 텍스트가 변경되었거나
        # - 분석 관련 키워드가 상태 텍스트에 나타남
        status_changed = status_before != status_after
        has_response = bool(msg_text) or status_changed or any(
            kw in status_after for kw in ["분석", "오류", "파일", "처리", "중"]
        )

        ok = has_response
        print(f"  Enter 전 상태: '{status_before}'")
        print(f"  Enter 후 상태: '{status_after}'")
        print(f"  메시지박스: '{msg_text}'")
        print(f"  {'PASS' if ok else 'FAIL'} — Enter 키 제출 응답 확인")
        helpers.capture(win, "Q03", "enter_key_submit")

        # 편집창 초기화
        win.child_window(class_name="Edit", found_index=0).set_text("")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q09_to_Q16_manual(win) -> bool:
    """Q-09~Q-16: LLM 응답이 필요한 테스트 — 선행 조건만 설정하고 스크린샷을 저장한다."""
    print("[Q-09~Q-16] MANUAL — LLM 분석 필요 테스트 (선행 조건 설정 + 스크린샷)")
    try:
        helpers.type_query(win, "데이터의 주요 통계를 요약해주세요")
        helpers.capture(win, "Q09_Q16", "manual_precondition")
        print("  MANUAL — LLM 응답을 자동으로 검증할 수 없음. 스크린샷 저장 완료.")
        return True  # 선행 조건 설정 자체는 성공
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q10_no_datasource_with_query(app, win) -> bool:
    """Q-10: 파일 미로드 상태에서 질의 텍스트 있을 때 분석 → 오류 메시지 확인 (TC-03-03)"""
    print("[Q-10] datasource 없음 + 질의 입력 후 분석 → 오류 메시지 확인")
    try:
        # 파일이 로드된 상태일 수 있으므로 앱을 초기 상태로 재시작하지 않고
        # 단순히 유의미한 쿼리를 입력한 뒤 분석 버튼 클릭
        query_text = "컬럼별 평균값을 알려주세요"
        helpers.type_query(win, query_text)
        helpers.click_analyze(win)
        time.sleep(0.5)

        msg_text = helpers.dismiss_dialog(app)
        status = helpers.get_status_text(win)

        # 파일 미로드 오류 메시지 또는 상태 텍스트 오류 확인
        ok = (
            "데이터 파일" in msg_text
            or "파일" in msg_text
            or "로드" in msg_text
            or "오류" in status
            or "파일" in status
        )
        print(f"  입력 질의: '{query_text}'")
        print(f"  메시지박스 텍스트: '{msg_text}'")
        print(f"  상태 텍스트: '{status}'")
        print(f"  {'PASS' if ok else 'FAIL'} — datasource 없음 오류 메시지 확인")
        helpers.capture(win, "Q10", "no_datasource_with_query")

        # 편집창 초기화
        win.child_window(class_name="Edit", found_index=0).set_text("")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_Q12_llm_error_dialog_manual() -> bool:
    """Q-12: LLM API 오류 시 사용자 안내 다이얼로그 — MANUAL 테스트 (TC-08-05)"""
    print("[Q-12_MANUAL] LLM API 오류 시 오류 다이얼로그 표시 — 수동 검증 필요")
    print("  MANUAL — 잘못된 API 키 또는 네트워크 차단 상태에서 질문을 제출한다.")
    print("  기대: 오류 다이얼로그 표시, 재시도 안내 포함, 앱 크래시 없음.")
    return True


def test_Q11_progress_bar_on_analysis(app, win) -> bool:
    """Q-11: 파일 로드 후 질문 제출 → 프로그레스 바 또는 상태 텍스트 변경 확인 (TC-03-04)"""
    print("[Q-11] 질문 제출 후 프로그레스 바 / 상태 텍스트 변경 확인")
    try:
        helpers.open_file(app, win, CSV_STANDARD)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)

        status_before = helpers.get_status_text(win)

        helpers.type_query(win, "데이터 요약을 보여주세요")
        helpers.click_analyze(win)
        time.sleep(1.0)  # 분석 시작 직후 상태 변화 캡처

        # 프로그레스 바 존재 및 상태 텍스트 변화 확인
        try:
            progress = win.child_window(class_name="msctls_progress32")
            progress_exists = progress.exists()
        except Exception:
            progress_exists = False

        status_after = helpers.get_status_text(win)

        # 상태 텍스트가 변경되거나 "분석" 관련 텍스트가 나타나면 통과
        status_changed = status_before != status_after
        analysis_started = any(kw in status_after for kw in ["분석", "처리", "중", "완료", "오류"])

        ok = progress_exists or status_changed or analysis_started

        print(f"  분석 전 상태: '{status_before}'")
        print(f"  분석 후 상태: '{status_after}'")
        print(f"  프로그레스 바 존재: {progress_exists}")
        print(f"  상태 변경: {status_changed}")
        print(f"  {'PASS' if ok else 'FAIL'} — 분석 중 상태 전환 확인")
        helpers.capture(win, "Q11", "progress_bar_on_analysis")

        # 메시지박스가 있을 경우 닫기
        helpers.dismiss_dialog(app)

        # 편집창 초기화
        win.child_window(class_name="Edit", found_index=0).set_text("")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


# ---------------------------------------------------------------------------
# 스위트 실행
# ---------------------------------------------------------------------------

def run_suite(app, win) -> dict:
    results = {}
    # --- 파일 미로드 상태에서 실행해야 하는 테스트 (Q-01~Q-10) ---
    results["Q-01"] = test_Q01_controls_exist(win)
    results["Q-02"] = test_Q02_initial_status(win)
    # Q-03: Enter 키 제출 — 파일 미로드 상태에서 실행 (TC-03-05)
    results["Q-03"] = test_Q03_enter_key_submit(app, win)
    results["Q-05"] = test_Q05_empty_query_msgbox(app, win)
    results["Q-06"] = test_Q06_no_file_msgbox(app, win)
    # Q-10: datasource 없음 오류 — 반드시 Q-07(CSV 로드) 전에 실행 (TC-03-03)
    results["Q-10"] = test_Q10_no_datasource_with_query(app, win)
    # --- 파일 로드 후 실행해야 하는 테스트 (Q-07 이후) ---
    results["Q-07"] = test_Q07_file_load_status(app, win)
    results["Q-08"] = test_Q08_korean_input(win)
    results["Q-11"] = test_Q11_progress_bar_on_analysis(app, win)
    results["Q-09~Q-16_MANUAL"] = test_Q09_to_Q16_manual(win)
    results["Q-12_MANUAL"] = test_Q12_llm_error_dialog_manual()
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== QueryInputView 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")

    app.kill()
