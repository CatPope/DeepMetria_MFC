# API 명세서 (API Specification)

DeepMetria 백엔드 API의 전체 엔드포인트, 요청/응답 형식, 인증 규칙을 정의한 문서입니다.

## 목차
1. 개요 (Overview) ..................... L15
2. 인증 (Auth) ......................... L60
3. 데이터소스 (DataSource) .............. L165
4. 분석 (Analysis) ..................... L288
5. 대시보드 (Dashboard) ................. L412
6. 시각화 (Visualization) .............. L569
7. 공통 에러 코드 (Error Codes) ........ L700
---

## 1. 개요 (Overview)

### 기본 정보
- **Base URL**: `/api`
- **Protocol**: HTTP/HTTPS (FastAPI)
- **Response Format**: JSON
- **Version**: 0.1.0

### 인증 방식
모든 엔드포인트는 Bearer Token 기반 JWT 인증을 사용합니다.

**요청 헤더:**
```
Authorization: Bearer {access_token}
```

`/auth/register`, `/auth/login`, `/health`는 인증 불필요합니다.

### 페이지네이션
리스트 응답은 다음 쿼리 파라미터를 지원합니다:
- `page` (int, 기본값: 1): 페이지 번호 (1부터 시작)
- `size` (int, 기본값: 20): 페이지당 항목 수 (최대 100)

### 응답 구조
모든 응답은 다음 형식을 따릅니다:

**성공 응답 (2xx):**
```json
{
  "data": { /* 응답 바디 */ }
}
```

**에러 응답 (4xx, 5xx):**
```json
{
  "error": {
    "code": "ERROR_CODE",
    "message": "에러 설명"
  }
}
```

---

## 2. 인증 (Auth)

### POST /api/auth/register
신규 사용자 등록 및 JWT 토큰 발급

**인증 필요**: 불필요

**요청:**
```json
{
  "email": "user@example.com",
  "password": "MyPassword123",
  "name": "사용자명"
}
```

**요청 검증:**
- `email`: 유효한 이메일 형식
- `password`: 8~128자, 영문 + 숫자 필수
- `name`: 1~100자

**응답 (201 Created):**
```json
{
  "user": {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "email": "user@example.com",
    "name": "사용자명",
    "tier": "free",
    "created_at": "2026-04-21T10:00:00Z"
  },
  "access_token": "eyJhbGc...",
  "refresh_token": "eyJhbGc..."
}
```

---

### POST /api/auth/login
이메일/비밀번호로 로그인

**인증 필요**: 불필요

**요청:**
```json
{
  "email": "user@example.com",
  "password": "MyPassword123"
}
```

**응답 (200 OK):**
```json
{
  "access_token": "eyJhbGc...",
  "refresh_token": "eyJhbGc...",
  "expires_in": 1800
}
```

---

### POST /api/auth/refresh
Refresh Token으로 새 Access Token 발급

**인증 필요**: 불필요

**요청:**
```json
{
  "refresh_token": "eyJhbGc..."
}
```

**응답 (200 OK):**
```json
{
  "access_token": "eyJhbGc...",
  "expires_in": 1800
}
```

---

### GET /api/auth/me
현재 로그인한 사용자 정보 + 이번 달 사용량 조회

**인증 필요**: 필수 (Bearer Token)

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "email": "user@example.com",
  "name": "사용자명",
  "tier": "free",
  "usage": {
    "used": 5,
    "limit": 10
  }
}
```

---

## 3. 데이터소스 (DataSource)

### POST /api/datasources/upload
파일 업로드 및 DataSource 생성 (CSV / Excel / JSON)

**인증 필요**: 필수 (Bearer Token)

**요청:**
- **Content-Type**: `multipart/form-data`
- **file** (file, 필수): CSV/Excel/JSON 파일 (최대 50MB)
- **name** (string, 선택): 데이터소스 이름 (미제공 시 파일명 사용)

**응답 (202 Accepted):**
```json
{
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "status": "processing",
  "stream_url": "/api/datasources/550e8400-e29b-41d4-a716-446655440000/stream"
}
```

---

### GET /api/datasources
사용자 데이터소스 목록 조회

**인증 필요**: 필수 (Bearer Token)

**쿼리 파라미터:**
- `page` (int, 기본값: 1): 페이지 번호
- `size` (int, 기본값: 20): 페이지당 항목 수

**응답 (200 OK):**
```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "sales_data.csv",
      "file_type": "csv",
      "file_size": 102400,
      "row_count": 1000,
      "column_count": 5,
      "status": "completed",
      "error_message": null,
      "created_at": "2026-04-21T10:00:00Z"
    }
  ],
  "total": 15,
  "page": 1
}
```

---

### GET /api/datasources/{datasource_id}
데이터소스 상세 조회 (스키마 + 요약 포함)

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `datasource_id` (UUID): 데이터소스 ID

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "sales_data.csv",
  "status": "completed",
  "file_type": "csv",
  "file_size": 102400,
  "row_count": 1000,
  "column_count": 5,
  "schema": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440001",
      "column_name": "date",
      "column_index": 0,
      "dtype": "datetime",
      "nullable": false,
      "sample_values": ["2026-01-01", "2026-01-02"]
    },
    {
      "id": "550e8400-e29b-41d4-a716-446655440002",
      "column_name": "amount",
      "column_index": 1,
      "dtype": "float",
      "nullable": false,
      "sample_values": [100.5, 250.3]
    }
  ],
  "summary": {
    "id": "550e8400-e29b-41d4-a716-446655440003",
    "domain": "판매",
    "description": "월별 판매 데이터",
    "statistics": {
      "amount": {
        "mean": 500.0,
        "std": 150.0,
        "min": 10.0,
        "max": 2000.0
      }
    },
    "row_count": 1000
  }
}
```

