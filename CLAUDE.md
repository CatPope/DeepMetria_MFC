# 프로젝트 설정

## 목차
1. 프로젝트 설정 .. L18



          


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
