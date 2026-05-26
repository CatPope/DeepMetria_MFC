// test_crash_recovery_tdd.cpp
// TDD 테스트: 크래시 복구 (NFR-03)
//
// 요구사항:
//   NFR-03: 앱 비정상 종료 후 재시작 시 마지막 작업 상태 복원
//
// 현재 상태: 크래시 복구 미구현
// 모든 테스트는 DISABLED_ 접두사로 비활성화되어 있다.
// 구현 완료 후 DISABLED_ 를 제거하여 활성화한다.
//
// TDD: 크래시 복구 미구현 — 구현 후 활성화
//
// 설계 계약:
//   - 상태 파일 경로: %APPDATA%\DeepMetria\last_state.json
//   - 상태 포함 항목: 마지막 열린 파일 경로, 활성 대시보드 ID, 창 위치
//   - 복원 판별: 상태 파일 존재 + "dirty" 플래그 = TRUE → 복원 제안
//   - 손상 감지: JSON 파싱 실패 시 상태 파일 삭제 후 정상 시작

#include <gtest/gtest.h>
#include <windows.h>
#include <fstream>
#include <string>

#include "Common/Types.h"

// ============================================================
// 예상 인터페이스 명세 (미구현 — TDD RED 단계)
// 아래 클래스/함수는 구현되어야 할 계약을 정의한다.
// ============================================================
namespace CrashRecovery {

// 저장할 앱 상태 구조체
struct AppLastState {
    CString lastFilePath;       // 마지막으로 열린 파일 경로
    CString activeDashboardId;  // 활성 대시보드 ID (없으면 빈 문자열)
    int     windowX;            // 창 X 위치
    int     windowY;            // 창 Y 위치
    int     windowW;            // 창 너비
    int     windowH;            // 창 높이
    bool    dirty;              // 비정상 종료 판별 플래그 (종료 시 false 로 초기화)

    AppLastState()
        : windowX(100), windowY(100), windowW(1200), windowH(800), dirty(false) {}
};

// 상태 파일 기본 경로 반환 헬퍼 (테스트 오버라이드 가능)
CString GetStateFilePath() {
    TCHAR appData[MAX_PATH] = {};
    ::GetEnvironmentVariable(_T("APPDATA"), appData, MAX_PATH);
    CString path(appData);
    path += _T("\\DeepMetria\\last_state.json");
    return path;
}

// 상태 파일 경로를 받아 AppLastState 를 JSON 으로 직렬화 후 저장
// 반환: TRUE = 성공, FALSE = 실패
// [미구현 스텁 — TDD RED]
inline BOOL SaveLastState(const AppLastState& /*state*/, const CString& /*path*/) {
    return FALSE;  // 스텁: 미구현
}

// 상태 파일 경로에서 AppLastState 를 역직렬화하여 로드
// 반환: TRUE = 복원 가능 상태 존재, FALSE = 없거나 실패
// [미구현 스텁 — TDD RED]
inline BOOL LoadLastState(const CString& /*path*/, AppLastState& /*outState*/) {
    return FALSE;  // 스텁: 미구현
}

// 손상된 상태 파일을 감지하고 안전하게 삭제한다.
// 반환: TRUE = 손상 감지 후 삭제 완료, FALSE = 정상 파일
// [미구현 스텁 — TDD RED]
inline BOOL HandleCorruptState(const CString& /*path*/) {
    return FALSE;  // 스텁: 미구현
}

} // namespace CrashRecovery

// ============================================================
// 테스트 픽스처
// ============================================================
class CrashRecoveryTDDTest : public ::testing::Test {
protected:
    // 실제 AppData 오염 방지 — 임시 경로 사용
    CString stateFilePath;

    void SetUp() override {
        wchar_t tempDir[MAX_PATH] = {};
        ::GetTempPathW(MAX_PATH, tempDir);
        stateFilePath = CString(tempDir) + _T("deepmetria_test_last_state.json");
        // 이전 테스트 잔여물 정리
        ::DeleteFileW(stateFilePath);
    }

    void TearDown() override {
        ::DeleteFileW(stateFilePath);
    }
};

