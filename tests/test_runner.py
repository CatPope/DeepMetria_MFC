"""
DeepMetria MFC 자동화 테스트 러너

사용법:
  python tests/test_runner.py                    # 전체 테스트 실행
  python tests/test_runner.py --connect          # 이미 실행 중인 앱에 연결
  python tests/test_runner.py --suite layout     # 특정 스위트만 실행
                                                 # 스위트: layout, file_load, query_input, data_summary,
                                                 #         json_load, export, dashboard

환경변수:
  DEEPMETRIA_EXE  — 앱 실행 파일 경로 (기본값: x64\\Release\\DeepMetria.exe)
"""
import sys
import os
import argparse
import datetime
import time

# tests/ 디렉토리를 경로에 추가
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import helpers
from config import SCREENSHOT_DIR, CLAUDE_TEMP_DIR, PROJECT_ROOT

import test_layout
import test_file_load
import test_query_input
import test_data_summary
import test_json_load
import test_export
import test_dashboard


# ---------------------------------------------------------------------------
# 디렉토리 초기화
# ---------------------------------------------------------------------------

def ensure_directories():
    os.makedirs(SCREENSHOT_DIR, exist_ok=True)
    os.makedirs(CLAUDE_TEMP_DIR, exist_ok=True)
    os.makedirs(os.path.join(PROJECT_ROOT, "tests"), exist_ok=True)


# ---------------------------------------------------------------------------
# 결과 집계 및 출력
# ---------------------------------------------------------------------------

MANUAL_MARKER = "MANUAL"

def print_summary(all_results: dict, elapsed: float):
    total = len(all_results)
    manual = sum(1 for tid in all_results if MANUAL_MARKER in tid)
    passed = sum(1 for tid, ok in all_results.items() if ok and MANUAL_MARKER not in tid)
    failed = sum(1 for tid, ok in all_results.items() if not ok and MANUAL_MARKER not in tid)

    sep = "=" * 52
    print(f"\n{sep}")
    print("  DeepMetria MFC 테스트 결과 요약")
    print(sep)
    print(f"  {'테스트 ID':<20} {'결과'}")
    print(f"  {'-'*20} {'-'*10}")
    for tid, ok in all_results.items():
        if MANUAL_MARKER in tid:
            label = "MANUAL"
        else:
            label = "PASS" if ok else "FAIL"
        print(f"  {tid:<20} {label}")
    print(f"  {'-'*20} {'-'*10}")
    print(f"  전체: {total}  PASS: {passed}  FAIL: {failed}  MANUAL: {manual}")
    print(f"  소요 시간: {elapsed:.1f}초")
    print(sep)


def save_results(all_results: dict, elapsed: float):
    result_path = os.path.join(PROJECT_ROOT, "tests", "test_results.txt")
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines = [
        f"DeepMetria MFC 자동화 테스트 결과",
        f"실행 시각: {ts}",
        f"소요 시간: {elapsed:.1f}초",
        "",
        f"{'테스트 ID':<20} {'결과'}",
        "-" * 32,
    ]
    total = len(all_results)
    manual = sum(1 for tid in all_results if MANUAL_MARKER in tid)
    passed = sum(1 for tid, ok in all_results.items() if ok and MANUAL_MARKER not in tid)
    failed = sum(1 for tid, ok in all_results.items() if not ok and MANUAL_MARKER not in tid)

    for tid, ok in all_results.items():
        if MANUAL_MARKER in tid:
            label = "MANUAL"
        else:
            label = "PASS" if ok else "FAIL"
        lines.append(f"{tid:<20} {label}")

    lines += [
        "-" * 32,
        f"전체: {total}  PASS: {passed}  FAIL: {failed}  MANUAL: {manual}",
    ]

    with open(result_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"\n  결과 저장: {result_path}")


# ---------------------------------------------------------------------------
# 스위트 실행
# ---------------------------------------------------------------------------

SUITE_ORDER = ["layout", "file_load", "query_input", "data_summary", "json_load", "export", "dashboard"]

