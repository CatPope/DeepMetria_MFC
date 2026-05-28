"""
DeepMetria DnD 파일 로딩 테스트 (TC-01-08)
Drag & Drop은 pywinauto win32 백엔드에서 자동화 불가 -> MANUAL
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import helpers


def test_DnD_file_load_manual(app, win) -> bool:
    """TC-01-08: DnD 파일 로딩 — MANUAL 테스트"""
    print("[DnD-01_MANUAL] DnD 파일 로딩 테스트")
    print("  MANUAL -- pywinauto win32 백엔드에서 Drag & Drop 자동화 불가")
    print("  검증 항목: 탐색기에서 CSV/xlsx 파일을 메인 윈도우로 끌어다 놓기")
    print("  기대 결과: 파일이 정상 로드되어 DataSummaryView에 데이터가 표시됨")
    return None  # MANUAL


def run_suite(app, win) -> dict:
    return {
        "DnD-01_MANUAL": test_DnD_file_load_manual(app, win),
    }


if __name__ == "__main__":
    app = helpers.start_app()
    win = helpers.get_main_window(app)
    win.wait("ready", timeout=10)

    results = run_suite(app, win)

    print("\n=== DnD 파일 로딩 테스트 결과 ===")
    for tid, ok in results.items():
        label = "MANUAL" if ok is None else ("PASS" if ok else "FAIL")
        print(f"  {tid}: {label}")

    app.kill()
