"""tests/integration/run_selftest.py
DeepMetria.exe --self-test 를 실행하고 종료코드 + 로그 파일을 종합 판정.

산출:
- tests/integration_results.txt  — 라인별 [PASS]/[FAIL] <command-name>
- 종료코드 0 (모두 PASS) 또는 1 (한 개라도 FAIL)
"""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / "tests" / "integration_results.txt"
LOG     = ROOT / "tests" / "selftest_log.txt"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="Debug", choices=["Debug", "Release"])
    args = ap.parse_args()

    exe = ROOT / "x64" / args.config / "DeepMetria.exe"
    if not exe.exists():
        RESULTS.write_text(f"[FAIL] integration  EXE missing: {exe}\n", encoding="utf-8")
        return 1

    # 기존 로그/PNG 정리
    if LOG.exists():     LOG.unlink()
    out_png = ROOT / "tests" / "out" / "selftest.png"
    if out_png.exists(): out_png.unlink()

    started = time.time()
    proc = subprocess.run([str(exe), "--self-test"], cwd=str(ROOT),
                          capture_output=True, timeout=120)
    elapsed = round(time.time() - started, 2)

    lines: list[str] = []
    # 로그 파일 우선, 없으면 stdout 시도
    text = ""
    if LOG.exists():
        text = LOG.read_text(encoding="utf-8", errors="replace")
    elif proc.stdout:
        text = proc.stdout.decode("utf-8", errors="replace")

    for raw in text.splitlines():
        s = raw.strip()
        if s.startswith("[PASS]") or s.startswith("[FAIL]") or s.startswith("SELFTEST"):
            lines.append(s)

    # 종료코드 검사
    overall = "[PASS] integration" if proc.returncode == 0 else f"[FAIL] integration (rc={proc.returncode})"
    lines.append(overall)
    lines.append(f"# elapsed: {elapsed}s")

    RESULTS.parent.mkdir(parents=True, exist_ok=True)
    RESULTS.write_text("\n".join(lines) + "\n", encoding="utf-8")

    for L in lines: print(L)
    return 0 if proc.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
