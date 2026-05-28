"""
DeepMetria 개발 파이프라인
빌드 → UI 테스트 → 결과 분석을 자동화한다.

사용법:
  python pipeline.py                    # 전체 파이프라인 (빌드 + 테스트 + 분석)
  python pipeline.py --build-only       # 빌드만
  python pipeline.py --test-only        # 테스트만 (이미 빌드된 상태)
  python pipeline.py --config Release   # Release 빌드로 파이프라인 실행
  python pipeline.py --max-loops 3      # 최대 루프 횟수 (기본: 1, 자동 반복 없음)
"""
import subprocess
import sys
import os
import json
import datetime
import argparse
import re
import ctypes

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.stderr.reconfigure(encoding="utf-8", errors="replace")

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
RESULTS_FILE = os.path.join(PROJECT_ROOT, "tests", "test_results.txt")
PIPELINE_LOG = os.path.join(PROJECT_ROOT, "pipeline_result.json")


def run_build(config: str = "Debug") -> dict:
    """build.bat을 실행하고 결과를 반환한다."""
    print(f"\n{'='*52}")
    print(f"  [1/3] 빌드 ({config} x64)")
    print(f"{'='*52}")

    bat = os.path.join(PROJECT_ROOT, "build.bat")
    result = subprocess.run(
        ["cmd.exe", "/c", bat, config],
        capture_output=True, text=True, encoding="utf-8", errors="replace",
        cwd=PROJECT_ROOT, timeout=300
    )

    success = result.returncode == 0
    exe_path = os.path.join(PROJECT_ROOT, "x64", config, "DeepMetria.exe")
    exe_exists = os.path.isfile(exe_path)

    if success and exe_exists:
        print(f"  [OK] 빌드 성공: {exe_path}")
    else:
        print(f"  [FAILED] 빌드 실패 (exit code: {result.returncode})")
        if result.stdout:
            for line in result.stdout.strip().splitlines()[-5:]:
                print(f"    {line}")
        if result.stderr:
            for line in result.stderr.strip().splitlines()[-5:]:
                print(f"    {line}")

    return {
        "step": "build",
        "success": success and exe_exists,
        "config": config,
        "exe_path": exe_path if exe_exists else None,
        "exit_code": result.returncode,
    }


def run_tests(config: str = "Debug", suite: str = None) -> dict:
    """test_runner.py를 실행하고 결과를 반환한다."""
    print(f"\n{'='*52}")
    print(f"  [2/3] UI 테스트 실행")
    print(f"{'='*52}")

    exe_path = os.path.join("x64", config, "DeepMetria.exe")
    full_exe = os.path.join(PROJECT_ROOT, exe_path)
    if not os.path.isfile(full_exe):
        print(f"  [SKIP] 실행 파일 없음: {full_exe}")
        return {"step": "test", "success": False, "reason": "exe_not_found"}

    env = os.environ.copy()
    env["DEEPMETRIA_EXE"] = exe_path
    env["PYTHONIOENCODING"] = "utf-8"

    cmd = [sys.executable, os.path.join(PROJECT_ROOT, "tests", "test_runner.py")]
    if suite:
        cmd += ["--suite", suite]

    print(f"  EXE: {exe_path}")
    print(f"  스위트: {suite or '전체'}")
    print()

    # SetForegroundWindow 제한 해제 (UI 자동화에 필수)
    focus_fix_dir = os.path.join(PROJECT_ROOT, ".claude", "temp")
    site_customize = os.path.join(focus_fix_dir, "sitecustomize.py")
    if os.path.isfile(site_customize):
        existing = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = focus_fix_dir + (os.pathsep + existing if existing else "")

    result = subprocess.run(
        cmd, capture_output=True, text=True, encoding="utf-8", errors="replace",
        cwd=PROJECT_ROOT, env=env, timeout=300
    )

    if result.stdout:
        print(result.stdout)

    return {
        "step": "test",
        "success": result.returncode == 0,
        "exit_code": result.returncode,
    }


