# DeepMetria MFC 수동 테스트 환경

## 설치

```bash
pip install -r requirements.txt
```

## 실행

```bash
python test_runner.py
```

스크린샷은 `tests/screenshots/` 폴더에 저장됩니다.

## 테스트 데이터 파일 (`tests/data/`)

| 파일 | 설명 |
|------|------|
| `sample_sales.csv` | 표준 테스트 파일 — 10행 5열(날짜/카테고리/매출/수량/지역), 한국어 값 포함, 차트 생성 테스트용 |
| `sample_wide.csv` | 넓은 파일 — 10열 5행, DataSummaryView 다수 컬럼 표시 테스트용 |
| `sample_narrow.csv` | 좁은 파일 — 3열 3행, 최소 데이터 표시 테스트용 |
| `sample_missing.csv` | 결측값 파일 — 5열 8행, 일부 셀 공백으로 null 카운트 표시 테스트용 |
| `sample_header_only.csv` | 헤더만 있는 파일 — 데이터 행 없음, 빈 데이터 처리 테스트용 |
| `sample_korean_path_test.csv` | 한글 경로 테스트용 — 한글 폴더명 경로에서 파일 열기 테스트용 |
| `sample_invalid.txt` | 잘못된 확장자 파일 — CSV가 아닌 .txt 파일 오류 처리 테스트용 |

모든 CSV 파일은 UTF-8 with BOM 인코딩으로 한국어 호환성을 지원합니다.

## 스크린샷 출력

캡처된 스크린샷은 `tests/screenshots/` 디렉토리에 저장됩니다.
