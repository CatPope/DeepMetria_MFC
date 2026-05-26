"""
DeepMetria MFC 테스트 공통 헬퍼 함수
pywinauto win32 백엔드 기반
"""
import os
import time
import datetime

from pywinauto import Application, timings
from pywinauto.keyboard import send_keys

from config import (
    APP_EXE, PROJECT_ROOT, SCREENSHOT_DIR, CLAUDE_TEMP_DIR,
    APP_START_TIMEOUT, ANALYSIS_TIMEOUT, FILE_LOAD_TIMEOUT,
)


# ---------------------------------------------------------------------------
# 앱 시작 / 연결
# ---------------------------------------------------------------------------

def start_app():
    """DeepMetria.exe 를 새로 실행하고 Application 객체를 반환한다."""
    exe_path = os.path.join(PROJECT_ROOT, APP_EXE)
    app = Application(backend="win32").start(exe_path)
    # 메인 윈도우가 나타날 때까지 대기
    timings.wait_until(
        APP_START_TIMEOUT,
        0.5,
        lambda: len(app.windows()) > 0,
    )
    time.sleep(1)  # MFC 초기화 여유 시간
    return app


def connect_app():
    """이미 실행 중인 DeepMetria 에 연결하고 Application 객체를 반환한다."""
    app = Application(backend="win32").connect(title_re=".*DeepMetria.*")
    return app


# ---------------------------------------------------------------------------
# 메인 윈도우
# ---------------------------------------------------------------------------

def get_main_window(app):
    """타이틀에 'DeepMetria' 가 포함된 메인 윈도우를 반환한다."""
    return app.window(title_re=".*DeepMetria.*")


# ---------------------------------------------------------------------------
# 스크린샷
# ---------------------------------------------------------------------------

def _ensure_dirs():
    os.makedirs(SCREENSHOT_DIR, exist_ok=True)
    os.makedirs(CLAUDE_TEMP_DIR, exist_ok=True)


def capture(win, test_id: str, description: str) -> str:
    """
    윈도우 전체 스크린샷을 캡처한다.
    SCREENSHOT_DIR 과 CLAUDE_TEMP_DIR 양쪽에 저장하고 경로를 반환한다.
    """
    _ensure_dirs()
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{test_id}_{description}_{ts}.png"

    main_path = os.path.join(SCREENSHOT_DIR, filename)
    temp_path = os.path.join(CLAUDE_TEMP_DIR, filename)

    try:
        img = win.capture_as_image()
        img.save(main_path)
        img.save(temp_path)
        print(f"  [SCREENSHOT] {main_path}")
    except Exception as exc:
        print(f"  [SCREENSHOT ERROR] {exc}")

    return main_path


def capture_control(control, test_id: str, description: str) -> str:
    """특정 컨트롤의 스크린샷을 캡처한다."""
    _ensure_dirs()
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"{test_id}_{description}_ctrl_{ts}.png"

    main_path = os.path.join(SCREENSHOT_DIR, filename)
    temp_path = os.path.join(CLAUDE_TEMP_DIR, filename)

    try:
        img = control.capture_as_image()
        img.save(main_path)
        img.save(temp_path)
        print(f"  [SCREENSHOT] {main_path}")
    except Exception as exc:
        print(f"  [SCREENSHOT ERROR] {exc}")

    return main_path


# ---------------------------------------------------------------------------
# 파일 열기
# ---------------------------------------------------------------------------

