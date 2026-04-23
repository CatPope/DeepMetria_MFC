# DeepMetria 보안 점검 보고서
점검일: 2026-04-21 | 점검 범위: 초기 보안 검토

## 목차
1. 점검 환경 및 범위 ........................ L13
2. OWASP Top 10 점검 결과 .............. L31
3. 추가 보안 점검 항목 ................... L227
4. 발견된 취약점 및 문제 사항 .......... L309
5. 권장 조치 사항 .......................... L350

---

## 1. 점검 환경 및 범위

**점검 대상**: DeepMetria 백엔드 (FastAPI, Python 3.x)
**점검 방식**: 정적 코드 분석, 설정 검토
**검토 파일**:
- `app/core/security.py` — JWT, 비밀번호 해싱
- `app/api/auth.py` — 인증 엔드포인트
- `app/api/datasource.py` — 파일 업로드
- `app/core/config.py` — 설정 및 시크릿
- `app/core/dependencies.py` — 인증 미들웨어
- `app/domain/datasource/service.py` — 파일 검증 및 처리
- `app/main.py` — CORS, 미들웨어
- `app/infrastructure/llm/router.py` — LLM API 통합

**거버넌스 단계**: 표준 (Standard)

---

## 2. OWASP Top 10 점검 결과

### A01: Broken Access Control

**상태**: PASS

**검증 내용**:
- `get_current_user()` 미들웨어로 모든 보호 엔드포인트에 인증 강제
- 데이터소스 조회 시 `current_user.id` 검증으로 사용자별 격리 확인
  - `get_datasource()`: `if result.user_id != current_user.id: raise ForbiddenError()`
  - `list_datasources()`: `where(DataSource.user_id == current_user.id)`
  - `delete_datasource()`: 동일 패턴으로 권한 확인
- Bearer 토큰 기반 접근 제어 구현

**권장 사항**: 향후 RBAC(역할 기반) 제어 고려

---

### A02: Cryptographic Failures

**상태**: PASS

**검증 내용**:
- 비밀번호 해싱: bcrypt 사용 (강력한 알고리즘, `passlib.context`)
  - `pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")`
- JWT 서명:
  - 알고리즘: HS256 (대칭 암호화, 적절)
  - 시크릿 키: `settings.APP_SECRET_KEY` (환경 변수 기반)
  - 토큰 타입 검증 (`access` vs `refresh`)
- 토큰 만료 설정:
  - Access Token: 30분 (적절)
  - Refresh Token: 7일 (적절)

**주의사항**: 프로덕션 환경에서 `APP_SECRET_KEY = "change-me"` 기본값이 사용되면 안됨

---

### A03: Injection

**상태**: PASS

**검증 내용**:
- SQL Injection: SQLAlchemy ORM 사용으로 매개변수화 쿼리 기본
  ```python
  select(User).where(User.email == body.email)  # ORM 바인딩
  ```
- 파일 경로 Injection: 파일명을 직접 사용하지 않고, UUID 기반 저장 경로 사용
  ```python
  storage_key = f"users/{user_id}/datasources/{datasource_id}/{filename}"
  ```
- 명령어 Injection: 파일 처리 시 `pandas.read_csv()` 등 라이브러리 함수만 사용, 시스템 명령어 호출 없음
- JSON 파싱: `pd.read_json()` 사용, 직접 `eval()` 없음

**권장 사항**: 파일명 정규식 검증 추가 고려 (특수문자 제거)

---

### A04: Insecure Design

**상태**: PARTIAL

**검증 내용**:
- 파일 업로드 검증:
  - 파일 크기 제한: 50MB (적절)
  - 파일 타입 검증: `.csv`, `.xlsx`, `.xls`, `.json`만 허용
  - MIME 타입 검증 있음: `CONTENT_TYPE_MAP`
- DataFrame 파싱 전 검증 수행 (업로드 전 타당성 확인)

**발견된 문제**:
- MIME 타입 검증이 파일 확장자에만 의존 (MIME 타입 헤더 검증 없음)
- 파일명 정규식 검증 부재 (특수문자 포함 가능)

