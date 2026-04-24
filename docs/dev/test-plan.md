# DeepMetria 테스트 계획서 — MFC C++ 데스크톱 분석 애플리케이션

## 목차
1. 테스트 범위 ................................................ L17
2. 테스트 유형 및 도구 ........................................ L41
3. 테스트 환경 ................................................ L102
4. 테스트 케이스 (FR별) ...................................... L147
5. 합격 기준 .................................................. L263
6. 리스크 및 대응 전략 ........................................ L307





---

## 1. 테스트 범위

### 1.1 테스트 대상

| 범주 | 대상 |
|------|------|
| 기능 요구사항 | FR-01 ~ FR-08 (파일 로드, AI 요약, 자연어 질문, CoT 분석, 시각화 생성, 서식 편집, 내보내기, 멀티 LLM) |
| C++ 클래스 | CFileParser, CLLMRouter, CAnalysisTool (13개), CAuthManager, CDashboardView, CChartRenderer |
| MFC 다이얼로그/뷰 | CMainFrame, CDashboardView, CQueryInputDlg, CFormatEditorDlg, CDownloadDlg |
| 백엔드 도메인 로직 | 파일 파서(CSV/Excel/JSON), LLM Router, AnalysisTool 13개, 스트리밍(HTTP SSE) |
| 비기능 요구사항 | NFR-01 성능, NFR-02 사용성, NFR-03 신뢰성 |
| 보안 | 메모리 안전, 로컬 파일 보안, DLL 인젝션 방지, API 키 보호 |

### 1.2 테스트 제외 범위

| 제외 항목 | 이유 |
|-----------|------|
| 과금/결제 시스템 | post-MVP 범위 외 |
| 온프레미스 서버 배포 | post-MVP 범위 외 |
| 실제 LLM API 호출 | 비결정성 및 비용 문제 → mock으로 대체 |
| 외부 클라우드 스토리지 | 외부 의존성 → 로컬 파일 시스템 mock으로 대체 |

---

## 2. 테스트 유형 및 도구

### 2.1 테스트 유형 매트릭스

| 유형 | 도구 | 대상 | 커버리지 목표 |
|------|------|------|--------------|
| C++ 단위 테스트 | Google Test (gtest) | 도메인 로직, AnalysisTool 13개, 파일 파서, LLM Router | ≥ 80% |
| MFC UI 단위 테스트 | Visual Studio 단위 테스트 프레임워크 | MFC 다이얼로그/뷰 컴포넌트 | ≥ 80% |
| 통합 테스트 | Google Test + HTTP mock | 클래스 간 연동, LLM API 통신 레이어 | ≥ 80% |
| E2E / UI 자동화 | Windows UI Automation (UIA) / AutoIt | 핵심 플로우 (파일 로드→요약→질문→시각화→내보내기) | 핵심 플로우 100% |
| 성능 테스트 | Visual Studio Profiler / VTune | 파일 파싱, 렌더링, 메모리 사용량 | NFR-01 기준 충족 |
| 보안 테스트 | 정적 분석(PVS-Studio/Coverity) + 수동 점검 | 메모리 안전, DLL 인젝션, 로컬 파일 보안 | 위반 0건 |

### 2.2 C++ 단위 테스트 세부 대상

```
tests/unit/
├── test_file_parser.cpp        # CSV, Excel, JSON 파서
├── test_llm_router.cpp         # LLM Router 추상화 (mock LLM)
├── test_analysis_tools.cpp     # AnalysisTool 13개
├── test_auth_manager.cpp       # 자격증명 저장/로드, API 키 검증
├── test_datasource_service.cpp # DataSource 생성/검증 로직
├── test_streaming.cpp          # HTTP SSE 이벤트 파싱 로직
└── test_settings_manager.cpp   # 앱 설정 저장/로드 (레지스트리/INI)
```

### 2.3 MFC UI 단위 테스트 세부 대상

```
tests/ui/
├── test_main_frame.cpp         # CMainFrame 메뉴/툴바 동작
├── test_dashboard_view.cpp     # CDashboardView 렌더링/레이아웃
├── test_query_input_dlg.cpp    # CQueryInputDlg 입력 검증
├── test_chart_renderer.cpp     # CChartRenderer 차트 타입별 렌더링
├── test_format_editor_dlg.cpp  # CFormatEditorDlg 스타일 편집
└── test_download_dlg.cpp       # CDownloadDlg 파일 내보내기
```