// ============================================================
// 1. SaveLastState  [TDD]
// 앱 종료 시 AppLastState 가 JSON 파일로 직렬화되어야 한다.
// TDD: 크래시 복구 미구현 — 구현 후 활성화
// ============================================================
TEST_F(CrashRecoveryTDDTest, DISABLED_SaveLastState) {
    // TDD: CrashRecovery::SaveLastState 미구현 — 구현 후 활성화
    //
    // 구현 요구사항:
    //   1. AppLastState 를 JSON 문자열로 직렬화
    //   2. %APPDATA%\DeepMetria\ 디렉터리가 없으면 생성
    //   3. last_state.json 파일에 원자적으로 저장 (임시 파일 → 이름 변경)
    //   4. dirty = true 로 저장 (종료 전 true, 정상 종료 시 false 로 갱신)

    CrashRecovery::AppLastState state;
    state.lastFilePath      = _T("C:\\Users\\test\\data\\sales.csv");
    state.activeDashboardId = _T("dash-001");
    state.windowX = 200; state.windowY = 150;
    state.windowW = 1280; state.windowH = 720;
    state.dirty = true;

    BOOL ok = CrashRecovery::SaveLastState(state, stateFilePath);
    ASSERT_TRUE(ok) << "SaveLastState 가 FALSE 반환 — 구현 확인 필요";

    // 파일이 생성되어야 한다.
    DWORD attr = ::GetFileAttributesW(stateFilePath);
    EXPECT_NE(attr, INVALID_FILE_ATTRIBUTES)
        << "SaveLastState 호출 후 파일이 생성되지 않음: "
        << (LPCWSTR)stateFilePath;

    // 파일 크기가 0 보다 커야 한다 (빈 파일 방지).
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    if (::GetFileAttributesExW(stateFilePath, GetFileExInfoStandard, &fad)) {
        EXPECT_GT(fad.nFileSizeLow, 0U) << "상태 파일이 비어 있음";
    }
}

// ============================================================
// 2. RestoreLastState  [TDD]
// 앱 시작 시 dirty = true 인 상태 파일이 존재하면
// AppLastState 를 복원해야 한다.
// TDD: 크래시 복구 미구현 — 구현 후 활성화
// ============================================================
TEST_F(CrashRecoveryTDDTest, DISABLED_RestoreLastState) {
    // TDD: CrashRecovery::LoadLastState 미구현 — 구현 후 활성화
    //
    // 구현 요구사항:
    //   1. 상태 파일이 없으면 FALSE 반환 (정상 시작)
    //   2. 파일이 있고 dirty == true 이면 TRUE 반환 및 outState 채우기
    //   3. 파일이 있고 dirty == false 이면 FALSE 반환 (정상 종료된 이전 세션)

    // 1단계: 상태 저장
    CrashRecovery::AppLastState saved;
    saved.lastFilePath      = _T("C:\\Users\\test\\data\\sales.csv");
    saved.activeDashboardId = _T("dash-001");
    saved.dirty = true;

    BOOL saveOk = CrashRecovery::SaveLastState(saved, stateFilePath);
    ASSERT_TRUE(saveOk) << "SaveLastState 실패 — RestoreLastState 테스트 전제 조건 미충족";

    // 2단계: 상태 복원
    CrashRecovery::AppLastState restored;
    BOOL loadOk = CrashRecovery::LoadLastState(stateFilePath, restored);

    ASSERT_TRUE(loadOk)
        << "LoadLastState 가 FALSE 반환 — dirty=true 상태 파일이므로 복원 가능해야 함";

    // 복원된 값이 저장 값과 일치해야 한다.
    EXPECT_EQ(restored.lastFilePath, saved.lastFilePath)
        << "복원된 lastFilePath 불일치";
    EXPECT_EQ(restored.activeDashboardId, saved.activeDashboardId)
        << "복원된 activeDashboardId 불일치";
    EXPECT_TRUE(restored.dirty)
        << "복원된 dirty 플래그가 false — 비정상 종료 감지 실패";
}

// ============================================================
// 3. CorruptStateFile  [TDD]
// 손상된 상태 파일(유효하지 않은 JSON)이 있을 때
// 앱이 파일을 삭제하고 정상적으로 시작해야 한다.
// TDD: 크래시 복구 미구현 — 구현 후 활성화
// ============================================================
TEST_F(CrashRecoveryTDDTest, DISABLED_CorruptStateFile) {
    // TDD: CrashRecovery::HandleCorruptState 미구현 — 구현 후 활성화
    //
    // 구현 요구사항:
    //   1. 상태 파일을 읽어 JSON 파싱 시도
    //   2. 파싱 실패(손상) 시 해당 파일 삭제
    //   3. FALSE 반환하여 정상 시작 경로로 진행

    // 손상된 파일 생성 (유효하지 않은 JSON)
    {
        std::ofstream f(static_cast<LPCTSTR>(stateFilePath));
        ASSERT_TRUE(f.is_open()) << "손상 파일 생성 실패";
        f << "{ this is NOT valid JSON !!!  broken <<<>>>";
    }

    // 손상 감지 및 처리
    BOOL isCorrupt = CrashRecovery::HandleCorruptState(stateFilePath);
    EXPECT_TRUE(isCorrupt) << "HandleCorruptState 가 손상 파일을 감지하지 못함";

    // 손상된 파일이 삭제되어야 한다.
    DWORD attr = ::GetFileAttributesW(stateFilePath);
    EXPECT_EQ(attr, INVALID_FILE_ATTRIBUTES)
        << "HandleCorruptState 호출 후 손상 파일이 삭제되지 않음";

    // 삭제 후 LoadLastState 는 FALSE 를 반환해야 한다 (정상 시작).
    CrashRecovery::AppLastState outState;
    BOOL loadOk = CrashRecovery::LoadLastState(stateFilePath, outState);
    EXPECT_FALSE(loadOk)
        << "손상 파일 삭제 후 LoadLastState 가 TRUE 반환 — 상태 없어야 함";
}
