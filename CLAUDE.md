# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 작업 언어

한국어. 코드 식별자/주석/문서 모두 한국어 우선이며, 외부 라이브러리 용어는 원어 유지.

## 프로젝트 개요

DeepMetria MFC — Windows 데스크톱 LLM 데이터 분석 앱 (MFC C++, SQLite, WinHTTP, GDI+). 현재 브랜치는 `clean-base`로 **소스 코드가 아직 비어 있다**. 요구사항은 `@docs/SRS.md`(IEEE 830), 빌드·실행·키 등록 절차는 `@docs/build manual/README.md`에 정의돼 있다.

## 빌드 · 실행 (비표준)

루트 스크립트(`build.bat` 등)는 아직 추가되지 않았으며, 코드가 들어오면 `docs/build manual/`의 래퍼들이 그것들을 호출한다. 명령은 **항상 프로젝트 루트**에서 실행한다.

| 작업 | 명령 | 비고 |
|------|------|------|
| Debug 증분 빌드 | `"docs\build manual\build.bat"` (또는 루트 `build.bat`) | 산출물 `x64\Debug\DeepMetria.exe` |
| Release 빌드 | `"docs\build manual\build.bat" Release` | 산출물 `x64\Release\DeepMetria.exe` |
| 클린 재빌드 | `"docs\build manual\rebuild.bat"` | Debug x64 풀빌드, 전체 로그는 `build_rebuild.log` |
| 실행 | `"docs\build manual\run.bat" [Release]` | 인자 없으면 Debug |

**툴체인 고정**: Visual Studio 2022 Community + MSVC v14.44.35207 + Windows 10 SDK. `build.bat`는 `C:\Program Files\Microsoft Visual Studio\2022\Community\...\vcvars64.bat`를 명시적으로 호출하므로 설치 경로가 다르면 그 줄을 직접 수정한다. 임의로 다른 MSVC 툴셋으로 바꾸지 말 것.

빈도 높은 오류:
- `LNK2019 unresolved external` → 먼저 `rebuild.bat`로 풀 클린 후 재빌드.
- `vcvars64.bat 호출 실패` → MSVC v14.44 미설치. VS Installer에서 추가.
- 실행 즉시 종료 / DLL 누락 → `vcredist_x64.exe`(VC++ 재배포 패키지) 설치.

## API 키 정책

LLM API 키는 **반드시 DPAPI로 암호화하여 `HKCU\Software\DeepMetria\Settings` 레지스트리에 저장**한다. 평문 키를 디스크(소스, 설정 파일, 로그)에 절대 남기지 않는다.

- 등록: `"docs\build manual\setup-secrets.bat"` (`secrets.json` 입력 → 등록 후 자동 삭제)
- 검증: `powershell -ExecutionPolicy Bypass -File scripts\verify-key.ps1`
- `secrets.json`은 어떤 경우에도 커밋하지 않는다 (`.gitignore` 필수).

키 미등록·만료 시 앱이 "API 인증에 실패했습니다"를 표시한다. 쿼터 소진(`QUOTA_EXCEEDED` / 429)은 코드 문제가 아니므로 모델·프로바이더를 바꾸거나 대기한다.

## 문서 컨벤션

`docs/SRS.md`(및 향후 `docs/dev/*.md`)는 상단 15줄이 **목차 + L-라인 넘버**, 16줄째부터 본문이다. 전체 로드 금지 — `limit:15`로 ToC만 먼저 읽고 필요한 섹션은 L값을 `offset`으로 접근한다. 섹션을 편집하면 영향 받은 L값을 갱신한다.

## 저장소 등 etiquette

- `origin`: `https://github.com/CatPope/DeepMetria_MFC.git`. 기본 작업 브랜치는 현재 `clean-base`.
- 커밋 메시지는 한국어, Conventional 스타일 권장 (`chore:`, `feat:`, `fix:` 등 — 기존 `cb7f233 chore: 빈 브랜치 초기화 (clean-base)` 형식 참고).
- `.omc/`, `x64/`, `build_rebuild.log`, `pipeline_result.json`, `secrets.json`, `tests/screenshots/`는 커밋하지 않는다.
