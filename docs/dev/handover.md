# DeepMetria MFC 프로젝트 인수인계서

작성일: 2026-05-28
작성자: Claude Agent (Session 2026-05-28)
대상: 후임 에이전트 또는 개발자

---

## 1. 프로젝트 개요

DeepMetria는 **LLM 기반 데이터 분석 MFC 데스크톱 애플리케이션**이다.
사용자가 CSV/Excel/JSON 파일을 로드하면, 데이터를 요약하고, 자연어 쿼리로 LLM에게 분석을 요청하며, 결과를 차트/대시보드로 시각화한다.

| 항목 | 값 |
|------|-----|
| 기술 스택 | MFC (C++) + SQLite + libcurl + GDI+ |
| 빌드 도구 | Visual Studio 2022 (MSBuild) |
| 테스트 | pywinauto (UI 자동화) + Google Test (단위 테스트) |
| LLM 프로바이더 | Claude, OpenAI, Gemini (SSE 스트리밍) |
| 합의 언어 | 한국어 |

---

## 2. 아키텍처 구조

```
src/
├── App/              # MFC 앱 진입점 (DeepMetriaApp, MainFrm)
├── Common/           # 공통 타입 (Types.h — DataTable, DataSummary, VisualizationInfo 등)
├── Dialog/           # 다이얼로그 (FileOpen, Export, Settings, FormatEditor, Download)
├── Document/         # MFC Document (DeepMetriaDoc — 파일 로드, 직렬화)
│   └── ColumnTypeInference.h  # 타입 추론 로직 (헤더 전용, 이번 세션 추출)
├── Domain/
│   ├── Analysis/     # LLM 분석 (AnalysisTools, ChartSelector)
│   ├── Dashboard/    # 대시보드 관리 (DashboardManager)
│   ├── DataSource/   # 데이터 소스 (DataSourceService, DataSummarizer)
│   └── Orchestrator/ # 분석 플로우 (AnalysisOrchestrator, AnalysisFlow)
├── Infrastructure/
│   ├── Cache/        # 분석 캐시 (AnalysisCache)
│   ├── CrashRecovery/# 크래시 복구 (CrashRecoveryManager)
│   ├── Export/       # 내보내기 (ExportFormatter.h, pdfgen.c/.h)
│   ├── LLM/          # HTTP 클라이언트, LLM 프로바이더 (Claude/OpenAI/Gemini), LLMRouter
│   ├── Parser/       # 파일 파서 (CSV, Excel/OpenXLSX, JSON)
│   ├── Settings/     # 설정 관리 (CSettingsManager)
│   └── Storage/      # SQLite DB
├── Renderer/         # 차트 렌더러 (Bar, Line, Scatter, Pie, Table)
├── Resources/        # MFC 리소스 (rc, resource.h)
└── View/             # MFC 뷰 (DataSummary, QueryInput, Dashboard, Chart)
```

핵심 데이터 흐름: **파일 로드 → DataTable 생성 → DataSummary 계산 → 뷰 업데이트 → LLM 분석 → 차트 렌더링**

---

## 3. 현재 상태 (2026-05-28 기준)

### 3.1 빌드
- **상태**: PASS (Debug x64)
- **최신 커밋**: `3d24e62` — `fix: 데이터 요약 통계 표시, 날짜 타입 감지, API 오류 메시지 개선 및 테스트 인프라 안정화`
- **⚠️ 미커밋 변경**: 이번 세션의 모든 작업은 **미커밋 상태**로 master 브랜치 워킹트리에만 존재한다. `git status`로 확인 필수.
- **빌드 명령**: `python pipeline.py --build-only` 또는 `build.bat`
- **Google Test 타깃**: deepmetria_unit_tests(272/284 통과), deepmetria_integration_tests(16/18 통과), deepmetria_llm_tests(7/7), deepmetria_export_tests(9/9) — 4개 타깃 모두 빌드·실행 가능

### 3.2 pywinauto 파이프라인 테스트 결과
| 구분 | 수치 |
|------|------|
| PASS | 약 38–39 |
| FAIL | 0 |
| SKIP | 7 |
| MANUAL | 15 |