**위험도**: 낮음 (UUID 기반 저장이므로 경로 탈출 위험 완화)

---

### A05: Security Misconfiguration

**상태**: PARTIAL

**검증 내용**:
- CORS 설정: `CORS_ORIGINS = ["http://localhost:3000"]` (환경 변수로 제어 가능)
- 디버그 모드: `docs_url="/docs" if settings.APP_DEBUG else None` (조건부 활성화)
- 환경 변수: `.env` 파일에서 로드

**발견된 문제**:
1. 기본 시크릿:
   - `APP_SECRET_KEY = "change-me"` (기본값이 약함)
   - `DATABASE_URL = "postgresql+asyncpg://postgres:password@localhost:5432/deepmetria"` (기본 비밀번호)
   - `REDIS_URL = "redis://localhost:6379/0"` (인증 없음)
2. CORS 설정: `allow_methods=["*"]`, `allow_headers=["*"]` (과도하게 개방적)
3. 에러 응답: 500 에러 메시지가 "서버 내부 오류"로 일괄 처리되지만, 스택 트레이스 노출 여부 확인 필요

**위험도**: 중간 (프로덕션 배포 시 심각)

---

### A06: Vulnerable Components

**상태**: PASS (수동 검증 필수)

**검증 내용**:
- 사용 라이브러리:
  - `passlib` + `bcrypt`: 최신, 널리 사용
  - `python-jose`: JWT 표준 구현
  - `fastapi`: 현재 활발히 유지
  - `sqlalchemy`: ORM 보안 우수
  - `pandas`: 데이터 처리, 보안 특화 아님
  - `litellm`: LLM 다중 제공자 통합

**권장 사항**:
- `pip audit` 또는 `poetry check` 등으로 정기적 CVE 스캔
- `requirements.txt` 또는 `pyproject.toml` 버전 핀 권장

---

### A07: Authentication Failures

**상태**: PASS

**검증 내용**:
- 비밀번호 정책: 등록 시 평문 입력 가능하지만, bcrypt 해시로 저장
- 토큰 만료:
  - Access: 30분 (적절)
  - Refresh: 7일 (적절)
- Refresh 토큰 검증: `decode_token()` 시 `token_type="refresh"` 확인
- 삭제된 사용자 확인: `user.deleted_at is not None` 체크로 소프트 삭제 대응

**발견된 문제**:
1. 비밀번호 정책 명시 부재 (최소 길이, 복잡도 요구사항 없음)
2. 로그인 실패 시 속도 제한(rate limiting) 없음
   - 브루트포스 공격 가능성
3. 토큰 블랙리스트 없음 (로그아웃 구현 없음)

**위험도**: 중간

---

### A08: Data Integrity Failures

**상태**: PARTIAL

**검증 내용**:
- 파일 타입 검증: MIME 타입 맵 존재
- 직렬화: Pydantic `model_validate()` 사용으로 타입 강제
- DataFrame 검증: pandas `read_csv()` 등의 내장 검증 사용

**발견된 문제**:
1. 파일 업로드 후 재검증 없음 (R2 저장 후)
2. 파일 무결성 검사(해시) 없음
3. 메타데이터 조작 방지 미흡

**위험도**: 낮음 (클라우드 스토리지 신뢰도 높음)

---

### A09: Logging and Monitoring

**상태**: FAIL

**검증 내용**:
- 로깅 현황: 최소한의 로깅만 존재
  - `LLMRouter.chat_with_tools()`: `logger.warning()` (제한적)
  - 인증 로깅 없음
  - 파일 업로드 로깅 없음
  - 접근 제어 실패 로깅 없음

**발견된 문제**:
1. 감사 로그(audit log) 완전 부재
   - 사용자 로그인/로그아웃
   - 데이터 접근 (조회/수정/삭제)
   - 관리자 작업
2. 에러 로깅 미흡
3. 보안 이벤트 추적 불가능

**권장 사항**: 구조화된 로깅(structured logging) 도입 필수

**위험도**: 높음 (규정 준수, 사고 조사 불가)

