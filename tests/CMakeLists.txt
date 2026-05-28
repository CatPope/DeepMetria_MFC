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
# ATL include 경로 자동 탐색 (atlstr.h 위치)
# ============================================================
set(ATL_INCLUDE_DIR "")
file(GLOB _MSVC_VERSIONS
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/*/atlmfc/include"
)
list(SORT _MSVC_VERSIONS ORDER DESCENDING)
if(_MSVC_VERSIONS)
    list(GET _MSVC_VERSIONS 0 ATL_INCLUDE_DIR)
    message(STATUS "ATL include: ${ATL_INCLUDE_DIR}")
endif()

# ATL lib 경로 자동 탐색 (atls.lib 위치) — 링크 시 필요
set(ATL_LIB_DIR "")
file(GLOB _MSVC_LIB_VERSIONS
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/*/atlmfc/lib"
)
list(SORT _MSVC_LIB_VERSIONS ORDER DESCENDING)
if(_MSVC_LIB_VERSIONS)
    list(GET _MSVC_LIB_VERSIONS 0 _ATL_LIB_BASE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ATL_LIB_DIR "${_ATL_LIB_BASE}/x64")
    else()
        set(ATL_LIB_DIR "${_ATL_LIB_BASE}/x86")
    endif()
    message(STATUS "ATL lib: ${ATL_LIB_DIR}")
endif()

# ============================================================
# 공통 include 경로
# ============================================================
set(COMMON_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures"  # stdafx.h 스텁이 src/stdafx.h 보다 먼저 탐색됨
    "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
    "${SRC_ROOT}"
)
if(ATL_INCLUDE_DIR)
    list(APPEND COMMON_INCLUDES "${ATL_INCLUDE_DIR}")
endif()

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
    "${SRC_ROOT}/Infrastructure/Parser/JsonParser.cpp"
    # 데이터 소스
    "${SRC_ROOT}/Domain/DataSource/DataSourceService.cpp"
    # 대시보드 (+ SQLiteDB 의존)
    "${SRC_ROOT}/Domain/Dashboard/DashboardManager.cpp"
    "${SRC_ROOT}/Infrastructure/Storage/SQLiteDB.cpp"
    # LLM (libcurl 링크 필요)
    "${SRC_ROOT}/Infrastructure/LLM/LLMRouter.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/ClaudeProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/OpenAIProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/GeminiProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/HttpClient.cpp"
    # 설정 관리
    "${SRC_ROOT}/Infrastructure/Settings/CSettingsManager.cpp"
)

if(UNIT_SOURCES)
    add_executable(deepmetria_unit_tests ${UNIT_SOURCES} ${UNIT_IMPL_SOURCES})

    target_include_directories(deepmetria_unit_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${COMMON_INCLUDES}
    )

    if(ATL_LIB_DIR)
        target_link_directories(deepmetria_unit_tests PRIVATE "${ATL_LIB_DIR}")
    endif()

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
        TRACE=
    )

    # MSVC가 소스의 UTF-8(한국어 주석)을 cp949로 오인하지 않도록 강제
    if(MSVC)
        target_compile_options(deepmetria_unit_tests PRIVATE /utf-8)
    endif()

    # CTest 등록
    include(GoogleTest)
    gtest_discover_tests(deepmetria_unit_tests)
endif()

# ============================================================
# LLM 라우터 집중 테스트 실행 파일: deepmetria_llm_tests
# 전체 unit 타깃에는 무관한 선행 결함(TRACE/min/max/fixtures 경로)이 있어
# LLM 폴백 동작만 격리 검증하기 위한 최소 타깃이다.
# ============================================================
add_executable(deepmetria_llm_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_llm_router.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/LLMRouter.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/ClaudeProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/OpenAIProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/GeminiProvider.cpp"
    "${SRC_ROOT}/Infrastructure/LLM/HttpClient.cpp"
)

target_include_directories(deepmetria_llm_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"   # mocks/, fixtures/ 접두 포함 해소
    ${COMMON_INCLUDES}
)

if(ATL_LIB_DIR)
    target_link_directories(deepmetria_llm_tests PRIVATE "${ATL_LIB_DIR}")
endif()

target_link_libraries(deepmetria_llm_tests PRIVATE
    gtest_main
    gmock_main
    ${WIN_LIBS}
)

target_compile_definitions(deepmetria_llm_tests PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    DEEPMETRIA_UNIT_TEST
    TRACE=   # MFC 전용 TRACE 매크로 무력화: TRACE(a,b) -> (a,b) 콤마식(유효)
)

if(MSVC)
    # /utf-8: 한국어 주석 UTF-8 강제 인식
    target_compile_options(deepmetria_llm_tests PRIVATE /utf-8)
endif()

include(GoogleTest)
gtest_discover_tests(deepmetria_llm_tests)

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
    "${SRC_ROOT}/Infrastructure/Parser/JsonParser.cpp"
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

    target_include_directories(deepmetria_integration_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${COMMON_INCLUDES}
    )

    if(ATL_LIB_DIR)
        target_link_directories(deepmetria_integration_tests PRIVATE "${ATL_LIB_DIR}")
    endif()

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
        TRACE=
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    )

    if(MSVC)
        # /utf-8: 한국어 주석 UTF-8 강제 인식
        # /FI stdafx.h: 모든 TU가 fixtures/stdafx.h(CString=atlstr.h 제공)를
        #   먼저 포함하도록 강제 — 일부 테스트가 stdafx.h를 직접 include하지
        #   않아 Common/Types.h의 CString 사용이 미정의되는 문제 해소
        target_compile_options(deepmetria_integration_tests PRIVATE /utf-8 /FIstdafx.h)
    endif()

    include(GoogleTest)
    gtest_discover_tests(deepmetria_integration_tests)
endif()

# ============================================================
# CTest 활성화
# ============================================================
enable_testing()

# ============================================================
# Export 포맷터 집중 테스트 실행 파일: deepmetria_export_tests
# ExportFormatter.h는 헤더 전용(ATL CString + std)이라 컴파일할 .cpp가 없다.
# deepmetria_llm_tests 패턴을 미러링: /utf-8 + ATL include + 최소 라이브러리.
# libcurl/sqlite 불필요 — CSV 수식 인젝션 가드/HTML escape를 격리 검증한다.
# ============================================================
add_executable(deepmetria_export_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/test_export_formatter.cpp"
)

target_include_directories(deepmetria_export_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"   # mocks/, fixtures/ 접두 포함 해소
    ${COMMON_INCLUDES}
)

if(ATL_LIB_DIR)
    target_link_directories(deepmetria_export_tests PRIVATE "${ATL_LIB_DIR}")
endif()

target_link_libraries(deepmetria_export_tests PRIVATE
    gtest_main
    gmock_main
)

target_compile_definitions(deepmetria_export_tests PRIVATE
    _UNICODE
    UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    DEEPMETRIA_UNIT_TEST
)

if(MSVC)
    # /utf-8: 한국어 주석 UTF-8 강제 인식
    target_compile_options(deepmetria_export_tests PRIVATE /utf-8)
endif()

include(GoogleTest)
gtest_discover_tests(deepmetria_export_tests)