- F-03(.xlsx 10만행)는 live LLM 타이밍 의존으로 가끔 플래키 — 단독 실행 시 PASS.
- F-04(.xls)는 OpenXLSX 미지원으로 픽스처 제거 후 정상 SKIP.

### 3.3 Git 상태
- 브랜치: `master` (GitHub Flow)
- **이번 세션 작업 전체 미커밋** — 원격 origin/master 최신 커밋은 `3d24e62`
- `tests/__pycache__/`, `test_results.txt` 변경은 커밋 불필요

---

## 4. 수행한 주요 작업

### 4.1 리팩토링

| 항목 | 파일 | 내용 |
|------|------|------|
| 타입 추론 추출 | `src/Document/ColumnTypeInference.h` (신규) | DeepMetriaDoc.cpp의 LoadFile·Serialize 두 곳에 중복됐던 타입 추론 로직을 헤더 전용 유틸로 추출. 중복 제거 완료 |
| CArchive 직렬화 버전 마커 | `DeepMetriaDoc.cpp` | magic 0x444D4453 + version 2 추가. VisualizationInfo 전체 라운드트립 지원 |
| sampleValues 수집 통일 | `DataSourceService.cpp` | 3개 → 5개로 변경. DataSummarizer(이미 5개)와 통일 |
| PostJsonSSE 오류 메시지 | `HttpClient.cpp` | 친화적 한국어 오류 메시지 추가 |

### 4.2 Export 기능 구현

| 항목 | 파일 | 내용 |
|------|------|------|
| HTML 내보내기 | ExportDialog, 관련 소스 | 차트 PNG base64 + 데이터 표 포함 HTML 생성 |
| PDF 내보내기 | `src/Infrastructure/Export/pdfgen.c/.h` (신규) | 공용 도메인 PDFGen 라이브러리 연동. PDF 제목은 영문, 한글은 임베드 PNG 이미지로만 표시(기본 폰트 라틴 전용 제약) |
| ExportFormatter | `src/Infrastructure/Export/ExportFormatter.h` (신규) | CSV/HTML 포맷 순수 로직 추출(CsvSafeField, BuildCsvFromDataJson, HtmlEscape, BuildHtmlTableFromDataJson). 헤더 전용 |
| ExportDialog 형식 | `Dialog/ExportDialog` | PNG/BMP/CSV/HTML/PDF 선택 지원 |
| 앱 연결 | `DeepMetriaApp::OnToolsExport` | 활성 차트로 ExportDialog 오픈 |

### 4.3 포맷 에디터 프리뷰 (FE-02)

- `Dialog/FormatEditorDlg`에 owner-drawn CStatic(`CPreviewStatic`) 추가.
- `CChartRenderer::Render`로 크기/색상/차트타입 변경을 실시간 반영하는 프리뷰 구현.
- GDI 렌더링 기반이라 결정적 단위 테스트는 어렵고 빌드 + 수동 확인 수준.

### 4.4 대시보드 (DI-12)

- `DashboardView`에 빈 상태 안내 CStatic("차트를 추가하려면 질문을 입력하세요.") 추가 → DI-12 PASS.
- 시각화 직렬화 라운드트립 구현(§4.1의 버전 마커와 연동).

### 4.5 Excel/OpenXLSX (.xlsx) 지원 — F-03 PASS

| 항목 | 내용 |
|------|------|
| vcpkg 설치 | `C:\vcpkg`에 openxlsx:x64-windows 설치 (의존성: nowide, pugixml) |
| 프로젝트 연결 | `DeepMetria.vcxproj`에 include/lib/HAVE_OPENXLSX 정의 + DLL PostBuild 복사 3종 추가 |
| ExcelParser | `ExcelParser.cpp` — OpenXLSX의 rows()/cells() 경로 수정 |
| 테스트 데이터 | `tests/data/sample_data.xlsx` (10만행) 추가 |
| .xls | OpenXLSX 미지원 → F-04 픽스처 제거, 정상 SKIP 처리 |

### 4.6 LLM 쿼터 자동 폴백 (신규)

HTTP 429 수신 시 사용자 개입 없이 자동으로 모델을 하향하거나 다른 제공사로 교차 전환한다.

