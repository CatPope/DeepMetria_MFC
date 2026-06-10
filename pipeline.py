"""pipeline.py - DeepMetria 빌드 + UI 자동화 테스트 + 결과 분석

기본 동작: build → tests/test_runner.py → integration → 결과 분석 → pipeline_result.json 저장

옵션:
    --build-only          : 빌드만
    --test-only           : 테스트만 (기존 산출물 사용)
    --integration         : 통합 셀프테스트만 (tests/integration/run_selftest.py)
    --config Debug|Release: 빌드 설정 (기본 Debug)
    --suite NAME          : 특정 스위트만 실행 (test_runner.py 에 인자 전달)
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent
RESULT_JSON = ROOT / "pipeline_result.json"
TEST_RUNNER = ROOT / "tests" / "test_runner.py"
TEST_RESULTS = ROOT / "tests" / "test_results.txt"
INTEGRATION_RUNNER = ROOT / "tests" / "integration" / "run_selftest.py"
INTEGRATION_RESULTS = ROOT / "tests" / "integration_results.txt"


def run_build(config: str) -> dict[str, Any]:
    started = time.time()
    cmd = ["cmd", "/c", r".\build.bat", config]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return {
        "ok": proc.returncode == 0,
        "rc": proc.returncode,
        "config": config,
        "elapsed_sec": round(time.time() - started, 2),
        "stdout_tail": (proc.stdout or "")[-2000:],
        "stderr_tail": (proc.stderr or "")[-2000:],
    }


def run_tests(suite: str | None, config: str) -> dict[str, Any]:
    if not TEST_RUNNER.exists():
        return {"ok": False, "error": f"test runner 없음: {TEST_RUNNER}"}
    args = [sys.executable, str(TEST_RUNNER), "--config", config]
    if suite:
        args += ["--suite", suite]
    started = time.time()
    proc = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return {
        "ok": proc.returncode == 0,
        "rc": proc.returncode,
        "elapsed_sec": round(time.time() - started, 2),
        "stdout_tail": (proc.stdout or "")[-3000:],
        "results_text": TEST_RESULTS.read_text(encoding="utf-8", errors="replace") if TEST_RESULTS.exists() else "",
    }


def run_integration(config: str) -> dict[str, Any]:
    if not INTEGRATION_RUNNER.exists():
        return {"ok": False, "error": "runner missing"}
    started = time.time()
    proc = subprocess.run(
        [sys.executable, str(INTEGRATION_RUNNER), "--config", config],
        cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    elapsed = round(time.time() - started, 2)
    text = INTEGRATION_RESULTS.read_text(encoding="utf-8", errors="replace") if INTEGRATION_RESULTS.exists() else ""
    passed = sum(1 for L in text.splitlines() if L.startswith("[PASS]"))
    failed = sum(1 for L in text.splitlines() if L.startswith("[FAIL]"))
    return {
        "ok": proc.returncode == 0,
        "rc": proc.returncode,
        "elapsed_sec": elapsed,
        "passed": passed,
        "failed": failed,
        "results_text": text,
    }


def parse_suites(text: str) -> list[dict[str, str]]:
    out: list[dict[str, str]] = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line or not (line.startswith("[PASS]") or line.startswith("[FAIL]") or line.startswith("[SKIP]")):
            continue
        status, rest = line.split(" ", 1)
        out.append({"status": status.strip("[]"), "name": rest.strip()})
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-only",   action="store_true")
    ap.add_argument("--test-only",    action="store_true")
    ap.add_argument("--integration",  action="store_true")
    ap.add_argument("--config", default="Debug", choices=["Debug", "Release"])
    ap.add_argument("--suite",  default=None)
    args = ap.parse_args()

    result: dict[str, Any] = {
        "schema": 1,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "config": args.config,
    }

    # --integration 단독: 통합 셀프테스트만 실행
    if args.integration and not args.build_only and not args.test_only:
        result["integration"] = run_integration(args.config)
        RESULT_JSON.write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
        ig = result["integration"]
        print(f"[pipeline] build=skipped  suites: pass=0 fail=0  "
              f"integration: pass={ig['passed']} fail={ig['failed']}")
        return 0 if ig["ok"] else 1

    # 빌드 단계
    if not args.test_only and not args.integration:
        result["build"] = run_build(args.config)
        if not result["build"]["ok"]:
            RESULT_JSON.write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
            print("[pipeline] BUILD FAILED - 자세한 로그는 build_rebuild.log 또는 stdout_tail 확인")
            return 1

    # 테스트 단계
    if not args.build_only and not args.integration:
        result["test"] = run_tests(args.suite, args.config)
        result["suites"] = parse_suites(result["test"].get("results_text", ""))

    # 전체 모드(옵션 없음): integration 도 포함
    if not args.test_only and not args.build_only and not args.integration:
        result["integration"] = run_integration(args.config)

    RESULT_JSON.write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")

    passed = sum(1 for s in result.get("suites", []) if s["status"] == "PASS")
    failed = sum(1 for s in result.get("suites", []) if s["status"] == "FAIL")
    ig = result.get("integration", {})
    ig_pass = ig.get("passed", 0)
    ig_fail = ig.get("failed", 0)
    build_status = result.get("build", {}).get("ok", "skipped")
    print(f"[pipeline] build={build_status}  suites: pass={passed} fail={failed}  "
          f"integration: pass={ig_pass} fail={ig_fail}")
    return 0 if (result.get("build", {}).get("ok", True) and failed == 0 and ig.get("ok", True)) else 1


if __name__ == "__main__":
    sys.exit(main())