---

### A10: SSRF (Server-Side Request Forgery)

**상태**: PASS

**검증 내용**:
- 외부 API 호출:
  - LLM API: litellm 라이브러리 사용, 직접 URL 생성 없음
  - R2 스토리지: boto3 + 하드코딩된 엔드포인트
- 사용자 입력 기반 URL 생성 없음
- 다운로드/업로드 시 경로 검증 (UUID 기반)

**권장 사항**: 향후 사용자 지정 데이터 소스 통합 시 URL 검증 필수

---

## 3. 추가 보안 점검 항목

### 3.1 LLM API 키 보안

**상태**: PASS

**검증**:
- API 키를 `litellm.anthropic_key` 등으로 설정 (환경 변수 → 메모리로 로드)
- 클라이언트에 노출되지 않음 (백엔드에서만 사용)
- `ANTHROPIC_API_KEY`, `OPENAI_API_KEY`, `GEMINI_API_KEY` 모두 환경 변수 의존

**주의**: API 키가 로그에 노출되지 않는지 확인 필수

---

### 3.2 파일 업로드 MIME 타입 검증

**상태**: PARTIAL

**검증**:
- 확장자 기반 검증: `_detect_file_type()`
- CONTENT_TYPE 맵: 존재
- 실제 MIME 타입 헤더 검증: 없음

**권장 사항**: `python-magic` 또는 `filetype` 라이브러리로 파일 매직 번호 검증

---

### 3.3 JWT 토큰 관리

**상태**: PASS

**검증**:
- 토큰 발급: 명확한 타입(`access`, `refresh`) 분리
- 토큰 검증: 타입 및 만료 시간 확인
- Refresh 흐름: 있음

**발견된 문제**:
- 토큰 블랙리스트 없음 (로그아웃 미구현)
- 토큰 갱신 시 이전 refresh 토큰 무효화 없음

---

### 3.4 데이터 격리 및 멀티테넌시

**상태**: PASS

**검증**:
- 모든 데이터 조회에서 `user_id` 필터링
- 쿼리 예시:
  ```python
  select(DataSource).where(DataSource.user_id == current_user.id)
  ```

---

### 3.5 HTTPS 강제화

**상태**: 미검증 (배포 설정 필요)

**권장 사항**:
- 프로덕션 환경에서 HTTPS 필수
- HSTS 헤더 추가: `Strict-Transport-Security: max-age=31536000`
- nginx/Cloudflare 역프록시에서 설정

---

### 3.6 Cloudflare R2 액세스 제어

**상태**: 미검증

**현황**:
- R2 자격증명: 환경 변수 기반
- 파일 경로: `users/{user_id}/datasources/{datasource_id}/{filename}`

**권장 사항**:
- R2 버킷 정책 검토 (공개 접근 금지)
- 서명된 URL(presigned URL) 사용 고려
- ACL 최소 권한 원칙 적용

---

## 4. 발견된 취약점 및 문제 사항

### 심각도: 높음 (High)

1. **A09 로깅 부재**
   - 감시 및 로깅 완전 미흡
   - 보안 이벤트 추적 불가능
   - 규제 준수(PCI-DSS, GDPR 등) 어려움
   - 영향: 사고 대응 불가, 감사 추적 불가

2. **A05 보안 설정 미흡**
   - 기본 시크릿: `APP_SECRET_KEY = "change-me"`
   - CORS: 모든 메서드, 모든 헤더 허용
   - 영향: 프로덕션 배포 시 즉시 위험

3. **A07 브루트포스 방어 없음**
   - 로그인 실패 시 속도 제한 없음
   - 영향: 비밀번호 추측 공격 가능

### 심각도: 중간 (Medium)

1. **A07 비밀번호 정책 미흡**
   - 최소 길이, 복잡도 요구사항 없음
   - 영향: 약한 비밀번호 가능성

2. **A04 MIME 타입 검증 미흡**
   - 확장자 기반만 검증, 실제 파일 타입 미검증
   - 영향: 악의적 파일 업로드 가능성 (낮음, UUID 저장으로 완화)