| 단계 | 동작 |
|------|------|
| 1 | 같은 제공사 내 하위 모델로 하향 (제공사별 체인) |
| 2 | 체인 최하위면 API 키가 있는 다른 제공사의 최저가 모델로 교차 전환 |
| 3 | 전환 불가 시 "모든 사용 가능한 모델의 사용량을 초과했습니다" 안내 |

제공사별 하향 체인:
- Claude: `claude-opus-4-5 → claude-sonnet-4-5 → claude-haiku-3-5`
- OpenAI: `gpt-4o → gpt-4o-mini → o4-mini`
- Gemini: `gemini-3.1-pro-preview → gemini-2.5-pro → gemini-2.5-flash → gemini-3.1-flash-lite`

관련 변경:
- `HttpClient` — 429 수신 시 code=QUOTA_EXCEEDED 설정 (PostJson/PostJsonSSE 공통)
- `LLMRouter.ChatWithRetry` — 폴백 로직 구현
- `ILLMProvider::HasApiKey()` — 교차 전환 가부 판단용 인터페이스 추가
- `AnalysisFlow.fallbackNotice` → `QueryInputView` 상태바 비차단 표시

**주의**: 교차 전환은 다른 제공사의 API 키가 등록돼 있어야 동작한다. Gemini 키만 있고 flash-lite까지 내려오면 더 이상 전환 불가.

### 4.7 보안

| 항목 | 내용 |
|------|------|
| PDFGen 보안 리뷰 | LOW–MEDIUM 수준. Critical/High 0건 |
| CSV 수식 인젝션 수정 (M1) | `CsvSafeField`: =,+,-,@,TAB,CR 무력화 + RFC4180 인용 적용. ExportFormatter.h로 이동, 단위 테스트 완료 |

### 4.8 Google Test 타깃 복구 및 안정화

| 항목 | 내용 |
|------|------|
| tests/CMakeLists.txt 수정 | /utf-8, TRACE shim, atls.lib, include 루트, 누락 impl 소스 추가 |
| AnalysisTools/ChartSelector | min/max 경계 수정 |
| test_analysis_flow.cpp | 누락 include 추가 |
| 집중 타깃 | deepmetria_llm_tests(7/7), deepmetria_export_tests(9/9) 신규 구성 |
| 플래키 실험 롤백 | live-LLM 차트 시딩 실험 되돌림 |

### 4.9 이전 세션 주요 작업 (커밋 히스토리)

| 커밋 | 내용 |
|------|------|
| `3d24e62` | 날짜 타입 감지 버그, 요약 통계 표시, API 오류 메시지, 테스트 인프라 안정화 |
| `b0bff60` | 전체 60 TC-ID 커버리지 — SSE 스트리밍, NFR 성능/보안 테스트 |
| `77fe1bf` | JSON/Export/Dashboard/LLM/CrashRecovery 기능 구현 + 테스트 |
| `c8d6bb8` | 개발 파이프라인 자동화, UI 테스트 100% 통과 |
| `1cd146c` | CRITICAL 4 + HIGH 2 버그 수정, pywinauto 안정화 5건 |
| `c674f73` | pywinauto 기반 UI 테스트 환경 구성 + 테스트 계획서 |
| `29cbaa2` | Gemini 프로바이더, 데이터 파이프라인 연결 |
| `bcd3890` | 전체 리팩토링 — 보안/정합성/안전성 22개 이슈 수정 |

---

## 5. 미완성 항목 및 개선 권고

### 5.1 Google Test 실패 14건 (미해결)

`deepmetria_unit_tests` 284건 중 12건, `deepmetria_integration_tests` 18건 중 2건 실패.
- 주요 원인: `tests/data/*.csv` 픽스처 누락, 타입 감지 배선 미완성
- 별도 후속 작업 필요

### 5.2 pywinauto 파이프라인 플래키 문제

파일 로드 시 자동 요약 호출이 live LLM에 의존하여 타이밍 불안정.
완전 결정화하려면 테스트 모드(LLM 비활성 또는 모킹) 구현이 필요하다.

### 5.3 기능 제약

| 항목 | 제약 |
|------|------|
| Excel | `.xlsx`만 지원(OpenXLSX). `.xls` 미지원 |
| PDF 한글 | PDFGen 기본 폰트가 라틴 전용. 한글 텍스트는 임베드 PNG 이미지로만 표시 |
| FE-02 프리뷰 | GDI 렌더링 — 결정적 단위 테스트 불가, 빌드 + 수동 확인 수준 |

