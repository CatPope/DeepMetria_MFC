# DeepMetria MFC — 빌드·실행 매뉴얼

> 작성일: 2026-06-09
> 대상: 개발자 / 후임 에이전트
> 위치: `<프로젝트루트>/docs/build manual/`

이 폴더에는 DeepMetria MFC 프로젝트를 **빌드·실행·테스트**하는 데 필요한 스크립트와 사용 설명이 모여 있습니다. 모든 래퍼 스크립트는 프로젝트 루트의 원본 스크립트를 호출하므로, 이 폴더만 따로 들고 다녀도(같은 저장소 안이라면) 동일하게 동작합니다.

---

## 목차

1. [사전 요구사항](#1-사전-요구사항)
2. [빠른 시작](#2-빠른-시작)
3. [스크립트 일람](#3-스크립트-일람)
4. [빌드](#4-빌드)
5. [실행](#5-실행)
6. [테스트 (UI 파이프라인)](#6-테스트-ui-파이프라인)
7. [API 키 설정](#7-api-키-설정)
8. [자주 보는 오류와 대처](#8-자주-보는-오류와-대처)
9. [참고 경로](#9-참고-경로)

---

## 1. 사전 요구사항

| 항목 | 요구 버전 | 비고 |
|------|-----------|------|
| OS | Windows 10/11 x64 | MFC + GDI+ 필수 |
| Visual Studio | 2022 Community | `MSBuild.exe` 사용 |
| MSVC 툴셋 | v14.44.35207 | `build.bat`에서 명시적으로 고정 |
| Windows SDK | 10.0 이상 | MFC 헤더·런타임 |
| Python | 3.13+ | UI 자동화 테스트용 |
| PowerShell | 5.1+ 또는 7.x | DPAPI 키 저장 |

설치 경로 가정:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\
```
경로가 다르면 `build.bat` / `rebuild.bat`의 `call ... vcvars64.bat` 줄을 수정해야 합니다.

---

## 2. 빠른 시작

처음 받은 저장소를 동작 상태까지 끌어올리는 최소 순서:

```bat
REM 1) (선택) API 키 등록 — LLM 기능 사용 시
"docs\build manual\setup-secrets.bat"

REM 2) 빌드
"docs\build manual\build.bat"

REM 3) 실행
"docs\build manual\run.bat"

REM 4) (선택) 자동 UI 테스트
"docs\build manual\pipeline.bat"
```

> 모든 명령은 **프로젝트 루트**에서 실행합니다. 경로에 공백이 있으므로 cmd/PowerShell 양쪽 모두 따옴표로 감쌉니다. PowerShell에서는 `\` 대신 `/`도 동작합니다.

---

## 3. 스크립트 일람

| 파일 (경로: `docs/build manual/`) | 역할 | 원본 |
|------|------|------|
| `build.bat` | Debug/Release 빌드 (증분) | `<root>/build.bat` |
| `rebuild.bat` | Clean + 풀빌드 (로그 저장) | `<root>/rebuild.bat` |
| `run.bat` | 빌드 산출물 실행 | (래퍼만 존재) |
| `pipeline.bat` | 빌드 → UI 테스트 → 분석 | `<root>/pipeline.py` |
| `setup-secrets.bat` | DPAPI 암호화로 API 키 레지스트리 저장 | `<root>/scripts/setup-secrets.ps1` |

---

## 4. 빌드

### 4.1 Debug 빌드 (기본)

```bat
"docs\build manual\build.bat"
```

산출물: `x64\Debug\DeepMetria.exe`

### 4.2 Release 빌드

```bat
"docs\build manual\build.bat" Release
```

산출물: `x64\Release\DeepMetria.exe`

### 4.3 클린 재빌드 (문제 진단 시)

```bat
"docs\build manual\rebuild.bat"
```

- 항상 **Debug x64**로 풀 클린 후 재빌드
- 빌드 로그는 `build_rebuild.log`에 전체 기록 (콘솔에는 요약만)
- "왜 안 되는지 모르겠다" 싶을 때 1차로 돌릴 것

### 4.4 Python 파이프라인으로 빌드만

```bat
python pipeline.py --build-only
python pipeline.py --build-only --config Release
```

`pipeline_result.json`에 결과가 JSON으로 남으므로 CI/스크립트화에 유리.

---

## 5. 실행

### 5.1 매뉴얼 래퍼

```bat
"docs\build manual\run.bat"
"docs\build manual\run.bat" Release
```

- 인자 없으면 Debug, `Release` 주면 Release 실행
- 산출물이 없으면 안내 메시지 출력 후 종료

### 5.2 직접 실행

```bat
x64\Debug\DeepMetria.exe
x64\Release\DeepMetria.exe
```

> 처음 실행 시 LLM API 키가 등록돼 있지 않으면 분석 기능에서 "API 인증에 실패했습니다"가 표시됩니다. §7 참고.

---

## 6. 테스트 (UI 파이프라인)

### 6.1 전체 사이클

```bat
"docs\build manual\pipeline.bat"
```

흐름:
```
build.bat  →  tests/test_runner.py  →  test_results.txt 분석
                                              ↓
                                     pipeline_result.json
                                     tests/screenshots/run_<TIMESTAMP>/
```

### 6.2 빌드/테스트만 따로

```bat
python pipeline.py --build-only
python pipeline.py --test-only
```

### 6.3 특정 스위트만

```bat
python pipeline.py --test-only --suite file_load
```

선택 가능한 스위트: `layout`, `file_load`, `query_input`, `data_summary`, `json_load`, `export`, `dashboard`, `format_editor`

### 6.4 결과 확인

| 항목 | 경로 |
|------|------|
| 구조화된 결과(JSON) | `pipeline_result.json` |
| 테스트 결과(텍스트) | `tests/test_results.txt` |
| 스크린샷 | `tests/screenshots/run_YYYYMMDD_HHMMSS/` |

> CLAUDE.md §4 규칙: 파이프라인 실행 후 **반드시 스크린샷 이미지를 직접 시각 확인**할 것. 텍스트 로그만으로 PASS/FAIL을 판단하면 잘못된 결론에 이를 수 있습니다.

---

## 7. API 키 설정

LLM 기능(자연어 분석·차트 생성)을 사용하려면 API 키가 필요합니다. 키는 **DPAPI로 암호화**되어 `HKCU\Software\DeepMetria\Settings`에 저장됩니다 (평문 키는 디스크에 남지 않음).

### 7.1 절차

```bat
REM 1) 템플릿을 복사
copy secrets.json.template secrets.json

REM 2) secrets.json을 열어 사용할 프로바이더의 키 입력
REM    {
REM      "provider": "Gemini",
REM      "model":    "gemini-2.5-flash",
REM      "keys": {
REM        "Gemini": "AIza...",
REM        "Claude": "",
REM        "OpenAI": ""
REM      }
REM    }

REM 3) 등록
"docs\build manual\setup-secrets.bat"

REM (성공 시 secrets.json은 0으로 덮어쓴 뒤 자동 삭제됨)
```

### 7.2 키 교체

키를 바꾸고 싶으면 `secrets.json`을 다시 만들어 `setup-secrets.bat`을 다시 돌리면 됩니다. 기존 레지스트리 값은 덮어쓰기 됩니다.

### 7.3 검증

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify-key.ps1
```

레지스트리에 키가 올라가 있는지, 복호화가 정상인지 확인합니다.

---

## 8. 자주 보는 오류와 대처

| 증상 | 원인 / 해결 |
|------|-------------|
| `MSBuild.exe`를 찾을 수 없음 | VS 2022 Community 설치 경로가 다름. `build.bat`의 경로 수정 |
| `vcvars64.bat` 호출 실패 | MSVC 빌드 도구(v14.44) 미설치. Visual Studio Installer에서 추가 |
| `LNK2019 unresolved external symbol` | 풀 클린 후 재빌드 → `"docs\build manual\rebuild.bat"` |
| 실행 시 즉시 종료 / DLL 누락 | `vcredist_x64.exe`(VS C++ 재배포 패키지) 미설치 또는 Debug 빌드를 다른 PC에서 실행 시도 |
| "API 인증에 실패했습니다" | 키 미등록·만료. §7 절차로 재등록 |
| "사용량 한도", `QUOTA_EXCEEDED`, 429 | LLM 무료 쿼터 소진. 모델/프로바이더 변경 또는 24h 대기 |
| Python 파이프라인이 EXE를 못 찾음 | 빌드가 안 됐거나 `--config Release` 옵션을 빠뜨림 |
| 스크린샷 폴더가 비어 있음 | 자동화 도구가 창 포커스를 못 잡음. `.claude/temp/sitecustomize.py`의 `SetForegroundWindow` 우회 PATH가 적용됐는지 확인 |

---

## 9. 참고 경로

| 위치 | 내용 |
|------|------|
| `<root>/build.bat` | 원본 증분 빌드 스크립트 |
| `<root>/rebuild.bat` | 원본 클린 재빌드 스크립트 |
| `<root>/pipeline.py` | 빌드+테스트+분석 파이프라인 |
| `<root>/scripts/setup-secrets.ps1` | DPAPI 키 등록 |
| `<root>/scripts/verify-key.ps1` | DPAPI 키 검증 |
| `<root>/secrets.json.template` | API 키 입력 양식 |
| `<root>/DeepMetria.vcxproj` | MSBuild 프로젝트 파일 |
| `<root>/x64/Debug/DeepMetria.exe` | Debug 산출물 |
| `<root>/x64/Release/DeepMetria.exe` | Release 산출물 |
| `<root>/tests/test_runner.py` | UI 테스트 러너 |
| `<root>/tests/screenshots/` | 회차별 스크린샷 |
| `<root>/pipeline_result.json` | 가장 최근 파이프라인 결과 |
| `<root>/docs/dev/Architecture.md` | 시스템 구조 문서 |
| `<root>/docs/dev/DetailedSpec.md` | 상세 설계서 |
| `<root>/docs/dev/handover.md` | 인수인계서 |

---

문서 갱신 조건: 빌드 도구 버전 변경, 스크립트 추가/이름 변경, API 키 저장 방식 변경 시.