3. **A08 파일 무결성 검사 없음**
   - 파일 해시 미검증
   - 영향: 저장된 파일 변조 가능성

### 심각도: 낮음 (Low)

1. **A03 파일명 정규식 검증 부재**
   - 특수문자 포함 가능
   - 영향: 미흡 (경로 탈출은 UUID로 방어)

---

## 5. 권장 조치 사항

### 즉시 조치 (Critical) — 배포 전

1. **기본 시크릿 변경**
   ```bash
   # .env 파일에 강력한 값 설정
   APP_SECRET_KEY=<64자 이상의 난수>
   DATABASE_URL=<실제 DB 자격증명>
   REDIS_URL=<인증 포함>
   ```

2. **CORS 제한**
   ```python
   allow_origins=["https://yourdomain.com"],  # 특정 도메인만
   allow_methods=["GET", "POST"],  # 필요한 메서드만
   allow_headers=["Authorization", "Content-Type"],  # 필요한 헤더만
   ```

3. **로깅 구조화**
   ```python
   # 로거 설정 (예: structlog, python-json-logger)
   - 모든 인증 시도 (성공/실패)
   - 모든 데이터 접근 (조회/수정/삭제)
   - 에러 이벤트
   - 보안 이벤트
   ```

4. **비밀번호 정책 추가**
   ```python
   # RegisterRequest 스키마에 추가
   - 최소 8자
   - 대문자 1개, 소문자 1개, 숫자 1개 포함
   ```

### 단기 조치 (Important) — 배포 후 1개월 내

1. **속도 제한(Rate Limiting) 도입**
   ```
   - 로그인 실패: 5회 이상 → 10분 잠금
   - API 엔드포인트: 분당 100회 제한
   ```

2. **파일 검증 강화**
   ```python
   # python-magic 라이브러리 사용
   import magic
   mime = magic.from_buffer(content, mime=True)
   if mime not in ALLOWED_MIMES:
       raise UnsupportedFileTypeError()
   ```

3. **파일 무결성 검사**
   ```python
   import hashlib
   file_hash = hashlib.sha256(content).hexdigest()
   # DataSource 모델에 저장 및 재검증
   ```

4. **토큰 블랙리스트 도입**
   ```
   - 로그아웃 시 refresh 토큰을 Redis에 저장
   - 토큰 검증 시 블랙리스트 확인
   ```

5. **R2 버킷 정책 검토**
   ```
   - 공개 접근 금지
   - 사서명된 URL(presigned URL) 구현
   - 임시 액세스만 허용
   ```

### 장기 조치 (Enhancement) — 배포 후 3개월 내

1. **보안 감사**
   - 전문가 코드 리뷰
   - 침투 테스트(Penetration Testing)

2. **RBAC 도입**
   - 역할 기반 접근 제어 (Admin, Analyst, Viewer 등)

3. **암호화 강화**
   - 민감한 데이터(사용자 이메일 등) 필드 레벨 암호화
   - TDE(Transparent Data Encryption) 검토

4. **모니터링 대시보드**
   - 실시간 보안 이벤트 추적
   - 이상 탐지(Anomaly Detection)

---

## 점검 결과 요약

| 항목 | 상태 | 심각도 |
|------|------|--------|
| A01 Broken Access Control | PASS | — |
| A02 Cryptographic Failures | PASS | — |
| A03 Injection | PASS | — |
| A04 Insecure Design | PARTIAL | Low |
| A05 Security Misconfiguration | PARTIAL | High |
| A06 Vulnerable Components | PASS | — |
| A07 Authentication Failures | PARTIAL | High |
| A08 Data Integrity Failures | PARTIAL | Low |
| A09 Logging and Monitoring | FAIL | High |
| A10 SSRF | PASS | — |
| 추가 항목 | PARTIAL | Medium |

**전체 평가**: 코어 보안(접근 제어, 암호화, 주입)은 우수하지만, 배포 설정, 로깅, 속도 제한이 취약함. **배포 전 반드시 기본 시크릿 변경 및 로깅 구현 필수**.
