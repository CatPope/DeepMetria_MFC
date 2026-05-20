"""
DataSummaryView 테스트 (D-01 ~ D-14)
우상단 CListView(SysListView32)의 컬럼 구성 및 데이터 표시를 검증한다.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import csv
import time
import helpers
from config import CSV_STANDARD, CSV_NARROW, FILE_LOAD_TIMEOUT

# 기대 컬럼 헤더 (순서 포함)
EXPECTED_COLUMNS = ["컬럼명", "타입", "고유값 수", "결측치", "최솟값", "최댓값"]


def _count_csv_columns(csv_path: str) -> int:
    """CSV 파일의 헤더 컬럼 수를 반환한다."""
    try:
        with open(csv_path, encoding="utf-8-sig", newline="") as f:
            reader = csv.reader(f)
            headers = next(reader)
            return len(headers)
    except Exception:
        return -1


# ---------------------------------------------------------------------------
# 테스트 케이스
# ---------------------------------------------------------------------------

def test_D01_no_items_before_load(win) -> bool:
    """D-01: 파일 로드 전 CListView 에 항목이 0개인지 확인한다."""
    print("[D-01] 파일 로드 전 ListVew 항목 수 확인")
    try:
        lv = win.child_window(class_name="SysListView32")
        count = lv.item_count()
        ok = count == 0
        print(f"  항목 수: {count}")
        print(f"  {'PASS' if ok else 'FAIL'} — 기대: 0")
        helpers.capture(win, "D01", "no_items_before_load")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_D02_column_headers(win) -> bool:
    """D-02: CListView 의 컬럼 헤더가 6개이고 기대값과 일치하는지 확인한다."""
    print("[D-02] 컬럼 헤더 확인")
    try:
        cols = helpers.get_listview_columns(win)
        print(f"  실제 컬럼: {cols}")
        ok = len(cols) == len(EXPECTED_COLUMNS) and cols == EXPECTED_COLUMNS
        print(f"  {'PASS' if ok else 'FAIL'} — 기대: {EXPECTED_COLUMNS}")
        helpers.capture(win, "D02", "column_headers")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_D04_item_count_matches_csv(app, win) -> bool:
    """D-04: CSV 로드 후 ListVew 항목 수가 CSV 헤더 컬럼 수와 일치하는지 확인한다."""
    print("[D-04] CSV 로드 후 항목 수 = CSV 컬럼 수 확인")
    try:
        helpers.open_file(app, win, CSV_STANDARD)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)

        lv = win.child_window(class_name="SysListView32")
        actual_count = lv.item_count()
        expected_count = _count_csv_columns(CSV_STANDARD)

        ok = actual_count == expected_count
        print(f"  ListView 항목: {actual_count}, CSV 컬럼 수: {expected_count}")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "D04", "item_count_matches_csv")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_D05_first_column_name(win) -> bool:
    """D-05: 첫 번째 항목의 '컬럼명' 셀이 CSV 첫 번째 헤더와 일치하는지 확인한다."""
    print("[D-05] 첫 번째 컬럼명 확인")
    try:
        # CSV 첫 번째 헤더 읽기
        with open(CSV_STANDARD, encoding="utf-8-sig", newline="") as f:
            reader = csv.reader(f)
            headers = next(reader)
        expected_first = headers[0].strip()

        items = helpers.get_listview_items(win)
        if not items:
            print("  FAIL — ListView 에 항목 없음")
            return False

        actual_first = items[0][0] if items[0] else ""
        ok = actual_first == expected_first
        print(f"  실제: '{actual_first}', 기대: '{expected_first}'")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "D05", "first_column_name")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_D06_type_column_values(win) -> bool:
    """D-06: '타입' 컬럼 값이 'numeric' 또는 'text' 중 하나인지 확인한다."""
    print("[D-06] 타입 컬럼 값 확인")
    try:
        items = helpers.get_listview_items(win)
        if not items:
            print("  FAIL — ListView 에 항목 없음")
            return False

        valid_types = {"numeric", "text", "숫자", "텍스트", "integer", "float", "string"}
        type_col_index = 1  # '타입' 컬럼은 두 번째(인덱스 1)

        invalid = []
        for row in items:
            if len(row) > type_col_index:
                val = row[type_col_index].strip().lower()
                if val and val not in valid_types:
                    invalid.append(val)

        ok = len(invalid) == 0
        if invalid:
            print(f"  예상치 못한 타입 값: {invalid}")
        else:
            types_found = [row[type_col_index] for row in items if len(row) > type_col_index]
            print(f"  발견된 타입: {set(types_found)}")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "D06", "type_column_values")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


def test_D13_reload_different_csv(app, win) -> bool:
    """D-13: 다른 CSV 로드 시 ListView 항목이 변경되는지 확인한다."""
    print("[D-13] 다른 CSV 로드 후 항목 변경 확인")
    try:
        lv = win.child_window(class_name="SysListView32")
        count_before = lv.item_count()

        helpers.open_file(app, win, CSV_NARROW)
        time.sleep(FILE_LOAD_TIMEOUT * 0.5)

        count_after = lv.item_count()
        expected_after = _count_csv_columns(CSV_NARROW)

        ok = count_after == expected_after
        print(f"  이전 항목 수: {count_before}, 이후 항목 수: {count_after}, CSV 컬럼 수: {expected_after}")
        print(f"  {'PASS' if ok else 'FAIL'}")
        helpers.capture(win, "D13", "reload_different_csv")
        return ok
    except Exception as exc:
        print(f"  FAIL — 예외: {exc}")
        return False


# ---------------------------------------------------------------------------
# 스위트 실행
# ---------------------------------------------------------------------------

def run_suite(app, win) -> dict:
    results = {}
    results["D-01"] = test_D01_no_items_before_load(win)
    results["D-02"] = test_D02_column_headers(win)
    results["D-04"] = test_D04_item_count_matches_csv(app, win)
    results["D-05"] = test_D05_first_column_name(win)
    results["D-06"] = test_D06_type_column_values(win)
    results["D-13"] = test_D13_reload_different_csv(app, win)
    return results


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== DataSummaryView 테스트 결과 ===")
    for tid, ok in results.items():
        print(f"  {tid}: {'PASS' if ok else 'FAIL'}")

    app.kill()
