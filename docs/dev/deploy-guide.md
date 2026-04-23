# DeepMetria 배포 가이드

## 목차
1. 사전 요구사항 ................................. L14
2. 로컬 개발 환경 구축 ............................ L54
3. Docker 배포 ................................... L129
4. 프로덕션 배포 (Cloudflare/Vercel) ........... L251
5. DB 마이그레이션 ............................... L396
6. 모니터링 및 로깅 .............................. L452
7. 롤백 절차 ..................................... L506

---

## 1. 사전 요구사항

### 필수 설치 항목

#### 백엔드 개발 환경
- Python 3.11 이상
- pip 또는 uv (패키지 관리자)
- PostgreSQL 15
- Redis 7
- Alembic (DB 마이그레이션 도구)

#### 프론트엔드 개발 환경
- Node.js 20 LTS
- npm 또는 yarn

#### 배포 환경
- Docker 및 Docker Compose 20.10+
- Cloudflare 계정 (R2 스토리지 및 Pages)
- Vercel 계정 (프론트엔드 호스팅)
- AWS CLI 또는 boto3 (Cloudflare R2 접근)

### 환경 변수 설정

백엔드 `.env.local`:
```
DATABASE_URL=postgresql+asyncpg://postgres:password@localhost:5432/deepmetria
REDIS_URL=redis://localhost:6379/0
SECRET_KEY=your-secret-key-here
R2_ACCOUNT_ID=your-account-id
R2_ACCESS_KEY_ID=your-access-key
R2_SECRET_ACCESS_KEY=your-secret-key
R2_BUCKET_NAME=deepmetria
```

프론트엔드 `.env.local`:
```
NEXT_PUBLIC_API_URL=http://localhost:8000
NEXT_PUBLIC_APP_ENV=development
```

## 2. 로컬 개발 환경 구축

### 2.1 저장소 복제

```bash
git clone https://github.com/your-org/DeepMetria.git
cd DeepMetria
```

### 2.2 백엔드 설정

```bash
cd backend
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -e ".[dev]"
```

환경 변수 파일 생성:
```bash
cp .env.example .env.local
# .env.local 편집하여 실제 값 입력
```

### 2.3 프론트엔드 설정

```bash
cd frontend
node --version  # 20.x 확인
npm ci
```

환경 변수 파일 생성:
```bash
cp .env.example .env.local
# .env.local 편집하여 실제 값 입력
```

### 2.4 Docker로 데이터베이스 실행

```bash
# 프로젝트 루트에서
docker-compose up -d postgres redis
```

상태 확인:
```bash
docker-compose ps
```

### 2.5 DB 마이그레이션 (로컬)

```bash
cd backend
alembic upgrade head
```

### 2.6 개발 서버 시작

터미널 1 - 백엔드:
```bash
cd backend
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

터미널 2 - 프론트엔드:
```bash
cd frontend
npm run dev
```

확인:
- 백엔드: http://localhost:8000/docs
- 프론트엔드: http://localhost:3000

## 3. Docker 배포

### 3.1 Docker 이미지 빌드

백엔드 Dockerfile 생성 (backend/Dockerfile):
```dockerfile
FROM python:3.11-slim

WORKDIR /app

COPY pyproject.toml .
RUN pip install --no-cache-dir -e .

COPY . .

CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]
```

프론트엔드 Dockerfile 생성 (frontend/Dockerfile):
```dockerfile
FROM node:20-alpine AS builder

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY . .
RUN npm run build

FROM node:20-alpine

WORKDIR /app

COPY --from=builder /app/.next .next
COPY --from=builder /app/node_modules node_modules
COPY --from=builder /app/package.json .

EXPOSE 3000

CMD ["npm", "start"]
```

### 3.2 docker-compose.yml 확장

프로덕션용 docker-compose (docker-compose.prod.yml):
```yaml
version: "3.9"

