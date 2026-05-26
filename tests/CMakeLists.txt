cmake_minimum_required(VERSION 3.20)
project(DeepMetriaTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Google Test — FetchContent로 자동 다운로드
# ============================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Windows: 공유 런타임 DLL 강제 방지 (MSVC 기본값 맞춤)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ============================================================
# 프로젝트 소스 루트 경로
# ============================================================
set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../src")

# ============================================================
# 공통 include 경로
# ============================================================
set(COMMON_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures"  # stdafx.h 스텁이 src/stdafx.h 보다 먼저 탐색됨
    "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
    "${SRC_ROOT}"
)

# ============================================================
# Windows 공통 링크 라이브러리
# ============================================================
set(WIN_LIBS
    ws2_32      # Winsock (libcurl 의존성)
    crypt32     # DPAPI (CryptProtectData / CryptUnprotectData)
    sqlite3     # SQLite3 (vcpkg 또는 수동 빌드)
    libcurl     # HTTP 클라이언트 (LLM 프로바이더 의존성)
)

# ============================================================
# 단위 테스트 실행 파일: deepmetria_unit_tests
# ============================================================
file(GLOB_RECURSE UNIT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
)

# 단위 테스트에 직접 컴파일하는 구현 소스
# (ATL 전용 환경에서 빌드 가능 — MFC 의존성 없음)
set(UNIT_IMPL_SOURCES
    # 분석 도구
    "${SRC_ROOT}/Domain/Analysis/AnalysisTools.cpp"
    "${SRC_ROOT}/Domain/Analysis/ChartSelector.cpp"
    "${SRC_ROOT}/Infrastructure/Cache/AnalysisCache.cpp"
    # 파서
    "${SRC_ROOT}/Infrastructure/Parser/CSVParser.cpp"
    "${SRC_ROOT}/Infrastructure/Parser/ExcelParser.cpp"
    # 데이터 소스
    "${SRC_ROOT}/Domain/DataSource/DataSourceService.cpp"
    # LLM (libcurl 링크 필요)
    "${SRC_ROOT}/Infrastructure/LLM/LLMRouter.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/ClaudeProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/OpenAIProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/GeminiProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/HttpClient.cpp"
)

if(UNIT_SOURCES)
    add_executable(deepmetria_unit_tests ${UNIT_SOURCES} ${UNIT_IMPL_SOURCES})

    target_include_directories(deepmetria_unit_tests PRIVATE ${COMMON_INCLUDES})

    target_link_libraries(deepmetria_unit_tests PRIVATE
        gtest_main
        gmock_main
        ${WIN_LIBS}
    )

    # Unicode 빌드 (MFC와 동일 설정)
    target_compile_definitions(deepmetria_unit_tests PRIVATE
        _UNICODE
        UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        DEEPMETRIA_UNIT_TEST
    )

    # CTest 등록
    include(GoogleTest)
    gtest_discover_tests(deepmetria_unit_tests)
endif()

# ============================================================
# 통합 테스트 실행 파일: deepmetria_integration_tests
# ============================================================
file(GLOB_RECURSE INTEGRATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
)

# 통합 테스트에 직접 컴파일하는 구현 소스
set(INTEGRATION_IMPL_SOURCES
    # 분석 도구
    "${SRC_ROOT}/Domain/Analysis/AnalysisTools.cpp"
    "${SRC_ROOT}/Domain/Analysis/ChartSelector.cpp"
    "${SRC_ROOT}/Infrastructure/Cache/AnalysisCache.cpp"
    # 파서
    "${SRC_ROOT}/Infrastructure/Parser/CSVParser.cpp"
    "${SRC_ROOT}/Infrastructure/Parser/ExcelParser.cpp"
    # 데이터 소스
    "${SRC_ROOT}/Domain/DataSource/DataSourceService.cpp"
    # 오케스트레이션
    "${SRC_ROOT}/Domain/Orchestrator/AnalysisFlow.cpp"
    # LLM (libcurl 링크 필요)
    "${SRC_ROOT}/Infrastructure/LLM/LLMRouter.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/ClaudeProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/OpenAIProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/GeminiProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/HttpClient.cpp"
    # 대시보드
    "${SRC_ROOT}/Domain/Dashboard/DashboardManager.cpp"
    # 스토리지
    "${SRC_ROOT}/Infrastructure/Storage/SQLiteDB.cpp"
)

if(INTEGRATION_SOURCES)
    add_executable(deepmetria_integration_tests ${INTEGRATION_SOURCES} ${INTEGRATION_IMPL_SOURCES})

    target_include_directories(deepmetria_integration_tests PRIVATE ${COMMON_INCLUDES})

    target_link_libraries(deepmetria_integration_tests PRIVATE
        gtest_main
        gmock_main
        ${WIN_LIBS}
    )

    target_compile_definitions(deepmetria_integration_tests PRIVATE
        _UNICODE
        UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        DEEPMETRIA_UNIT_TEST
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    )

    include(GoogleTest)
    gtest_discover_tests(deepmetria_integration_tests)
endif()

# ============================================================
# CTest 활성화
# ============================================================
enable_testing()