def analyze_results() -> dict:
    """test_results.txt를 파싱하여 구조화된 결과를 반환한다."""
    print(f"\n{'='*52}")
    print(f"  [3/3] 결과 분석")
    print(f"{'='*52}")

    if not os.path.isfile(RESULTS_FILE):
        print("  [SKIP] 결과 파일 없음")
        return {
            "step": "analyze",
            "success": False,
            "reason": "no_results_file",
            "tests": {},
            "summary": {"total": 0, "passed": 0, "failed": 0, "manual": 0},
        }

    tests = {}
    summary = {"total": 0, "passed": 0, "failed": 0, "manual": 0, "skipped": 0}
    failed_ids = []

    with open(RESULTS_FILE, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            match = re.match(r"^(\S+)\s+(PASS|FAIL|MANUAL|SKIP)$", line)
            if match:
                tid, result = match.groups()
                tests[tid] = result
                summary["total"] += 1
                if result == "PASS":
                    summary["passed"] += 1
                elif result == "FAIL":
                    summary["failed"] += 1
                    failed_ids.append(tid)
                elif result == "MANUAL":
                    summary["manual"] += 1
                elif result == "SKIP":
                    summary["skipped"] += 1

    auto_total = summary["passed"] + summary["failed"]
    pass_rate = (summary["passed"] / auto_total * 100) if auto_total > 0 else 0
    complete = summary["failed"] == 0 and auto_total > 0

    print(f"  전체: {summary['total']}  PASS: {summary['passed']}  "
          f"FAIL: {summary['failed']}  SKIP: {summary['skipped']}  "
          f"MANUAL: {summary['manual']}")
    print(f"  자동 테스트 통과율: {pass_rate:.0f}%")
    if failed_ids:
        print(f"  실패 항목: {', '.join(failed_ids)}")
    print(f"\n  판정: {'완성' if complete else '미완성'}")

    return {
        "step": "analyze",
        "success": True,
        "complete": complete,
        "tests": tests,
        "summary": summary,
        "failed_ids": failed_ids,
        "pass_rate": pass_rate,
    }


def save_pipeline_result(build_result: dict, test_result: dict, analysis: dict,
                          loop_count: int):
    """파이프라인 결과를 JSON으로 저장한다."""
    result = {
        "timestamp": datetime.datetime.now().isoformat(),
        "loop": loop_count,
        "build": build_result,
        "test": test_result,
        "analysis": analysis,
        "verdict": "PASS" if analysis.get("complete") else "FAIL",
    }
    with open(PIPELINE_LOG, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)
    print(f"\n  파이프라인 결과 저장: {PIPELINE_LOG}")
    return result


def run_pipeline(config: str = "Debug", suite: str = None, max_loops: int = 1,
                  build_only: bool = False, test_only: bool = False) -> dict:
    """전체 파이프라인을 실행한다."""
    print(f"\n{'#'*52}")
    print(f"  DeepMetria 개발 파이프라인")
    print(f"  {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"  구성: {config} | 최대 루프: {max_loops}")
    print(f"{'#'*52}")

    for loop in range(1, max_loops + 1):
        if max_loops > 1:
            print(f"\n{'*'*52}")
            print(f"  루프 {loop}/{max_loops}")
            print(f"{'*'*52}")

        build_result = {"step": "build", "success": True, "skipped": True}
        test_result = {"step": "test", "success": True, "skipped": True}
        analysis = {"step": "analyze", "complete": False, "skipped": True}

        if not test_only:
            build_result = run_build(config)
            if not build_result["success"]:
                analysis = {"step": "analyze", "complete": False,
                            "reason": "build_failed"}
                save_pipeline_result(build_result, test_result, analysis, loop)
                print("\n  [STOP] 빌드 실패 -코드 수정 필요")
                return {"verdict": "BUILD_FAILED", "loop": loop}

        if build_only:
            print("\n  [DONE] 빌드 전용 모드 -테스트 건너뜀")
            return {"verdict": "BUILD_OK", "loop": loop}

        test_result = run_tests(config, suite)
        analysis = analyze_results()
        pipeline_result = save_pipeline_result(
            build_result, test_result, analysis, loop)

        if analysis.get("complete"):
            print(f"\n{'#'*52}")
            print(f"  [COMPLETE] 파이프라인 완료 -모든 자동 테스트 통과")
            print(f"{'#'*52}")
            return {"verdict": "PASS", "loop": loop, "result": pipeline_result}

        if loop < max_loops:
            print(f"\n  [RETRY] 미완성 -다음 루프에서 재시도")
        else:
            print(f"\n  [INCOMPLETE] {max_loops}회 루프 완료 -미완성 항목 존재")

    return {"verdict": "INCOMPLETE", "loop": max_loops,
            "failed_ids": analysis.get("failed_ids", [])}


def main():
    parser = argparse.ArgumentParser(description="DeepMetria 개발 파이프라인")
    parser.add_argument("--config", default="Debug", choices=["Debug", "Release"],
                        help="빌드 구성 (기본: Debug)")
    parser.add_argument("--suite", default=None,
                        choices=["layout", "file_load", "query_input", "data_summary",
                                 "json_load", "export", "dashboard", "format_editor"],
                        help="특정 테스트 스위트만 실행")
    parser.add_argument("--max-loops", type=int, default=1,
                        help="최대 반복 횟수 (기본: 1)")
    parser.add_argument("--build-only", action="store_true",
                        help="빌드만 실행")
    parser.add_argument("--test-only", action="store_true",
                        help="테스트만 실행 (빌드 건너뜀)")
    args = parser.parse_args()

    result = run_pipeline(
        config=args.config,
        suite=args.suite,
        max_loops=args.max_loops,
        build_only=args.build_only,
        test_only=args.test_only,
    )

    exit_code = 0 if result["verdict"] in ("PASS", "BUILD_OK") else 1
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
