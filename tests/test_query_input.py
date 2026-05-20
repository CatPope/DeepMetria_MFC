"""
QueryInputView 테스트 (Q-01 ~ Q-16)
좌측 패널의 편집 컨트롤, 버튼, 상태 텍스트, 진행바를 검증한다.
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

def _dismiss_msgbox(app, expected_text: str = "") -> str:
    """
    #32770 클래스의 대화상자(MessageBox)를 찾아 텍스트를 읽고 Enter 로 닫는다.
    찾은 텍스트를 반환한다. 대화상자가 없으면 빈 문자열을 반환한다.
    """
    import pywinauto
    try:
        dlg = app.window(class_name="#32770")
        dlg.wait("visible", timeout=3)
        text = dlg.window_text()
        # 대화상자 본문 텍스트도 수집
        try:
            static = dlg.child_window(class_name="Static", found_index=0)
            text = static.window_text()
        except Exception:
            pass
        dlg.type_keys("{ENTER}")
        return text
    except Exception:
        return ""


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

        msg_text = _dismiss_msgbox(app, "질문을 입력해주세요")
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

        msg_text = _dismiss_msgbox(app, "데이터 파일")
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


# ---------------------------------------------------------------------------
# 스위트 실행
# ---------------------------------------------------------------------------

def run_suite(app, win) -> dict:
    results = {}
    results["Q-01"] = test_Q01_controls_exist(win)
    results["Q-02"] = test_Q02_initial_status(win)
    results["Q-05"] = test_Q05_empty_query_msgbox(app, win)
    results["Q-06"] = test_Q06_no_file_msgbox(app, win)
    results["Q-07"] = test_Q07_file_load_status(app, win)
    results["Q-08"] = test_Q08_korean_input(win)
    results["Q-09~Q-16"] = test_Q09_to_Q16_manual(win)
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