SUITE_DESCRIPTIONS = {
    "layout":       "레이아웃 테스트 (L-01~L-09)",
    "file_load":    "파일 로드 테스트 (F-01~F-09)",
    "query_input":  "QueryInputView 테스트 (Q-01~Q-16)",
    "data_summary": "DataSummaryView 테스트 (D-01~D-14)",
    "json_load":    "JSON 파일 로드 테스트 (J-01~J-04)",
    "export":       "내보내기 테스트 (E-01~E-04)",
    "dashboard":    "대시보드 인터랙션 테스트 (DI-01~DI-06)",
}


def run_layout(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['layout']}")
    print(f"{'─'*48}")
    return test_layout.run_suite(win)


def run_file_load(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['file_load']}")
    print(f"{'─'*48}")
    return test_file_load.run_suite(app, win)


def run_query_input(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['query_input']}")
    print(f"{'─'*48}")
    return test_query_input.run_suite(app, win)


def run_data_summary(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['data_summary']}")
    print(f"{'─'*48}")
    return test_data_summary.run_suite(app, win)


def run_json_load(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['json_load']}")
    print(f"{'─'*48}")
    return test_json_load.run_suite(app, win)


def run_export(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['export']}")
    print(f"{'─'*48}")
    return test_export.run_suite(app, win)


def run_dashboard(app, win) -> dict:
    print(f"\n{'─'*48}")
    print(f"  {SUITE_DESCRIPTIONS['dashboard']}")
    print(f"{'─'*48}")
    return test_dashboard.run_suite(app, win)


SUITE_RUNNERS = {
    "layout":       run_layout,
    "file_load":    run_file_load,
    "query_input":  run_query_input,
    "data_summary": run_data_summary,
    "json_load":    run_json_load,
    "export":       run_export,
    "dashboard":    run_dashboard,
}


# ---------------------------------------------------------------------------
# 진입점
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="DeepMetria MFC 자동화 테스트 러너"
    )
    parser.add_argument(
        "--connect",
        action="store_true",
        help="이미 실행 중인 DeepMetria 앱에 연결한다.",
    )
    parser.add_argument(
        "--suite",
        choices=SUITE_ORDER,
        default=None,
        help="실행할 테스트 스위트를 지정한다. 생략 시 전체 실행.",
    )
    args = parser.parse_args()

    ensure_directories()

    # 앱 시작 또는 연결
    print("\nDeepMetria MFC 자동화 테스트 시작")
    print(f"  앱 연결 방식: {'connect (기존 앱)' if args.connect else 'start (새 실행)'}")

    try:
        if args.connect:
            app = helpers.connect_app()
        else:
            app = helpers.start_app()
    except Exception as exc:
        print(f"\n[오류] 앱 시작/연결 실패: {exc}")
        print("  DEEPMETRIA_EXE 환경변수 또는 config.py 의 APP_EXE 를 확인하세요.")
        sys.exit(1)

    win = helpers.get_main_window(app)
    try:
        win.wait("ready", timeout=10)
    except Exception:
        pass  # 이미 준비된 경우 타임아웃 무시

    # 실행할 스위트 목록 결정
    suites_to_run = [args.suite] if args.suite else SUITE_ORDER

    all_results: dict = {}
    start_time = time.time()

    for suite_name in suites_to_run:
        # query_input은 초기 상태가 필요하므로 앱 재시작
        if suite_name == "query_input" and not args.connect and not args.suite:
            try:
                app.kill()
                time.sleep(1)
                app = helpers.start_app()
                win = helpers.get_main_window(app)
                win.wait("ready", timeout=10)
            except Exception as exc:
                print(f"\n  [앱 재시작 오류] {exc}")

        runner = SUITE_RUNNERS[suite_name]
        try:
            suite_results = runner(app, win)
            all_results.update(suite_results)
        except Exception as exc:
            print(f"\n  [스위트 오류] {suite_name}: {exc}")

    elapsed = time.time() - start_time

    print_summary(all_results, elapsed)
    save_results(all_results, elapsed)

    # 앱 종료 (start 모드에서만)
    if not args.connect:
        try:
            app.kill()
        except Exception:
            pass

    # 실패한 테스트가 있으면 비정상 종료 코드 반환
    failed = sum(
        1 for tid, ok in all_results.items()
        if not ok and MANUAL_MARKER not in tid
    )
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