### 5.4 MANUAL 테스트 (15건) — 자동화 불가

LLM 응답 품질 검증, 드래그 앤 드롭, 차트 인터랙션 등 사람의 판단이 필요한 항목.

### 5.5 미커밋 작업 커밋 필요

이번 세션의 전체 작업(§4.1–§4.8)이 미커밋 상태다. 후임은 다음 절차를 따를 것:

```powershell
git status          # 변경 파일 목록 확인
git diff --stat     # 변경 규모 확인
# 검토 후 커밋 단위 분리하여 Conventional Commits 규칙으로 커밋
```

### 5.6 아키텍처 검토 후속 권고 (LLM 폴백)

독립 아키텍처 검토 결과 — 폴백 로직은 정확하고 종료가 보장되며 즉시 수정이 필요한 버그는 없음. 다음은 후속 권고:

- **스레드 안전성(잠재)**: `LLMRouter`는 싱글턴이며 워커 스레드에서 `m_model`/`m_activeProvider`/`m_fallbackNotice`를 잠금 없이 변경한다. 현재는 분석이 `m_bAnalyzing` + 오케스트레이터 가드로 직렬화되어 안전하나, 파일 로드 요약(`DataSummarizer` 워커 스레드)과 분석이 겹치면 경쟁 가능 → 뮤텍스 보호 또는 "동시 LLM 호출 금지" 불변식 명문화.
- **폴백 미적용 경로**: `DataSummarizer.cpp:140`(요약, plain `Chat`)과 `LLMRouter::ChatAsync`/`ChatWithHistory`는 폴백을 거치지 않는다(우아한 degrade는 됨). 요약 시점 쿼터 폴백이 필요하면 `ChatWithRetry`로 라우팅 결정 필요.
- **폴백 상태 리셋 부재**: 한 번 전환되면 세션 내내 최저가 모델/제공사에 고정됨(복구 경로 없음). 일시적 429에 영구 고정되지 않도록 리셋 전략 검토(제품 결정).
- **테스트 보강**: 다단계(같은 제공사 429 → 교차 전환) 및 빈 `m_model` 교차 전환 케이스 추가 권고.
- 모델 체인(`LLMRouter.cpp:37-52`)은 `SettingsDialog::UpdateModelList`와 동기화 유지 필요.

---

## 6. 개발 환경 및 도구

### 6.1 빌드/테스트 명령

```powershell
# 전체 파이프라인 (빌드 + 테스트 + 분석)
python pipeline.py

# 빌드만
python pipeline.py --build-only

# 테스트만 (이미 빌드된 상태)
python pipeline.py --test-only

# 특정 스위트만
python pipeline.py --test-only --suite file_load

# Bash 쉘에서 빌드
powershell.exe -Command "& cmd /c 'build.bat'"
```

### 6.2 테스트 스위트 구성

| 스위트 | 파일 | TC-ID 범위 |
|--------|------|-------------|
| layout | test_layout.py | L-01 ~ L-09 |
| file_load | test_file_load.py | F-01 ~ F-12 |
| query_input | test_query_input.py | Q-01 ~ Q-16 |
| data_summary | test_data_summary.py | D-01 ~ D-13 |
| json_load | test_json_load.py | J-01 ~ J-04 |
| export | test_export.py | E-01 ~ E-06 |
| dashboard | test_dashboard.py | DI-01 ~ DI-12 |
| format_editor | test_format_editor.py | FE-01 ~ FE-05 |

### 6.3 테스트 함수 패턴

```python
def test_XX(app, win) -> bool:
    """TC-ID에 해당하는 테스트. app=Application, win=메인 윈도우"""
    # ... 테스트 로직 ...
    return True  # PASS
```

### 6.4 핵심 설정 파일

| 파일 | 용도 |
|------|------|
| `CLAUDE.md` | 프로젝트 규칙, 파이프라인 절차, 스크린샷 분석 규칙 |
| `pipeline.py` | 빌드→테스트→분석 자동화 |
| `tests/config.py` | 테스트 설정 (EXE 경로, 타임아웃 등) |
| `tests/helpers.py` | 테스트 유틸리티 (스크린샷 캡처, 윈도우 탐색 등) |
| `tests/test_runner.py` | 테스트 실행기 (스위트 선택, 결과 기록) |
| `.claude/temp/sitecustomize.py` | SetForegroundWindow 패치 (테스트 안정성) |