services:
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: ${DB_NAME:-deepmetria}
      POSTGRES_USER: ${DB_USER:-postgres}
      POSTGRES_PASSWORD: ${DB_PASSWORD}
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
    restart: always

  redis:
    image: redis:7
    ports:
      - "6379:6379"
    restart: always

  backend:
    build:
      context: ./backend
      dockerfile: Dockerfile
    environment:
      DATABASE_URL: postgresql+asyncpg://${DB_USER}:${DB_PASSWORD}@postgres:5432/${DB_NAME}
      REDIS_URL: redis://redis:6379/0
      SECRET_KEY: ${SECRET_KEY}
      R2_ACCOUNT_ID: ${R2_ACCOUNT_ID}
      R2_ACCESS_KEY_ID: ${R2_ACCESS_KEY_ID}
      R2_SECRET_ACCESS_KEY: ${R2_SECRET_ACCESS_KEY}
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    ports:
      - "8000:8000"
    restart: always

  frontend:
    build:
      context: ./frontend
      dockerfile: Dockerfile
    environment:
      NEXT_PUBLIC_API_URL: ${NEXT_PUBLIC_API_URL}
    ports:
      - "3000:3000"
    depends_on:
      - backend
    restart: always

volumes:
  postgres_data:
```

### 3.3 Docker로 배포

로컬에서 테스트:
```bash
docker-compose -f docker-compose.prod.yml up -d
```

컨테이너 로그 확인:
```bash
docker-compose -f docker-compose.prod.yml logs -f backend
docker-compose -f docker-compose.prod.yml logs -f frontend
```

정지:
```bash
docker-compose -f docker-compose.prod.yml down
```

## 4. 프로덕션 배포 (Cloudflare/Vercel)

### 4.1 백엔드 배포 (Cloudflare Workers 또는 VPS)

#### Cloudflare Workers 선택사항

Workers에 FastAPI 배포는 제약이 있으므로, Cloudflare의 VPS 또는 다른 호스팅 제공자 권장.

#### VPS (Hetzner, Linode, DigitalOcean) 배포 단계

1. VPS에 SSH 접속:
```bash
ssh root@your-server-ip
```

2. 필수 패키지 설치:
```bash
apt update && apt upgrade -y
apt install -y python3.11 python3-pip docker.io docker-compose git curl
```

3. GitHub에서 저장소 복제:
```bash
cd /opt
git clone https://github.com/your-org/DeepMetria.git
cd DeepMetria
```

4. 환경 변수 설정:
```bash
cat > .env.prod <<EOF
DB_NAME=deepmetria
DB_USER=deepmetria_user
DB_PASSWORD=$(openssl rand -base64 32)
SECRET_KEY=$(openssl rand -base64 32)
R2_ACCOUNT_ID=your-account-id
R2_ACCESS_KEY_ID=your-key
R2_SECRET_ACCESS_KEY=your-secret
NEXT_PUBLIC_API_URL=https://api.yourdomain.com
EOF
```

5. Docker로 시작:
```bash
docker-compose -f docker-compose.prod.yml up -d
```

6. Nginx 리버스 프록시 설정 (선택사항):
```nginx
upstream backend {
    server localhost:8000;
}

upstream frontend {
    server localhost:3000;
}