---

### DELETE /api/datasources/{datasource_id}
데이터소스 삭제 (연관 분석 결과, 시각화 포함)

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `datasource_id` (UUID): 데이터소스 ID

**응답 (204 No Content):**
응답 바디 없음

---

## 4. 분석 (Analysis)

### POST /api/analysis/query
분석 시작 및 flow_id 반환

**인증 필요**: 필수 (Bearer Token)

**요청:**
```json
{
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "dashboard_id": "550e8400-e29b-41d4-a716-446655440010",
  "question": "지난 3개월간 월별 판매 추이는?"
}
```

**요청 검증:**
- `datasource_id` (UUID, 필수): 분석 대상 데이터소스
- `dashboard_id` (UUID, 선택): 분석 결과를 추가할 대시보드
- `question` (string, 필수): 1~2000자

**응답 (201 Created):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440100",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "dashboard_id": "550e8400-e29b-41d4-a716-446655440010",
  "question": "지난 3개월간 월별 판매 추이는?",
  "status": "pending",
  "llm_provider": null,
  "llm_model": null,
  "cot_steps": [],
  "tool_calls": [],
  "error_message": null,
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:00:00Z"
}
```

---

### GET /api/analysis/{flow_id}/stream
SSE 스트림 — CoT 진행 과정 실시간 전달

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `flow_id` (UUID): 분석 플로우 ID

**응답 (200 OK):**
Server-Sent Events (SSE) 스트림

**이벤트 타입:**

1. **cot_step**: 사고 과정 단계
```json
{
  "event": "cot_step",
  "step": 1,
  "description": "데이터를 로드하고 기본 통계 계산"
}
```

2. **tool_call**: 도구 호출 (pandas, LLM 등)
```json
{
  "event": "tool_call",
  "tool": "pandas",
  "function": "groupby",
  "params": {"by": "month"}
}
```

3. **error**: 에러 발생
```json
{
  "event": "error",
  "message": "분석 중 오류가 발생했습니다."
}
```

---

### GET /api/analysis/{flow_id}
분석 결과 조회 (최종 상태)

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `flow_id` (UUID): 분석 플로우 ID

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440100",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "dashboard_id": "550e8400-e29b-41d4-a716-446655440010",
  "question": "지난 3개월간 월별 판매 추이는?",
  "status": "completed",
  "llm_provider": "openai",
  "llm_model": "gpt-4",
  "cot_steps": [
    {
      "step": 1,
      "description": "데이터를 로드하고 기본 통계 계산"
    }
  ],
  "tool_calls": [
    {
      "tool": "pandas",
      "function": "groupby",
      "params": {"by": "month"}
    }
  ],
  "error_message": null,
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:00:30Z"
}
```

---

## 5. 대시보드 (Dashboard)

### GET /api/dashboards
사용자 대시보드 목록 조회

**인증 필요**: 필수 (Bearer Token)

**쿼리 파라미터:**
- `page` (int, 기본값: 1): 페이지 번호
- `size` (int, 기본값: 20): 페이지당 항목 수

**응답 (200 OK):**
```json
{
  "items": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440010",
      "user_id": "550e8400-e29b-41d4-a716-446655440000",
      "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
      "name": "2026년 판매 현황",
      "layout": [],
      "created_at": "2026-04-21T10:00:00Z",
      "updated_at": "2026-04-21T10:00:00Z"
    }
  ],
  "total": 5,
  "page": 1
}
```

---

### POST /api/dashboards
새 대시보드 생성

**인증 필요**: 필수 (Bearer Token)

**요청:**
```json
{
  "name": "2026년 판매 현황",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "layout": []
}
```

**요청 검증:**
- `name` (string, 필수): 1~255자
- `datasource_id` (UUID, 필수): 연결할 데이터소스
- `layout` (array, 선택): 레이아웃 배열 (기본값: [])

**응답 (201 Created):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440010",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "2026년 판매 현황",
  "layout": [],
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:00:00Z"
}
```

---

### GET /api/dashboards/{dashboard_id}
대시보드 상세 조회 (시각화 포함)

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `dashboard_id` (UUID): 대시보드 ID

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440010",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "2026년 판매 현황",
  "layout": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440020",
      "x": 0,
      "y": 0,
      "width": 6,
      "height": 4
    }
  ],
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:00:00Z"
}
```

