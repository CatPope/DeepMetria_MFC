"""
DeepMetria MFC 자동화 테스트 설정
사용자가 DEEPMETRIA_EXE 환경변수를 설정하거나 APP_EXE를 직접 수정하세요.
"""
import os

# 앱 실행 파일 경로 — 환경변수 또는 기본값
APP_EXE = os.environ.get("DEEPMETRIA_EXE", r"x64\Release\DeepMetria.exe")

# 프로젝트 루트 (tests/ 의 상위)
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 테스트 데이터 디렉토리
DATA_DIR = os.path.join(PROJECT_ROOT, "tests", "data")

# 스크린샷 출력 디렉토리
SCREENSHOT_DIR = os.path.join(PROJECT_ROOT, "tests", "screenshots")

# Claude 분석용 임시 디렉토리
CLAUDE_TEMP_DIR = os.path.join(PROJECT_ROOT, ".claude", "temp")

# 테스트 CSV 파일 경로
CSV_STANDARD = os.path.join(DATA_DIR, "sample_sales.csv")
CSV_WIDE = os.path.join(DATA_DIR, "sample_wide.csv")
CSV_NARROW = os.path.join(DATA_DIR, "sample_narrow.csv")
CSV_MISSING = os.path.join(DATA_DIR, "sample_missing.csv")
CSV_HEADER_ONLY = os.path.join(DATA_DIR, "sample_header_only.csv")
INVALID_FILE = os.path.join(DATA_DIR, "sample_invalid.txt")

# 타임아웃 (초)
APP_START_TIMEOUT = 10
ANALYSIS_TIMEOUT = 90
FILE_LOAD_TIMEOUT = 10
