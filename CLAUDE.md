# 프로젝트 설정

## 목차
1. 프로젝트 설정 .. L18



          


---

<!-- DOCS-OMC-CONFIG-START -->
## 1. 프로젝트 설정

| 항목 | 값 | 비고 |
|------|-----|------|
| 합의 언어 | 한국어 | 모든 문서 작성 언어 |
| 속도 vs 보안 | 보안 우선 | SaaS 플랫폼, 멀티테넌트 |
| 거버넌스 단계 | 표준 | OWASP Top 10, 인증/인가, 라이선스 검토 |
| Git 브랜치 전략 | GitHub Flow | main + feature 브랜치 |
| 커밋 컨벤션 | Conventional Commits | feat/fix/chore/docs 등 |
| 코드 커버리지 목표 | 80% | 일반적인 SaaS 기준 |
| 복잡도 기준 | Cyclomatic ≤ 10 | 함수 단위 복잡도 제한 |
| 기술 스택 | Next.js 14 + FastAPI + PostgreSQL + Redis | 상세: docs/dev/Architecture.md §2 |
| auto-ralph | false | true 시 /dev-team 실행 시 자동 ralph 모드 |

<!-- DOCS-OMC-CONFIG-END -->
