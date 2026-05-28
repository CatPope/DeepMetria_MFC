# 프로젝트 설정

## 목차
1. 프로젝트 설정 .. L18
2. 테스트 파일 수정 규칙 .. L30
3. 개발 파이프라인 .. L41
4. 테스트 스크린샷 분석 규칙 (필수) .. L64



          


---

<!-- DOCS-OMC-CONFIG-START -->
## 1. 프로젝트 설정

| 항목 | 값 | 비고 |
|------|-----|------|
| 합의 언어 | 한국어 | 모든 문서 작성 언어 |
| 속도 vs 보안 | 보안 우선 | 데스크톱 앱, 로컬 데이터 보호 |
| 거버넌스 단계 | 표준 | 데스크톱 보안, 인증/라이선스, 라이선스 검토 |
| Git 브랜치 전략 | GitHub Flow | main + feature 브랜치 |
| 커밋 컨벤션 | Conventional Commits | feat/fix/chore/docs 등 |
| 코드 커버리지 목표 | 80% | Google Test 기준 |
| 복잡도 기준 | Cyclomatic ≤ 10 | 함수 단위 복잡도 제한 |
| 기술 스택 | MFC (C++) + SQLite + libcurl + GDI+ | 상세: docs/dev/Architecture.md §2 |
| auto-ralph | false | true 시 /dev-team 실행 시 자동 ralph 모드 |

<!-- DOCS-OMC-CONFIG-END -->

## 2. 테스트 파일 수정 규칙

| 항목 | 규칙 |
|------|------|
| 대상 경로 | `tests/**/*` |
| 수정 시 | 반드시 사용자 허가를 받은 후 수정 |
| 신규 생성 | 사용자 허가 필요 |
| 삭제 | 사용자 허가 필요 |

## 3. 개발 파이프라인

코드 작업 시 반드시 다음 루프를 따른다:

```
1. 코드 작성/수정
2. 빌드 (build.bat 또는 rebuild.bat)
3. UI 테스트 (python pipeline.py --test-only)
4. 결과 분석 (pipeline_result.json 확인)
   → FAIL: 1번으로 돌아가 수정
   → PASS: 루프 종료, 사용자에게 결과 보고
```

| 명령 | 용도 |
|------|------|
| `python pipeline.py` | 전체 (빌드+테스트+분석) |
| `python pipeline.py --build-only` | 빌드만 |
| `python pipeline.py --test-only` | 테스트+분석만 |
| `python pipeline.py --config Release` | Release 빌드 |

빌드 실행 방법: `powershell.exe -Command "& cmd /c '<프로젝트루트>\build.bat'"` (Bash 셸에서)

## 4. 테스트 스크린샷 분석 규칙 (필수)

파이프라인 실행 후 **반드시** 스크린샷을 Read 도구로 열어서 이미지를 직접 분석해야 한다. 텍스트 로그만 보고 PASS/FAIL을 판단하지 말 것.

| 항목 | 규칙 |
|------|------|
| 스크린샷 경로 | `tests/screenshots/run_YYYYMMDD_HHMMSS/` (실행별 폴더) |
| **필수 분석 시점** | 파이프라인 실행 후 **매번**, FAIL 원인 분석 시, 테스트 작성/디버깅 시 |
| 분석 방법 | `Read` 도구로 스크린샷 이미지 파일을 직접 열어 시각적으로 확인 |
| 확인 항목 | UI 레이아웃, 컨트롤 표시 상태, 다이얼로그 잔류 여부, 데이터 표시 정확성 |
| **금지 사항** | 스크린샷을 확인하지 않고 테스트 결과를 보고하는 것 |

### 분석 절차

```
1. 파이프라인 실행 완료
2. tests/screenshots/ 에서 최신 run_ 폴더 확인
3. 주요 테스트의 스크린샷을 Read 도구로 열어 이미지 분석
   - FAIL 항목: 해당 스크린샷 전수 확인
   - PASS 항목: 대표 스크린샷 샘플 확인 (최소 3장)
   - SKIP 항목: 원인 확인용 스크린샷 확인
4. 이미지에서 발견된 문제를 텍스트 로그와 대조
5. 분석 결과를 사용자에게 보고
```
