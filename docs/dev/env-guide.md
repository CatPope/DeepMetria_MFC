# 환경 설정 가이드

**DeepMetria** 프로젝트의 로컬 개발, Docker, 프로덕션 환경 설정을 위한 완전한 가이드.

## 목차
1. 필수 환경 변수 .......................... L15
2. 백엔드 설정 ............................ L44
3. 프론트엔드 설정 ........................ L85
4. CLI 설정 ............................... L126
5. Docker 환경 ............................ L149
6. .env 파일 예시 ......................... L180

---

## 1. 필수 환경 변수

다음 표는 모든 컴포넌트에서 사용되는 필수 및 선택 환경 변수를 정리한 것입니다.

| 변수명 | 설명 | 필수 | 기본값 | 예시 |
|--------|------|------|--------|------|
| **APP_ENV** | 실행 환경 (development, staging, production) | 아니오 | `development` | `production` |
| **APP_DEBUG** | 디버그 모드 활성화 | 아니오 | `false` | `true` |
| **APP_SECRET_KEY** | JWT 서명용 비밀키 (반드시 변경) | 예 | `change-me` | `your-secret-key-here` |
| **JWT_ALGORITHM** | JWT 서명 알고리즘 | 아니오 | `HS256` | `HS256` |
| **JWT_ACCESS_TOKEN_EXPIRE_MINUTES** | 액세스 토큰 만료 시간 (분) | 아니오 | `30` | `60` |
| **JWT_REFRESH_TOKEN_EXPIRE_DAYS** | 리프레시 토큰 만료 시간 (일) | 아니오 | `7` | `14` |
| **DATABASE_URL** | PostgreSQL 연결 문자열 | 예 | `postgresql+asyncpg://postgres:password@localhost:5432/deepmetria` | `postgresql+asyncpg://user:pass@db.example.com:5432/prod_db` |
| **REDIS_URL** | Redis 연결 문자열 | 예 | `redis://localhost:6379/0` | `redis://cache.example.com:6379/0` |
| **R2_ACCOUNT_ID** | Cloudflare R2 계정 ID | 아니오 | (없음) | `abc123def456` |
| **R2_ACCESS_KEY_ID** | Cloudflare R2 액세스 키 ID | 아니오 | (없음) | `AKIAIOSFODNN7EXAMPLE` |
| **R2_SECRET_ACCESS_KEY** | Cloudflare R2 비밀 액세스 키 | 아니오 | (없음) | `wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY` |
| **R2_BUCKET_NAME** | Cloudflare R2 버킷 이름 | 아니오 | `deepmetria-files` | `deepmetria-files` |
| **R2_PUBLIC_URL** | Cloudflare R2 공개 URL | 아니오 | (없음) | `https://cdn.example.com` |
| **ANTHROPIC_API_KEY** | Anthropic API 키 | 아니오 | (없음) | `sk-ant-...` |
| **OPENAI_API_KEY** | OpenAI API 키 | 아니오 | (없음) | `sk-...` |
| **GEMINI_API_KEY** | Google Gemini API 키 | 아니오 | (없음) | `AIza...` |
| **DEFAULT_LLM_PROVIDER** | 기본 LLM 제공자 | 아니오 | `anthropic` | `openai` |
| **DEFAULT_LLM_MODEL** | 기본 LLM 모델 | 아니오 | `claude-3-5-sonnet-20241022` | `gpt-4-turbo` |
| **CORS_ORIGINS** | CORS 허용 원본 (JSON 배열 또는 쉼표 구분) | 아니오 | `http://localhost:3000` | `["http://localhost:3000", "https://app.example.com"]` |
| **NEXT_PUBLIC_API_URL** | 프론트엔드에서 사용할 API 기본 URL | 아니오 | `http://localhost:8000/api/v1` | `https://api.example.com/api/v1` |

---

## 2. 백엔드 설정

### 2.1 로컬 개발 환경

백엔드는 **Python 3.11+** 및 **FastAPI**를 사용합니다.

**설치:**
```bash
cd backend
pip install -e ".[dev]"
```

**환경 변수 설정:**
```bash
cp .env.example .env
# .env 파일을 편집하여 로컬 데이터베이스 및 Redis 정보 입력
```

**데이터베이스 마이그레이션:**
```bash
alembic upgrade head
```