### 2.4 통합 테스트 세부 대상

```
tests/integration/
├── test_file_load_flow.cpp     # 파일 로드 → 파서 → DataSource 생성
├── test_llm_api_client.cpp     # HTTP 클라이언트 → LLM API mock 통신
├── test_analysis_flow.cpp      # 질문 입력 → AnalysisTool 선택 → 결과
├── test_chart_generation.cpp   # 분석 결과 → ChartRenderer 시각화
└── test_export_flow.cpp        # 시각화 → 이미지/파일 내보내기
```

### 2.5 E2E 시나리오 (Windows UI Automation)

```
e2e/
├── load_to_summary.au3         # 파일 로드 → 요약 자동 생성
├── query_to_visualization.au3  # 자연어 질문 → 시각화 생성
├── format_and_export.au3       # 서식 편집 → 내보내기
└── multi_llm_switch.au3        # LLM 전환 후 기능 동작
```

---

## 3. 테스트 환경

### 3.1 로컬 빌드 환경

| 항목 | 구성 |
|------|------|
| IDE | Visual Studio 2022 (v143 toolset) |
| OS | Windows 10 (22H2) / Windows 11 |
| 아키텍처 | x64 (Release/Debug 모두 테스트) |
| 테스트 프레임워크 | Google Test 1.14+ (vcpkg 또는 NuGet) |
| LLM | HTTP mock 서버 (WinHTTP stub) — 실제 API 호출 없음 |
| 파일 스토리지 | 로컬 임시 디렉터리 (`%TEMP%\deepmetria_test\`) |

### 3.2 CI 환경 (GitHub Actions — Windows Runner)

```yaml
# .github/workflows/test.yml 기준
- runs-on: windows-latest
- Visual Studio 2022 + MSBuild
- vcpkg (Google Test, nlohmann-json, libcurl)
- Google Test XML 보고서 출력
- Visual Studio Profiler (성능 기준선 측정)
```

### 3.3 Windows 버전 호환성 테스트

| Windows 버전 | 테스트 대상 | 비고 |
|-------------|-----------|------|
| Windows 10 22H2 | 전체 기능 | 기본 지원 환경 |
| Windows 11 23H2 | 전체 기능 | 최신 환경 |
| Windows 10 LTSC 2019 | 핵심 기능 | 기업 환경 |

### 3.4 테스트 데이터 (Fixtures)

| 파일 | 내용 | 용도 |
|------|------|------|
| `tests/fixtures/sample_sales.csv` | 100행 매출 데이터 | 기본 파일 로드/분석 테스트 |
| `tests/fixtures/sample_data.xlsx` | 멀티시트 Excel | Excel 파서 테스트 |
| `tests/fixtures/sample_data.json` | 중첩 JSON | JSON 파서 테스트 |
| `tests/fixtures/large_file_50mb.csv` | 50MB 경계값 | 파일 크기 제한 테스트 |
| `tests/fixtures/invalid_file.txt` | 지원 불가 형식 | 오류 처리 테스트 |
| `tests/fixtures/malicious_script.bat` | 악성 파일 시도 | 보안 파일 검증 테스트 |

---

## 4. 테스트 케이스 (FR별)

### 4.1 FR-01: 데이터 파일 로드

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-01-01 | CSV 파일 정상 로드 | 통합 | sample_sales.csv | CDataSource 생성, status=ready |
| TC-01-02 | Excel(.xlsx) 파일 정상 로드 | 통합 | sample_data.xlsx | CDataSource 생성 |
| TC-01-03 | Excel(.xls) 파일 정상 로드 | 통합 | sample_data.xls | CDataSource 생성 |
| TC-01-04 | JSON 파일 정상 로드 | 통합 | sample_data.json | CDataSource 생성 |
| TC-01-05 | 지원 불가 형식 로드 | 통합 | invalid_file.txt | UNSUPPORTED_FILE_TYPE 예외 |
| TC-01-06 | 50MB 초과 파일 로드 | 통합 | large_file_51mb.csv | FILE_TOO_LARGE 예외 |
| TC-01-07 | 50MB 경계값 파일 로드 | 통합 | large_file_50mb.csv | 정상 처리 |
| TC-01-08 | 파일 드래그앤드롭 | E2E | CSV 파일 DnD | 파일 로드 진행 상태 표시됨 |
| TC-01-09 | 파일 선택 다이얼로그 | E2E | CFileDialog 선택 | 파일 로드 진행 상태 표시됨 |
| TC-01-10 | 악성 파일 로드 시도 | 보안 | malicious_script.bat | UNSUPPORTED_FILE_TYPE 예외 |

### 4.2 FR-02: AI 데이터 자동 요약

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-02-01 | 파일 로드 후 SSE 스트림 수신 | 통합 | datasource_id | stage: loading→parsing→summarizing→done |
| TC-02-02 | summary_complete 이벤트 포함 확인 | 통합 | datasource_id | domain, row_count, statistics 포함 |
| TC-02-03 | DataSource 스키마 포함 확인 | 단위(C++) | CDataSource 객체 | column, dtype, nullable 배열 반환 |
| TC-02-04 | 기본 대시보드 자동 구성 확인 | 통합 | summary_complete 후 | CDashboardView에 위젯 배치 |
| TC-02-05 | 요약 뷰 렌더링 | 단위(UI) | CDataSummary 객체 | 컬럼 스키마, 도메인, 통계 렌더링됨 |
| TC-02-06 | LLM mock 요약 생성 도메인 로직 | 단위(C++) | CSV 파서 출력 | domain, statistics, row_count 포함 구조체 반환 |

### 4.3 FR-03: 자연어 질문 입력

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-03-01 | 정상 질문 제출 | 통합 | "월별 매출 추이" | flow_id 생성, 분석 시작 |
| TC-03-02 | 빈 질문 제출 | 단위(UI) | 빈 문자열 | EMPTY_QUESTION 오류 메시지 박스 |
| TC-03-03 | 존재하지 않는 datasource | 단위(C++) | 잘못된 ID | DATASOURCE_NOT_FOUND 예외 |
| TC-03-04 | 질문 입력 UI 로딩 상태 표시 | E2E | 질문 제출 후 | 프로그레스 바 / 대기 커서 표시됨 |
| TC-03-05 | CQueryInputDlg 제출 동작 | 단위(UI) | 텍스트 입력 후 Enter | OnSubmit 핸들러 호출됨 |

### 4.4 FR-04: AI 분석 도구 선택 및 호출 (CoT)

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-04-01 | CoT SSE 스트림 — cot_step 이벤트 수신 | 통합 | 분석 스트림 수신 | cot_step 이벤트 1개 이상 수신 |
| TC-04-02 | tool_call 이벤트 수신 | 통합 | 분석 스트림 | tool 이름이 정의된 AnalysisTool 목록 내 값 |
| TC-04-03 | tool_result 이벤트 수신 | 통합 | 분석 스트림 | tool_result 이벤트 포함, result 비어있지 않음 |
| TC-04-04 | AnalysisTool 13개 각 도구 단위 테스트 | 단위(C++) | 각 도구별 입력 fixture | 올바른 계산 결과 반환 |
| TC-04-05 | LLM Router — Claude mock 라우팅 | 단위(C++) | provider=L"claude" | claude mock 호출됨 |
| TC-04-06 | LLM Router — GPT mock 라우팅 | 단위(C++) | provider=L"openai" | openai mock 호출됨 |
| TC-04-07 | 정의되지 않은 도구 선택 시 오류 처리 | 단위(C++) | 존재하지 않는 tool명 | CToolNotFoundException 발생 |

### 4.5 FR-05: 차트 시각화 생성

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-05-01 | visualization_ready 이벤트 수신 | 통합 | 분석 스트림 완료 | visualization_id, type, config 포함 |
| TC-05-02 | 대시보드에 시각화 위젯 추가 | 통합 | CDashboardView | 신규 차트 위젯 배치됨 |
| TC-05-03 | 라인 차트 렌더링 | 단위(UI) | type=LINE, data=mock | GDI+ LineChart 렌더링됨 |
| TC-05-04 | 바 차트 렌더링 | 단위(UI) | type=BAR, data=mock | GDI+ BarChart 렌더링됨 |
| TC-05-05 | 테이블 시각화 렌더링 | 단위(UI) | type=TABLE, data=mock | CListCtrl 행/열 올바르게 렌더링됨 |
| TC-05-06 | E2E — 질문 후 차트 대시보드 표시 | E2E | "월별 매출 추이 보여줘" | 차트 위젯이 대시보드에 나타남 |

### 4.6 FR-06: 시각화 서식 편집

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-06-01 | 시각화 크기 변경 | 단위(UI) | CFormatEditorDlg 크기 조절 | OnSizeChange 핸들러 호출됨 |
| TC-06-02 | 시각화 위치 변경 (드래그) | E2E | 차트 위젯 드래그 | 레이아웃 갱신됨 |
| TC-06-03 | 시각화 스타일(색상) 변경 | 단위(UI) | 색상 선택 컨트롤 | OnColorChange 핸들러 호출됨 |
| TC-06-04 | CFormatEditorDlg 슬라이더 조작 | 단위(UI) | 높이/너비 슬라이더 | onChange 핸들러 호출됨 |
| TC-06-05 | E2E — 서식 변경 즉시 반영 | E2E | 색상 변경 | 대시보드에 즉시 시각적 반영 |

### 4.7 FR-07: 시각화 내보내기

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-07-01 | PNG 내보내기 | 통합 | GDI+ Bitmap 저장 | .png 파일 생성됨 |
| TC-07-02 | 대시보드 전체 내보내기 | 통합 | 전체 캡처 → 파일 저장 | 파일 다운로드 응답 |
| TC-07-03 | CDownloadDlg 저장 버튼 동작 | 단위(UI) | 버튼 클릭 | CFileDialog 열리고 저장 트리거됨 |
| TC-07-04 | E2E — 내보내기 버튼 클릭 → 파일 저장 | E2E | 내보내기 버튼 클릭 | 지정 경로에 PNG 파일 생성됨 |

### 4.8 FR-08: 멀티 LLM 지원

| TC-ID | 테스트 케이스 | 유형 | 입력 | 기대 결과 |
|-------|-------------|------|------|----------|
| TC-08-01 | Claude mock으로 분석 완료 | 단위(C++) | provider=L"claude" | 정상 분석 결과 반환 |
| TC-08-02 | GPT mock으로 분석 완료 | 단위(C++) | provider=L"openai" | 정상 분석 결과 반환 |
| TC-08-03 | LLM 전환 후 동일 결과 구조 보장 | 단위(C++) | 두 provider 결과 비교 | 응답 스키마 동일 |
| TC-08-04 | 지원하지 않는 LLM provider | 단위(C++) | provider=L"unknown" | CUnsupportedProviderException 발생 |
| TC-08-05 | LLM API 오류 시 사용자 안내 | 통합 | LLM mock 에러 반환 | 오류 다이얼로그 표시, 재시도 안내 |

### 4.9 비기능 요구사항 테스트

#### 성능 (NFR-01)

| TC-ID | 테스트 케이스 | 도구 | 기준 |
|-------|-------------|------|------|
| TC-P-01 | 파일 로드 후 요약 생성 시간 | VS Profiler | ≤ 30초 (중소형 파일) |
| TC-P-02 | 자연어 질문 → 시각화 생성 시간 | VS Profiler | ≤ 60초 (LLM mock 포함) |
| TC-P-03 | 대시보드 초기 렌더링 시간 | VS Profiler | ≤ 3초 |
| TC-P-04 | UI 응답 시간 (버튼 클릭 → 반응) | VS Profiler | ≤ 200ms |
| TC-P-05 | 메모리 누수 없음 | VS Diagnostics | 장시간 실행 후 메모리 증가 < 10MB |

#### 보안 (NFR-보안)

| TC-ID | 테스트 케이스 | 도구 | 기준 |
|-------|-------------|------|------|
| TC-S-01 | API 키 레지스트리/파일 암호화 저장 | 수동 | 평문 저장 없음 |
| TC-S-02 | DLL 인젝션 방지 (DLL 사이드로딩) | 수동/PVS-Studio | 서명된 DLL만 로드 |
| TC-S-03 | 버퍼 오버플로 취약점 | 정적 분석 | 오류 0건 |
| TC-S-04 | 악성 파일 로드 (확장자 우회) | 수동 | MIME 검증으로 거부 |
| TC-S-05 | LLM API 키 메모리 덤프 노출 | 수동 | 메모리에서 평문 키 미검출 |
| TC-S-06 | 로컬 파일 접근 경로 탈출 방지 | 수동 | 지정 디렉터리 외 접근 불가 |
| TC-S-07 | 프로세스 권한 최소화 | 수동 | 관리자 권한 불필요 |

---

## 5. 합격 기준

### 5.1 정량적 기준

| 항목 | 합격 기준 |
|------|----------|
| 테스트 통과율 | 100% (모든 TC 통과) |
| 코드 커버리지 | ≥ 80% (OpenCppCoverage 또는 VS Coverage) |
| Cyclomatic 복잡도 | ≤ 10 (함수 단위, 정적 분석 게이트) |
| UI 응답 시간 | ≤ 200ms |
| 파일 요약 생성 시간 | ≤ 30초 (중소형 파일) |
| 자연어 질문 → 시각화 | ≤ 60초 |
| 대시보드 초기 렌더링 | ≤ 3초 |
| 크리티컬 버그 | 0건 |
| 보안 정적 분석 위반 | 0건 (High/Critical 등급) |

### 5.2 게이트 기준 (CI 블로킹)

아래 기준 미충족 시 PR 머지 차단:

1. C++ 단위/통합 테스트 전체 통과 (Google Test)
2. MFC UI 단위 테스트 전체 통과
3. 코드 커버리지 ≥ 80%
4. 정적 분석 오류 0건 (PVS-Studio / Coverity)
5. Cyclomatic 복잡도 초과 함수 0개

E2E 및 성능/보안 테스트는 배포 전 수동 게이트로 운영.

### 5.3 인수 기준 매핑 (SRS AC)

| AC-ID | 인수 기준 내용 | 대응 TC |
|-------|--------------|---------|
| AC-01 | CSV/Excel/JSON 파일 로드 후 데이터 요약 자동 생성 | TC-01-01~04, TC-02-01~02 |
| AC-02 | 지원 불가 형식 로드 시 오류 메시지 | TC-01-05, TC-01-10 |
| AC-03 | 요약에 컬럼 스키마, 도메인, 기본 통계 포함 | TC-02-03, TC-02-06 |
| AC-04 | 파일 로드 후 기본 대시보드 자동 구성 | TC-02-04 |
| AC-05 | 자연어 질문 UI 및 AI 분석 시작 피드백 | TC-03-04, TC-03-05 |
| AC-06 | AI가 적절한 분석 도구와 차트 유형 선택 | TC-04-01~03 |
| AC-07 | 시각화 대시보드 실시간 반영, 테이블 지원 | TC-05-01~02, TC-05-05 |
| AC-08 | 시각화 크기/위치/스타일 편집 가능 | TC-06-01~05 |
| AC-09 | 이미지(PNG 등)로 내보내기 가능 | TC-07-01, TC-07-04 |

---

## 6. 리스크 및 대응 전략

### 6.1 리스크 매트릭스

| 리스크 | 영향도 | 발생 가능성 | 대응 전략 |
|--------|--------|------------|----------|
| LLM 응답 비결정성 | 높음 | 높음 | 모든 LLM 호출을 WinHTTP mock stub으로 대체. fixture 응답 고정 |
| MFC UI 테스트 자동화 어려움 | 높음 | 높음 | Windows UI Automation + AutoIt으로 핵심 플로우만 자동화 |
| GDI+ 렌더링 환경 차이 (DPI, 해상도) | 중간 | 중간 | DPI-Aware 설정 테스트, 96/120/144 DPI 환경 검증 |
| 대용량 파일(50MB) 경계값 처리 | 중간 | 중간 | TC-01-07 경계값 테스트 전용 fixture 파일 생성 |
| SSE 스트리밍 파싱 불안정성 | 중간 | 중간 | WinHTTP 스트리밍 테스트 헬퍼 클래스 구현 |
| Windows 버전별 API 차이 | 낮음 | 중간 | Win10 최소 지원, Win11 최신 API는 기능 감지 후 사용 |
| 성능 테스트 환경 차이 (로컬 vs 운영) | 낮음 | 높음 | 성능 수치는 참고 기준으로 운영. 배포 후 재측정 |

### 6.2 테스트 실행 순서

```
1. MSBuild Debug 빌드
2. Google Test 단위 테스트 (tests/unit/)
3. Visual Studio 단위 테스트 (tests/ui/)
4. Google Test 통합 테스트 (tests/integration/)
5. AutoIt / UIA E2E 테스트 (앱 실행 후)
6. Visual Studio Profiler (성능 기준선 측정)
7. PVS-Studio 정적 분석 (보안/품질)
```

### 6.3 테스트 데이터 관리

- 모든 fixture 파일은 `tests/fixtures/`에 버전 관리
- 테스트 간 독립성 보장: 각 테스트는 `%TEMP%\deepmetria_test\` 임시 폴더 사용 후 정리
- 민감 데이터(실제 API 키 등) 테스트 사용 금지 — mock 자격증명만 사용
