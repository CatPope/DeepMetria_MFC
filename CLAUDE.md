# 프로젝트 설정

## 목차
1. 프로젝트 설정 .. L18
2. 테스트 파일 수정 규칙 .. L30
3. 개발 파이프라인 .. L41



          


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