def open_file(app, win, filepath: str):
    """
    Ctrl+O 단축키로 파일 열기 대화상자를 열고,
    filepath 를 입력한 후 Enter 로 확인한다.
    """
    win.set_focus()
    send_keys("^o")  # Ctrl+O

    # 표준 Windows 파일 대화상자(#32770)가 나타날 때까지 대기
    try:
        timings.wait_until(
            FILE_LOAD_TIMEOUT,
            0.3,
            lambda: len(app.windows(class_name="#32770")) > 0,
        )
    except timings.TimeoutError:
        raise RuntimeError("파일 열기 대화상자가 열리지 않았습니다.")

    # 파일명 편집 컨트롤에 경로 입력
    # 표준 CFileDialog 의 파일명 Edit 컨트롤 class 는 "Edit"
    file_dialog = app.window(class_name="#32770")
    file_dialog.set_focus()

    # 파일명 입력 필드를 찾아 경로를 붙여넣기
    try:
        filename_edit = file_dialog.child_window(class_name="Edit", found_index=0)
        filename_edit.set_focus()
        filename_edit.set_text(filepath)
    except Exception:
        # fallback: 키보드로 직접 입력
        send_keys(filepath, with_spaces=True)

    send_keys("{ENTER}")
    time.sleep(1)  # 파일 로드 여유 시간


# ---------------------------------------------------------------------------
# QueryInputView 헬퍼
# ---------------------------------------------------------------------------

def get_status_text(win) -> str:
    """QueryInputView 의 CStatic(IDC_STATIC_STATUS) 텍스트를 반환한다."""
    try:
        status = win.child_window(class_name="Static", found_index=0)
        return status.window_text()
    except Exception as exc:
        print(f"  [WARN] get_status_text: {exc}")
        return ""


def type_query(win, text: str):
    """QueryInputView 의 CEdit 에 텍스트를 입력한다."""
    edit = win.child_window(class_name="Edit", found_index=0)
    edit.set_focus()
    edit.set_text(text)


def click_analyze(win):
    """'분석' 버튼(IDC_BTN_ANALYZE)을 클릭한다."""
    btn = win.child_window(title="분석", class_name="Button")
    btn.click()


def wait_for_analysis(win, timeout: int = ANALYSIS_TIMEOUT) -> str:
    """
    분석이 완료될 때까지 상태 텍스트를 폴링한다.
    '분석 완료' 또는 오류 키워드가 나타나면 해당 텍스트를 반환한다.
    timeout 초 내에 완료되지 않으면 TimeoutError 를 발생시킨다.
    """
    end_time = time.time() + timeout
    while time.time() < end_time:
        text = get_status_text(win)
        if "완료" in text or "오류" in text or "실패" in text or "error" in text.lower():
            return text
        time.sleep(0.5)
    raise TimeoutError(f"분석이 {timeout}초 내에 완료되지 않았습니다. 마지막 상태: {get_status_text(win)}")


# ---------------------------------------------------------------------------
# DataSummaryView 헬퍼
# ---------------------------------------------------------------------------

def get_listview_items(win) -> list:
    """
    DataSummaryView 의 SysListView32 에서 모든 행의 텍스트를 읽어
    list[list[str]] 형태로 반환한다.
    """
    try:
        lv = win.child_window(class_name="SysListView32")
        items = []
        count = lv.item_count()
        col_count = lv.column_count()
        for row in range(count):
            row_data = []
            for col in range(col_count):
                row_data.append(lv.get_item(row, col).text())
            items.append(row_data)
        return items
    except Exception as exc:
        print(f"  [WARN] get_listview_items: {exc}")
        return []


def get_listview_columns(win) -> list:
    """SysListView32 의 컬럼 헤더 텍스트 목록을 반환한다."""
    try:
        lv = win.child_window(class_name="SysListView32")
        return [col['text'] for col in lv.columns()]
    except Exception as exc:
        print(f"  [WARN] get_listview_columns: {exc}")
        return []


# ---------------------------------------------------------------------------
# 설정 대화상자
# ---------------------------------------------------------------------------

def open_settings(app, win):
    """Tools > Settings 메뉴를 통해 설정 대화상자를 연다."""
    win.set_focus()
    try:
        win.menu_select("Tools->Settings")
    except Exception:
        # 메뉴 타이틀이 다를 경우 대안 시도
        try:
            win.menu_select("도구->설정")
        except Exception as exc:
            print(f"  [WARN] open_settings: {exc}")