server {
    listen 80;
    server_name api.yourdomain.com;

    location / {
        proxy_pass http://backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}

server {
    listen 80;
    server_name yourdomain.com;

    location / {
        proxy_pass http://frontend;
        proxy_set_header Host $host;
    }
}
```

7. SSL 인증서 설정 (Let's Encrypt):
```bash
apt install -y certbot python3-certbot-nginx
certbot --nginx -d api.yourdomain.com -d yourdomain.com
```

### 4.2 프론트엔드 배포 (Vercel)

#### Vercel 계정 연결

1. Vercel CLI 설치:
```bash
npm install -g vercel
```

2. Vercel 로그인:
```bash
vercel login
```

3. 프론트엔드 배포:
```bash
cd frontend
vercel --prod
```

#### 환경 변수 설정 (Vercel 대시보드)

프로젝트 Settings → Environment Variables에서:
```
NEXT_PUBLIC_API_URL = https://api.yourdomain.com
```

#### 자동 배포 설정

GitHub와 연결하면 main 브랜치 푸시 시 자동 배포.

### 4.3 Cloudflare R2 스토리지 설정

1. Cloudflare 대시보드에서 R2 생성
2. API 토큰 생성: R2 API 토큰 (Account API Token)
3. 환경 변수 설정:
```bash
export R2_ACCOUNT_ID=your-account-id
export R2_ACCESS_KEY_ID=your-access-key
export R2_SECRET_ACCESS_KEY=your-secret-key
export R2_BUCKET_NAME=deepmetria
```

4. boto3로 접근 테스트:
```python
import boto3

s3 = boto3.client(
    's3',
    endpoint_url='https://{account_id}.r2.cloudflarestorage.com',
    aws_access_key_id='your-key',
    aws_secret_access_key='your-secret'
)

response = s3.list_buckets()
print(response)
```

## 5. DB 마이그레이션

### 5.1 Alembic 마이그레이션 기본 흐름

현재 상태 확인:
```bash
cd backend
alembic current
```

최신 버전으로 업그레이드:
```bash
alembic upgrade head
```

특정 버전으로 다운그레이드:
```bash
alembic downgrade -1  # 한 버전 이전으로
alembic downgrade base  # 초기 상태로
```

### 5.2 마이그레이션 히스토리 확인

```bash
alembic history
```

### 5.3 새 마이그레이션 생성

모델 변경 후:
```bash
alembic revision --autogenerate -m "설명: 새로운 테이블 추가"
alembic upgrade head
```

생성된 파일 확인 및 검증:
```bash
cat alembic/versions/xxxxx_description.py
```

### 5.4 프로덕션 배포 중 마이그레이션

1. 배포 전 로컬에서 테스트:
```bash
DATABASE_URL=postgresql+asyncpg://user:pass@localhost:5432/test_db alembic upgrade head
```

2. 무중단 마이그레이션:
```bash
# 기존 서비스 운영 중
docker-compose -f docker-compose.prod.yml exec backend alembic upgrade head

# 새 버전 배포
docker-compose -f docker-compose.prod.yml up -d --build backend
```

## 6. 모니터링 및 로깅

### 6.1 로그 확인

Docker 컨테이너 로그:
```bash
# 백엔드 로그
docker-compose logs -f backend

# 프론트엔드 로그
docker-compose logs -f frontend

# 데이터베이스 로그
docker-compose logs -f postgres
```

### 6.2 헬스 체크

백엔드 헬스 체크 엔드포인트:
```bash
curl http://localhost:8000/health
```

프론트엔드 헬스 체크:
```bash
curl http://localhost:3000
```

### 6.3 프로메테우스 메트릭 (선택사항)

백엔드에 prometheus-client 추가:
```bash
pip install prometheus-client
```

main.py에 메트릭 추가:
```python
from prometheus_client import Counter, Histogram
from fastapi import FastAPI
from prometheus_fastapi_instrumentator import Instrumentator

app = FastAPI()

Instrumentator().instrument(app).expose(app)
```

### 6.4 기본 모니터링 체크리스트

- 데이터베이스 연결 상태
- Redis 연결 상태
- CPU/메모리 사용률 (docker stats)
- API 응답 시간
- 에러 로그 (4xx, 5xx)

## 7. 롤백 절차

### 7.1 애플리케이션 롤백

#### Git을 통한 롤백

이전 커밋으로 되돌리기:
```bash
git log --oneline | head -20  # 커밋 히스토리 확인
git revert <commit-hash>       # 특정 커밋 되돌리기
git push origin main
```

또는 강제 롤백 (신중하게 사용):
```bash
git reset --hard <commit-hash>
git push origin main --force
```

#### Docker 이미지 롤백

이전 이미지 태그 사용:
```bash
# 빌드 시 태그 지정
docker build -t deepmetria-backend:v1.0.0 ./backend
docker build -t deepmetria-frontend:v1.0.0 ./frontend

# 롤백 시 이전 태그로 실행
docker-compose -f docker-compose.prod.yml down
sed -i 's/latest/v1.0.0/g' docker-compose.prod.yml
docker-compose -f docker-compose.prod.yml up -d
```

### 7.2 데이터베이스 롤백

마이그레이션 이전 버전으로 되돌리기:
```bash
cd backend

# 현재 상태 확인
alembic current

# 이전 버전으로 다운그레이드
alembic downgrade -1

# 또는 특정 버전으로
alembic downgrade <revision>
```

백업에서 복구:
```bash
# PostgreSQL 백업 복구
pg_restore -U postgres -d deepmetria /path/to/backup.dump
```

### 7.3 프론트엔드 롤백 (Vercel)

Vercel 대시보드:
1. Deployments 탭에서 이전 배포 선택
2. "Promote to Production" 클릭

CLI를 통한 롤백:
```bash
vercel rollback
```

### 7.4 롤백 체크리스트

- [ ] 변경 사항 백업 (데이터베이스, 코드, 설정)
- [ ] 롤백 대상 버전 확인
- [ ] 로컬에서 테스트
- [ ] 프로덕션 배포 직전 최종 확인
- [ ] 롤백 후 헬스 체크 수행
- [ ] 데이터 무결성 검증
- [ ] 팀에 공지