---

### PATCH /api/dashboards/{dashboard_id}
대시보드 이름 또는 레이아웃 업데이트

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `dashboard_id` (UUID): 대시보드 ID

**요청:**
```json
{
  "name": "2026년 판매 현황 (수정됨)",
  "layout": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440020",
      "x": 0,
      "y": 0,
      "width": 8,
      "height": 6
    }
  ]
}
```

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440010",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "datasource_id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "2026년 판매 현황 (수정됨)",
  "layout": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440020",
      "x": 0,
      "y": 0,
      "width": 8,
      "height": 6
    }
  ],
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:05:00Z"
}
```

---

### DELETE /api/dashboards/{dashboard_id}
대시보드 소프트 삭제

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `dashboard_id` (UUID): 대시보드 ID

**응답 (204 No Content):**
응답 바디 없음

---

## 6. 시각화 (Visualization)

### GET /api/visualizations/{viz_id}
시각화 상세 조회

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `viz_id` (UUID): 시각화 ID

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440020",
  "dashboard_id": "550e8400-e29b-41d4-a716-446655440010",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "viz_type": "line_chart",
  "title": "월별 판매액 추이",
  "chart_config": {
    "x_axis": "month",
    "y_axis": "amount",
    "series": ["2026"]
  },
  "style": {
    "color_scheme": "blue",
    "font_size": 12
  },
  "position": {
    "x": 0,
    "y": 0,
    "width": 6,
    "height": 4
  },
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:00:00Z"
}
```

---

### PATCH /api/visualizations/{viz_id}
시각화 스타일, 위치, 설정 수정

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `viz_id` (UUID): 시각화 ID

**요청:**
```json
{
  "title": "월별 판매액 추이 (2026년)",
  "chart_config": {
    "x_axis": "month",
    "y_axis": "amount",
    "series": ["2026", "2025"]
  },
  "style": {
    "color_scheme": "green",
    "font_size": 14
  },
  "position": {
    "x": 0,
    "y": 0,
    "width": 8,
    "height": 5
  }
}
```

**응답 (200 OK):**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440020",
  "dashboard_id": "550e8400-e29b-41d4-a716-446655440010",
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "viz_type": "line_chart",
  "title": "월별 판매액 추이 (2026년)",
  "chart_config": {
    "x_axis": "month",
    "y_axis": "amount",
    "series": ["2026", "2025"]
  },
  "style": {
    "color_scheme": "green",
    "font_size": 14
  },
  "position": {
    "x": 0,
    "y": 0,
    "width": 8,
    "height": 5
  },
  "created_at": "2026-04-21T10:00:00Z",
  "updated_at": "2026-04-21T10:05:00Z"
}
```

---

### DELETE /api/visualizations/{viz_id}
시각화 삭제

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `viz_id` (UUID): 시각화 ID

**응답 (204 No Content):**
응답 바디 없음

---

### GET /api/visualizations/{viz_id}/export
시각화 PNG 내보내기

**인증 필요**: 필수 (Bearer Token)

**경로 파라미터:**
- `viz_id` (UUID): 시각화 ID

**쿼리 파라미터:**
- `format` (string, 기본값: "png"): PNG만 지원

**응답 (200 OK):**
- **Content-Type**: `image/png`
- **바디**: PNG 바이너리 데이터
- **헤더**: `Content-Disposition: attachment; filename="visualization_{viz_id}.png"`

---

## 7. 공통 에러 코드 (Error Codes)

### 인증 관련 에러

| HTTP | Code | Message |
|------|------|---------|
| 401 | INVALID_CREDENTIALS | 이메일 또는 비밀번호가 올바르지 않습니다. |
| 401 | INVALID_TOKEN | 유효하지 않거나 만료된 토큰입니다. |
| 409 | EMAIL_ALREADY_EXISTS | 이미 사용 중인 이메일입니다. |

### 리소스 관련 에러

| HTTP | Code | Message |
|------|------|---------|
| 404 | NOT_FOUND | 요청한 리소스를 찾을 수 없습니다. |
| 403 | FORBIDDEN | 접근 권한이 없습니다. |

### 사용량 관련 에러

| HTTP | Code | Message |
|------|------|---------|
| 429 | QUOTA_EXCEEDED | 이번 달 사용량 한도를 초과했습니다. |

### 입력값 검증 에러

| HTTP | Code | Message |
|------|------|---------|
| 400 | VALIDATION_ERROR | 입력값이 올바르지 않습니다. |

### 서버 에러

| HTTP | Code | Message |
|------|------|---------|
| 500 | INTERNAL_ERROR | 서버 내부 오류가 발생했습니다. |

---

## 에러 응답 예제

```json
{
  "error": {
    "code": "INVALID_CREDENTIALS",
    "message": "이메일 또는 비밀번호가 올바르지 않습니다."
  }
}
```