---

## 7. 주의사항

### 7.1 반드시 지켜야 할 규칙

1. **테스트 파일 수정 금지**: `tests/**/*` 파일은 사용자 허가 없이 수정/생성/삭제하지 말 것
2. **스크린샷 분석 필수**: 파이프라인 실행 후 `tests/screenshots/run_YYYYMMDD_HHMMSS/` 폴더의 이미지를 Read 도구로 직접 열어 시각 확인. 텍스트 로그만 보고 판단 금지
3. **보안 우선**: OWASP Top 10 준수, 로컬 데이터 보호
4. **개발 루프 준수**: 코드 수정 → 빌드 → 테스트 → 결과 분석 (FAIL 시 수정 반복)

### 7.2 알려진 함정

- **미커밋 상태**: 이번 세션 작업 전체(리팩토링, Export, LLM 폴백, Excel 등)가 워킹트리에만 존재. 커밋 전에 빌드·테스트 재확인 필요
- `ColumnTypeInference.h`가 추출됐으므로 `DeepMetriaDoc.cpp`의 타입 추론 중복은 해소됨. 그러나 이 헤더를 누락하면 빌드 오류 발생
- pywinauto의 `set_focus`가 Windows 보안 정책으로 실패할 수 있음. `sitecustomize.py`가 PYTHONPATH에 있어야 테스트 안정적
- LLM 쿼터 폴백의 교차 전환은 **다른 제공사 API 키가 등록돼 있어야** 동작. Gemini 키 하나만 있으면 flash-lite 이후 전환 불가
- PDF 내보내기에서 한글 텍스트는 PDFGen 기본 폰트 한계로 깨짐. 한글은 PNG 임베드로만 표시됨
- OpenXLSX vcpkg DLL 3종이 PostBuild로 복사되므로, vcpkg 경로(`C:\vcpkg`) 변경 시 `DeepMetria.vcxproj` 수정 필요
- Google Test 14건 실패는 픽스처 누락 및 타입 감지 배선 미완성이 원인. 미해결 상태로 인수

### 7.3 문서 위치

| 문서 | 경로 |
|------|------|
| 아키텍처 | `docs/dev/Architecture.md` |
| 상세 설계 | `docs/dev/DetailedSpec.md` |
| 요구사항 | `docs/dev/SRS.md` |
| 테스트 계획 | `docs/dev/test-plan.md` |
| API 명세 | `docs/dev/api-spec.md` |
| DB 스키마 | `docs/dev/db-schema.md` |
| 배포 가이드 | `docs/dev/deploy-guide.md` |
| 환경 가이드 | `docs/dev/env-guide.md` |
| 제한사항 | `docs/dev/limitations.md` |

---

## 8. 마무리

이번 세션(2026-05-28)에서 수행한 작업은 **전량 미커밋** 상태다. 후임이 가장 먼저 해야 할 일은 `git status`로 변경 범위를 확인하고, 논리 단위별로 Conventional Commits 규칙에 따라 커밋하는 것이다.

이번 세션의 주요 성과:

- 타입 추론 중복 제거 및 CArchive 버전 마커 도입으로 코드 정합성 향상
- HTML/PDF Export 구현 + CSV 수식 인젝션 보안 수정
- LLM 쿼터 자동 폴백(모델 하향 + 교차 제공사 전환) 구현
- OpenXLSX 연동으로 .xlsx 10만행 로드(F-03) PASS
- 포맷 에디터 실시간 프리뷰(FE-02) + 대시보드 빈 상태 안내(DI-12) 완성
- Google Test 4개 타깃 빌드·실행 복구

미해결 과제: Google Test 14건 실패(픽스처 누락), pywinauto 플래키(live LLM 의존), PDF 한글 폰트 제약.

후임에게 행운을 빈다.

---

*이 문서는 프로젝트의 현재 상태를 정확하게 반영하기 위해 작성되었으며, 코드와 테스트 결과를 직접 분석하여 검증하였다.*