**개발 서버 실행:**
```bash
uvicorn app.main:app --reload --port 8000
```

### 2.2 주요 설정 파일

- **`app/core/config.py`** — Pydantic Settings로 환경 변수 로드
- **`alembic.ini`** — 데이터베이스 마이그레이션 설정 (DATABASE_URL로 오버라이드)

### 2.3 중요 사항

- `APP_SECRET_KEY`는 프로덕션에서 반드시 강력한 문자열로 변경
- PostgreSQL 연결 문자열에는 `asyncpg` 드라이버 필수
- Redis는 세션/캐시 관리에 사용

---

## 3. 프론트엔드 설정

### 3.1 로컬 개발 환경

프론트엔드는 **Next.js 16** 및 **React 19**를 사용합니다.

**설치:**
```bash
cd frontend
npm install
```

**환경 변수 설정:**
```bash
cat > .env.local << EOF
NEXT_PUBLIC_API_URL=http://localhost:8000/api/v1
EOF
```

**개발 서버 실행:**
```bash
npm run dev
```
브라우저: `http://localhost:3000`

### 3.2 API 클라이언트 설정

- **`src/lib/api/client.ts`** — JWT 자동 첨부, 401 처리, 에러 핸들링
- **`NEXT_PUBLIC_API_URL`** 환경 변수로 API 기본 URL 설정
- 인증 미인증 시 `/login` 페이지로 자동 리다이렉트 (`src/proxy.ts`)

### 3.3 주요 라이브러리

- **프레임워크:** Next.js 16, React 19
- **상태 관리:** Zustand
- **UI 컴포넌트:** shadcn/ui, Base UI
- **차트:** Recharts
- **테스트:** Vitest, Playwright

---

## 4. CLI 설정

### 4.1 설치

```bash
cd deepmetria-cli
pip install -e ".[dev]"
```

### 4.2 실행

```bash
deepmetria --help
```

### 4.3 주요 의존성

- **Click** — CLI 프레임워크
- **httpx** — HTTP 클라이언트
- **Rich** — 터미널 출력 포매팅

---

## 5. Docker 환경

### 5.1 서비스 구성

`docker-compose.yml`에 정의된 서비스:

| 서비스 | 이미지 | 포트 | 설명 |
|--------|--------|------|------|
| **postgres** | postgres:15 | 5432 | PostgreSQL 데이터베이스 |
| **redis** | redis:7 | 6379 | Redis 캐시/세션 |

### 5.2 시작

```bash
docker-compose up -d
```

### 5.3 확인

```bash
docker-compose ps
```

### 5.4 종료

```bash
docker-compose down
```

---

## 6. .env 파일 예시

로컬 개발 환경용 `.env` 파일 템플릿:

```env
# ── 앱 기본 ──────────────────────────────────────────────
APP_ENV=development
APP_DEBUG=true
APP_SECRET_KEY=dev-secret-key-change-in-production

# ── JWT ──────────────────────────────────────────────────
JWT_ALGORITHM=HS256
JWT_ACCESS_TOKEN_EXPIRE_MINUTES=30
JWT_REFRESH_TOKEN_EXPIRE_DAYS=7

# ── PostgreSQL ────────────────────────────────────────────
DATABASE_URL=postgresql+asyncpg://postgres:postgres@localhost:5432/deepmetria

# ── Redis ─────────────────────────────────────────────────
REDIS_URL=redis://localhost:6379/0

# ── Cloudflare R2 (선택) ────────────────────────────────
# R2_ACCOUNT_ID=
# R2_ACCESS_KEY_ID=
# R2_SECRET_ACCESS_KEY=
R2_BUCKET_NAME=deepmetria-files
# R2_PUBLIC_URL=

# ── LLM (선택: 최소 1개 제공자 필수) ───────────────────
ANTHROPIC_API_KEY=
OPENAI_API_KEY=
GEMINI_API_KEY=
DEFAULT_LLM_PROVIDER=anthropic
DEFAULT_LLM_MODEL=claude-3-5-sonnet-20241022

# ── CORS ──────────────────────────────────────────────────
CORS_ORIGINS=http://localhost:3000

# ── 프론트엔드 (frontend/.env.local) ────────────────────
NEXT_PUBLIC_API_URL=http://localhost:8000/api/v1
```

